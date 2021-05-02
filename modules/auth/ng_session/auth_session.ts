import { Component, Injectable, Directive, TemplateRef, OnInit, Output, Injector, ElementRef, ViewContainerRef, EventEmitter, Type, ComponentFactoryResolver } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders } from '@angular/common/http';
import { Observable, BehaviorSubject } from 'rxjs';
import { catchError, retry, map, takeWhile, tap, mergeMap } from 'rxjs/operators'
import { throwError, of, timer } from 'rxjs';
import { interval } from 'rxjs/internal/observable/interval';
import * as CryptoJS from 'crypto-js';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';

//-----------------------------------------------------------------------------

const SESSION_STORAGE_TOKEN        = 'session_token';
const SESSION_STORAGE_FIRSTNAME    = 'session_fn';
const SESSION_STORAGE_LASTNAME     = 'session_ln';
const SESSION_STORAGE_WORKSPACE    = 'session_wp';
const SESSION_STORAGE_LT           = 'session_lt';
const SESSION_STORAGE_VP           = 'session_vp';
const SESSION_STORAGE_WPID         = 'session_wpid';
const SESSION_STORAGE_GPID         = 'session_gpid';

//-----------------------------------------------------------------------------

@Injectable()
export class AuthSession
{
  private session_key: string;
  private interval_obj;
  private timer_idle_countdown;

  // use the default values
  private login_component: Type<any> = AuthSessionLoginComponent;
  private login_modal: boolean = true;

  private user: string = null;
  private pass: string = null;
  private wpid: string = null;

  // initialize the session emitter
  public session: BehaviorSubject<AuthSessionItem>;
  public roles: BehaviorSubject<object>;
  public idle: EventEmitter<number> = new EventEmitter();

  //-----------------------------------------------------------------------------

  constructor (private http: HttpClient, private modal_service: NgbModal)
  {
    this.roles = new BehaviorSubject(null);

    // get the last update timestamp
    this.wpid = sessionStorage.getItem (SESSION_STORAGE_WPID);
    if (this.wpid)
    {
      this.session = new BehaviorSubject({token: sessionStorage.getItem (SESSION_STORAGE_TOKEN), firstname: sessionStorage.getItem (SESSION_STORAGE_FIRSTNAME), lastname: sessionStorage.getItem (SESSION_STORAGE_LASTNAME), workspace: sessionStorage.getItem (SESSION_STORAGE_WORKSPACE), lt: sessionStorage.getItem (SESSION_STORAGE_LT), vp: Number(sessionStorage.getItem (SESSION_STORAGE_VP)), wpid: Number(sessionStorage.getItem (SESSION_STORAGE_WPID)), gpid: Number(sessionStorage.getItem (SESSION_STORAGE_GPID)), state: 0, user: null});

      this.json_rpc ('AUTH', 'session_roles', {}).subscribe ((data: object) => this.roles.next (data));
      this.timer_set (Number(sessionStorage.getItem (SESSION_STORAGE_VP)));
    }
    else
    {
      this.session = new BehaviorSubject(null);
    }
  }

  //-----------------------------------------------------------------------------

  public set_content_type (type: any, modal: boolean = false)
  {
    console.log('set custom content type');

    this.login_component = type;
    this.login_modal = modal;
  }

  //-----------------------------------------------------------------------------

  public get_content_type (): Type<any>
  {
    return this.login_component;
  }

  //---------------------------------------------------------------------------

  public enable (user: string, pass: string): EventEmitter<AuthSessionItem>
  {
    var response: EventEmitter<AuthSessionItem> = new EventEmitter<AuthSessionItem>();

    // set the credentials
    this.user = user;
    this.pass = pass;

    // try to get a session
    this.fetch_session (response);

    return response;
  }

  //---------------------------------------------------------------------------

  public disable (): void
  {
    this.storage_clear ();
    this.roles.next (null);
  }

  //---------------------------------------------------------------------------

  public contains_role__or (roles: object, permissions: string[]): boolean
  {
    if (roles && Array.isArray (permissions))
    {
      // if we don't have any entries it shall alwasy return true
      // -> this can be used to activate / deactivate parts depending
      // -> on the roles existenz only
      if (permissions.length == 0)
      {
        return true;
      }

      // check if permissions are in the roles
      for (var i in permissions)
      {
        if (roles [permissions[i]] != undefined)
        {
          return true;
        }
      }
    }

    return false;
  }

  //-----------------------------------------------------------------------------

  public name_gpid (gpid: number): string
  {
    return '';
  }

  //---------------------------------------------------------------------------

  private timer_set (idle_countdown: number)
  {
    // reset the idle countdown
    this.timer_idle_countdown = idle_countdown;

    if (this.interval_obj)
    {

    }
    else
    {
      this.interval_obj = interval(1000).subscribe(x => {

        this.timer_idle_countdown--;

        this.idle.emit (this.timer_idle_countdown);

        if (this.timer_idle_countdown == 0) this.disable ();

      });
    }
  }

  //---------------------------------------------------------------------------

  private timer_clear ()
  {
    if (this.interval_obj) this.interval_obj.unsubscribe();
    this.interval_obj = null;
  }

  //---------------------------------------------------------------------------

  private timer_update ()
  {
    var s: AuthSessionItem = this.session.getValue();
    this.timer_idle_countdown = s.vp;
  }

  //---------------------------------------------------------------------------

  private storage_set (sitem: AuthSessionItem): void
  {
    sessionStorage.setItem (SESSION_STORAGE_TOKEN, sitem.token);
    sessionStorage.setItem (SESSION_STORAGE_FIRSTNAME, sitem.firstname);
    sessionStorage.setItem (SESSION_STORAGE_LASTNAME, sitem.lastname);
    sessionStorage.setItem (SESSION_STORAGE_WORKSPACE, sitem.workspace);
    sessionStorage.setItem (SESSION_STORAGE_LT, sitem.lt);
    sessionStorage.setItem (SESSION_STORAGE_VP, String(sitem.vp));
    sessionStorage.setItem (SESSION_STORAGE_WPID, String(sitem.wpid));
    sessionStorage.setItem (SESSION_STORAGE_GPID, String(sitem.gpid));

    this.session.next (sitem);
    this.timer_set (sitem.vp);

    console.log('session was set, token = ' + sitem.token);
    let sitem2: AuthSessionItem = this.session ? this.session.value : null;

    if (sitem2)
    {
      console.log('session was get, token = ' + sitem2.token);
    }
  }

  //---------------------------------------------------------------------------

  private storage_clear (): void
  {
    this.timer_clear ();

    this.user = null;
    this.pass = null;
    this.wpid = null;

    sessionStorage.removeItem (SESSION_STORAGE_TOKEN);
    sessionStorage.removeItem (SESSION_STORAGE_FIRSTNAME);
    sessionStorage.removeItem (SESSION_STORAGE_LASTNAME);
    sessionStorage.removeItem (SESSION_STORAGE_WORKSPACE);
    sessionStorage.removeItem (SESSION_STORAGE_LT);
    sessionStorage.removeItem (SESSION_STORAGE_VP);
    sessionStorage.removeItem (SESSION_STORAGE_WPID);
    sessionStorage.removeItem (SESSION_STORAGE_GPID);

    this.session.next (null);
  }

  //---------------------------------------------------------------------------

  public json_rpc<T> (qbus_module: string, qbus_method: string, params: object): Observable<T>
  {
    var header: object;
    var sitem: AuthSessionItem = this.session ? this.session.value : null;

    if (sitem)
    {
      header = {headers: new HttpHeaders ({'Authorization': "Bearer " + sitem.token, 'Cache-Control': 'no-cache', 'Pragma': 'no-cache'})};
    }
    else
    {
      header = {};
    }

    return this.handle_error_session<T> (this.http.post<T>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params), header));
  }

  //---------------------------------------------------------------------------

  public json_rpc_blob (qbus_module: string, qbus_method: string, params: object): Observable<string>
  {
    var options: object;
    var sitem: AuthSessionItem = this.session ? this.session.value : null;

    if (sitem)
    {
      options = {headers: new HttpHeaders ({'Authorization': "Bearer " + sitem.token, 'Cache-Control': 'no-cache', 'Pragma': 'no-cache'}), responseType: 'blob'};
    }
    else
    {
      options = {responseType: 'blob'};
    }

    return this.http.post<string>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params), options);
  }

  //---------------------------------------------------------------------------

  public rest_GET<T> (path: string, params: object): Observable<T>
  {
    var header: object;
    var sitem: AuthSessionItem = this.session ? this.session.value : null;

    if (sitem)
    {
      header = {headers: new HttpHeaders ({'Authorization': "Bearer " + sitem.token, 'Cache-Control': 'no-cache', 'Pragma': 'no-cache'})};
    }
    else
    {
      header = {};
    }

    return this.handle_error_session<T> (this.http.get<T>('rest/' + path, header));
  }

  //---------------------------------------------------------------------------

/*
  public json_rpc_active<T> (qbus_module: string, qbus_method: string, params: object): Observable<T>
  {
    return this.session.pipe (mergeMap((data: AuthSessionItem) => {

      if (data)
      {
        return this.json_rpc<T> (qbus_module, qbus_method, params);
      }
      else
      {
        return of();
      }

    })) as Observable<T>;
  }
*/

  //-----------------------------------------------------------------------------

  private padding (str: string, max: number): string
  {
  	return str.length < max ? this.padding ("0" + str, max) : str;
  }

  //-----------------------------------------------------------------------------

  private crypt4 (user: string, pass: string, wpid: string): string
  {
    var iv: string = this.padding ((new Date).getTime().toString(), 16);

    var hash1: string = CryptoJS.SHA256 (user + ":" + pass).toString();
    var hash2: string = CryptoJS.SHA256 (iv + ":" + hash1).toString();

    if (wpid)
    {
      // create a Object object as text
      return JSON.stringify ({"ha":iv, "id":CryptoJS.SHA256 (user).toString(), "da":hash2, "wpid":wpid});
    }
    else
    {
      // create a Object object as text
      return JSON.stringify ({"ha":iv, "id":CryptoJS.SHA256 (user).toString(), "da":hash2});
    }
  }

  //---------------------------------------------------------------------------

  public json_crypt4_rpc<T> (qbus_module: string, qbus_method: string, params: object, response: EventEmitter<AuthSessionItem>): Observable<T>
  {
    var header: object;

    if (this.user && this.pass)
    {
      // use the old crypt4 authentication mechanism
      // to create a session handle in backend
      header = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (this.user, this.pass, this.wpid)})};
    }
    else
    {
      header = {};
    }

    return this.handle_error_login<T> (this.http.post<T>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params), header), response);
  }

  //---------------------------------------------------------------------------

  private fetch_session (response: EventEmitter<AuthSessionItem>)
  {
    this.json_crypt4_rpc ('AUTH', 'session_add', {type: 1}, response).subscribe((data: AuthSessionItem) => {

      if (data)
      {
        // set the user
        data.user = this.user;

        switch (data.state)
        {
          case 1:
          {
            // activate to use the session already
            this.roles.next (data['roles']);

            // to disable the login background
            this.session.next (data);

            this.modal_service.open (AuthFirstuseModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: AuthSessionItem, useValue: data}])}).result.then(() => {

              this.storage_set (data);
              response.emit (data);

            }, () => {

            });

            break;
          }
          case 2:
          {
            this.roles.next (data['roles']);
            this.storage_set (data);

            response.emit (data);

            break;
          }
        }
      }
      else
      {
        response.emit (null);
      }

    }, () => {

      response.emit (null);

    });
  }

  //---------------------------------------------------------------------------

  public show_login ()
  {
    if (this.login_modal)
    {
      this.modal_service.open (AuthSessionLoginModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static"}).result.then(() => {

      }, () => {

      });
    }
  }

  //---------------------------------------------------------------------------

  private show_workspace_selector (error: HttpErrorResponse, response: EventEmitter<AuthSessionItem>)
  {
    this.modal_service.open (AuthWorkspacesModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: HttpErrorResponse, useValue: error}])}).result.then((result) => {

      this.wpid = String(result);
      this.fetch_session (response);

    }, () => {

      this.storage_clear ();

    });
  }

  //---------------------------------------------------------------------------

  private handle_error_login<T> (http_request: Observable<T>, response: EventEmitter<AuthSessionItem>): Observable<T>
  {
    return http_request.pipe (catchError ((error) => {

      if (error.status == 428)
      {
        const headers: HttpHeaders = error.headers;
        const warning = headers.get('warning');

        // the warning returns the error message
        if (warning)
        {
          // split the error message into code and text
          var i = warning.indexOf(',');

          // retrieve code and text
          var text = warning.substring(i + 1).trim();
          var code = Number(warning.substring(0, i));

          if (text == 'vault')
          {
            // this will create a new vault entry in the auth module

          }
          else if (text == 'code_2factor')
          {
            // this will display the selection of the recipients
            // user can enter the code of the 2factor authentication

          }
          else
          {
            this.show_workspace_selector (error, response);
          }

          return new Observable<T>();
        }
        else
        {
          return throwError (error);
        }
      }
      else
      {
        return throwError (error);
      }
    }));
  }

  //---------------------------------------------------------------------------

  private handle_error_session<T> (http_request: Observable<T>): Observable<T>
  {
    return http_request.pipe (catchError ((error) => {

      if (error.status == 401)
      {
        this.disable ();
        this.show_login ();
      }
      else
      {
        return throwError (error);
      }

    }), map (res => {

      this.timer_update ();
      return res;
    }));
  }

  //---------------------------------------------------------------------------

}

export class AuthSessionItem
{
  token: string;
  state: number;

  firstname: string;
  lastname: string;
  workspace: string;

  lt: string;
  vp: number;

  wpid: number;
  gpid: number;

  // will be set internally
  user: string;
}

//=============================================================================

export abstract class AuthSessionLoginWidget
{
  public on_dismiss: any;
}

//=============================================================================

@Directive({
  selector: 'auth-session-component'
})
export class AuthSessionComponentDirective {

  @Output() onClose = new EventEmitter();

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private view: ViewContainerRef, private component_factory_resolver: ComponentFactoryResolver)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit ()
  {
    // clear the current view
    this.view.clear();

    try
    {
      const type = this.auth_session.get_content_type ();
      if (type)
      {
        // create a factory for creating new components
        const factory = this.component_factory_resolver.resolveComponentFactory (type);

        // use the factory to create the new component
        const compontent = this.view.createComponent(factory);

        // cast to the widget interface
        let widget: AuthSessionLoginWidget = compontent.instance;

        // set the callback
        widget.on_dismiss = () => this.onClose.emit();
      }
      else
      {
        console.log('no component is set');
      }
    }
    catch (e)
    {
      console.log(e);
    }
  }
}

//=============================================================================

@Component({
  selector: 'auth-wpace-modal-component',
  templateUrl: './modal_workspaces.html'
}) export class AuthWorkspacesModalComponent {

  workspaces: any;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private response: HttpErrorResponse)
  {
    this.workspaces = response.error;
  }

  //---------------------------------------------------------------------------

  select_workspace (wpid: number)
  {
    this.modal.close (wpid);
  }
}

//=============================================================================

@Component({
  selector: 'auth-firstuse-modal-component',
  templateUrl: './modal_firstuse.html'
}) export class AuthFirstuseModalComponent {

  public mode: number = 0;
  public user: string;
  public mode_next: boolean = false;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, sitem: AuthSessionItem)
  {
    this.user = sitem.user;
  }

  //---------------------------------------------------------------------------

  on_password_changed()
  {
    this.mode_next = true;
  }
}

//=============================================================================

@Component({
  selector: 'auth-login-modal-component',
  templateUrl: './modal_login.html'
}) export class AuthSessionLoginModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private auth_session: AuthSession)
  {
    auth_session.session.subscribe (() => {



    });
  }

}

//=============================================================================

@Component({
  selector: 'auth-login-component',
  templateUrl: './component_login.html'
}) export class AuthSessionLoginComponent implements AuthSessionLoginWidget {

  public user: string;
  public pass: string;

  public sitem: AuthSessionItem = null;
  public err: boolean = false;

  // this will be set from the auth component directive
  public on_dismiss: any;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession)
  {
  }

  //---------------------------------------------------------------------------

  login_submit ()
  {
    this.auth_session.enable (this.user, this.pass).subscribe ((sitem: AuthSessionItem) => {

      if (sitem)
      {
        this.sitem = sitem;
      }
      else
      {
        this.err = true;
      }

    });

    this.sitem = null;
    this.err = false;
    this.user = '';
    this.pass = '';
  }

  //---------------------------------------------------------------------------

  login_again ()
  {
    this.err = false;
  }

  //---------------------------------------------------------------------------

  close ()
  {
    this.on_dismiss ();
  }

}

//=============================================================================
