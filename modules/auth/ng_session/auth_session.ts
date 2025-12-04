import { Component, Injectable, Directive, TemplateRef, OnInit, Input, Output, Injector, ElementRef, ViewContainerRef, EventEmitter, Type, ComponentFactoryResolver } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders, HttpResponse, HttpEvent, HttpEventType } from '@angular/common/http';
import { CanActivate, RouterStateSnapshot, UrlTree, ActivatedRouteSnapshot } from '@angular/router';
import { Observable, Subscriber, BehaviorSubject, Subject } from 'rxjs';
import { catchError, retry, map, takeWhile, tap, mergeMap } from 'rxjs/operators'
import { throwError, of, timer } from 'rxjs';
import { interval } from 'rxjs/internal/observable/interval';
import * as CryptoJS from 'crypto-js';
import { QbngErrorHolder } from '@qbus/qbng_modals/header';
import { ConnService } from '@conn/conn_service';

//-----------------------------------------------------------------------------

const SESSION_STORAGE_TOKEN        = 'session_token';
const SESSION_STORAGE_FIRSTNAME    = 'session_fn';
const SESSION_STORAGE_LASTNAME     = 'session_ln';
const SESSION_STORAGE_WORKSPACE    = 'session_wp';
const SESSION_STORAGE_LT           = 'session_lt';
const SESSION_STORAGE_VP           = 'session_vp';
const SESSION_STORAGE_WPID         = 'session_wpid';
const SESSION_STORAGE_GPID         = 'session_gpid';
const SESSION_STORAGE_VSEC         = 'session_vsec';

//-----------------------------------------------------------------------------

@Injectable()
export class AuthSession
{
  private session_key: string;
  private session_token: string = null;
  private interval_obj;
  private timer_idle_countdown;

  private user: string = null;
  private pass: string = null;
  private wpid: string = null;
  private vault: boolean = false;    // special option to summit vault password

  // initialize the session emitter
  public session: BehaviorSubject<AuthSessionItem>;
  public roles: BehaviorSubject<object>;
  public idle: EventEmitter<number> = new EventEmitter();

  private conn_subscriber;

  //-----------------------------------------------------------------------------

  constructor (private http: HttpClient, private conn: ConnService)
  {
    this.roles = new BehaviorSubject(null);
    this.session = new BehaviorSubject(null);

    // get the last update timestamp
    this.wpid = sessionStorage.getItem (SESSION_STORAGE_WPID);
    if (this.wpid)
    {
      this.conn_subscriber = this.conn.status.subscribe ((status: ConnStatus) => {

        if (status.connected)
        {
          this.session.next ({
                vsec: sessionStorage.getItem (SESSION_STORAGE_VSEC),
                token: sessionStorage.getItem (SESSION_STORAGE_TOKEN),
                firstname: sessionStorage.getItem (SESSION_STORAGE_FIRSTNAME),
                lastname: sessionStorage.getItem (SESSION_STORAGE_LASTNAME),
                workspace: sessionStorage.getItem (SESSION_STORAGE_WORKSPACE),
                lt: sessionStorage.getItem (SESSION_STORAGE_LT),
                vp: Number(sessionStorage.getItem (SESSION_STORAGE_VP)),
                wpid: Number(sessionStorage.getItem (SESSION_STORAGE_WPID)),
                gpid: Number(sessionStorage.getItem (SESSION_STORAGE_GPID)),
                state: 0,
                user: null,
                remote: ''
          });
          this.json_rpc ('AUTH', 'session_roles', {}).subscribe ((data: object) => this.roles.next (data));
        }
        else
        {
          this.roles.next (null);
          this.session.next (null);
        }

      });

      this.timer_set (Number(sessionStorage.getItem (SESSION_STORAGE_VP)));
    }
    else
    {
      this.session = new BehaviorSubject(null);
    }
  }

  //---------------------------------------------------------------------------

  public enable (user: string, pass: string, code: string = null, wpid: number = 0): Observable<AuthLoginItem>
  {
    const navigator = window.navigator;
    const browser_info = {userAgent: navigator.userAgent, vendor: navigator.vendor, geolocation: navigator.geolocation, platform: navigator.platform};

    // set the credentials
    this.user = user;
    this.pass = pass;

    let creds: AuthLoginCreds = new AuthLoginCreds (wpid, user, pass, this.vault, code, browser_info, this.gen_vsec (user, pass));

    return this.conn.session__login (creds).pipe (map ((slogin: AuthLoginItem) => {

      let sitem: AuthSessionItem = slogin.sitem;
      if (sitem)
      {
        // set the user
        sitem.user = user;

        this.roles.next (sitem['roles']);

        console.log('login done, set storage');

        this.storage_set (sitem);
      }

      return slogin;

    }));
  }

  //---------------------------------------------------------------------------

  public disable (session_expired: boolean = false): void
  {
    this.conn.session__logout (session_expired);

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

  //-----------------------------------------------------------------------------

  public set_token (token: string): void
  {
    this.session_token = token;
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

        if (this.timer_idle_countdown == 0) this.disable (true);

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
    if (s)
    {
      this.timer_idle_countdown = s.vp;
    }
  }

  //---------------------------------------------------------------------------

  private gen_vsec (user: string, pass: string): string
  {
    return CryptoJS.SHA256 (this.user + ":" + this.pass).toString();
  }

  //---------------------------------------------------------------------------

  private storage_set (sitem: AuthSessionItem): void
  {
    // encode the vsec
    sitem.vsec = this.gen_vsec (this.user, this.pass);
    this.session.next (sitem);

    sessionStorage.setItem (SESSION_STORAGE_VSEC, sitem.vsec);
    sessionStorage.setItem (SESSION_STORAGE_TOKEN, sitem.token);
    sessionStorage.setItem (SESSION_STORAGE_FIRSTNAME, sitem.firstname);
    sessionStorage.setItem (SESSION_STORAGE_LASTNAME, sitem.lastname);
    sessionStorage.setItem (SESSION_STORAGE_WORKSPACE, sitem.workspace);
    sessionStorage.setItem (SESSION_STORAGE_LT, sitem.lt);
    sessionStorage.setItem (SESSION_STORAGE_VP, String(sitem.vp));
    sessionStorage.setItem (SESSION_STORAGE_WPID, String(sitem.wpid));
    sessionStorage.setItem (SESSION_STORAGE_GPID, String(sitem.gpid));

    this.timer_set (sitem.vp);
  }

  //---------------------------------------------------------------------------

  private storage_clear (): void
  {
    this.timer_clear ();

    this.user = null;
    this.pass = null;
    this.wpid = null;

    sessionStorage.removeItem (SESSION_STORAGE_VSEC);
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

  private session_url (qbus_module: string, qbus_method: string): string
  {
    if (this.session_token)
    {
      return 'json/' + qbus_module + '/' + qbus_method + '/__P/' + this.session_token;
    }
    else
    {
      return 'json/' + qbus_module + '/' + qbus_method;
    }
  }

  //---------------------------------------------------------------------------

  private session_get_token (): AuthSessionItem
  {
    if (this.session_token)
    {
      return null;
    }

    if (this.session)
    {
      return this.session.value;
    }

    return null;
  }

  //---------------------------------------------------------------------------

  private construct_bearer (sitem: AuthSessionItem): string
  {
    // get the linux time since 1970 in milliseconds
    var iv: string = this.padding ((new Date).getTime().toString(), 16);
    var da: string = CryptoJS.SHA256 (iv + ":" + sitem.vsec).toString();

    return btoa(JSON.stringify ({token: sitem.token, ha: iv, da: da}));
  }

  //---------------------------------------------------------------------------

  private construct_header (bearer: string): HttpHeaders
  {
    return new HttpHeaders ({'Authorization': "Bearer " + bearer, 'Cache-Control': 'no-cache', 'Pragma': 'no-cache'});
  }

  //---------------------------------------------------------------------------

  private session_get_bearer (): string
  {
    var sitem: AuthSessionItem = this.session_get_token ();

    if (sitem)
    {
      return this.construct_bearer (sitem);
    }
    else
    {
      return null;
    }
  }

  //---------------------------------------------------------------------------

  private session_options (): object
  {
    var options: object;
    var bearer: string = this.session_get_bearer ();

    if (bearer)
    {
      options = {headers: this.construct_header (bearer)};
    }
    else
    {
      options = {};
    }

    return options;
  }

  //---------------------------------------------------------------------------

  public json_rpc_upload (qbus_module: string, qbus_method: string, qbus_params: object, cb_progress, cb_done, cb_error)
  {
    // TODO: use the direct approach instead
    this.conn.session__json_rpc_upload (qbus_module, qbus_method, qbus_params, this.session_get_token (), this.session_token).subscribe ((uitem: AuthUploadItem) => {

      switch (uitem.state)
      {
        case 0:
        {
          cb_progress (uitem.progress);
          break;
        }
        case 1:
        {
          cb_done (uitem.data);
          break;
        }
      }

    }, (err: QbngErrorHolder) => cb_error (err));
  }

  //---------------------------------------------------------------------------

  public json_rpc<T> (qbus_module: string, qbus_method: string, qbus_params: object): Observable<T>
  {
    return this.conn.session__json_rpc<T> (qbus_module, qbus_method, qbus_params, this.session_get_token (), this.session_token).pipe(map((res: T) => {

      this.timer_update ();
      return res;

    }), catchError ((err: QbngErrorHolder) => {

      if (err.code == 11)   // no authentication
      {
        this.disable();
      }

      return throwError (err);
    }));
  }

  //---------------------------------------------------------------------------

  public blob_rpc_resp (qbus_module: string, qbus_method: string, qbus_params: object): Observable<HttpResponse<Blob>>
  {
    return this.conn.session__json_rpc_resp (qbus_module, qbus_method, qbus_params, this.session_get_token (), this.session_token).pipe(map((res: HttpResponse<Blob>) => {

      this.timer_update ();
      return res;

    }), catchError ((err: QbngErrorHolder) => {

      if (err.code == 11)   // no authentication
      {
        this.disable();
      }

      return throwError (err);
    }));
  }

  //---------------------------------------------------------------------------

  public json_rpc_blob (qbus_module: string, qbus_method: string, qbus_params: object): Observable<Blob>
  {
    return this.conn.session__json_rpc_blob (qbus_module, qbus_method, qbus_params, this.session_get_token (), this.session_token).pipe(map((res: Blob) => {

      this.timer_update ();
      return res;

    }), catchError ((err: QbngErrorHolder) => {

      if (err.code == 11)   // no authentication
      {
        this.disable();
      }

      return throwError (err);
    }));
  }

  //---------------------------------------------------------------------------

  public rest_GET<T> (path: string, params: object): Observable<T>
  {
    return this.handle_error_session<T> (this.http.get<T>('rest/' + path, this.session_options()));
  }

  //---------------------------------------------------------------------------

  public rest_POST<T> (path: string, params: object): Observable<T>
  {
    return this.handle_error_session<T> (this.http.post<T>('rest/' + path, JSON.stringify (params), this.session_options()));
  }

  //---------------------------------------------------------------------------

  public rest_PUT<T> (path: string, params: object): Observable<T>
  {
    return this.handle_error_session<T> (this.http.put<T>('rest/' + path, JSON.stringify (params), this.session_options()));
  }

  //---------------------------------------------------------------------------

  public rest_PATCH<T> (path: string, params: object): Observable<T>
  {
    return this.handle_error_session<T> (this.http.patch<T>('rest/' + path, JSON.stringify (params), this.session_options()));
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

  //---------------------------------------------------------------------------

  public json_token_rpc<T> (token: string, qbus_module: string, qbus_method: string, params: object): Observable<T>
  {
    return this.http.post<T>('json/' + qbus_module + '/' + qbus_method + '/__P/' + token, JSON.stringify (params));
  }

  //---------------------------------------------------------------------------

  public json_none_rpc<T> (qbus_module: string, qbus_method: string, params: object): Observable<T>
  {
    return this.http.post<T>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params));
  }

  //-----------------------------------------------------------------------------


  private padding (str: string, max: number): string
  {
  	return str.length < max ? this.padding ("0" + str, max) : str;
  }

  //---------------------------------------------------------------------------

  private handle_http_headers (headers: HttpHeaders)
  {
    const warning = headers.get('warning');

    if (warning)
    {
      var i = warning.indexOf(',');
      return throwError (new QbngErrorHolder (Number(warning.substring(0, i)), warning.substring(i + 1).trim()));
    }
    else
    {
      return throwError (new QbngErrorHolder (0, 'ERR.UNKNOWN'));
    }
  }

  //---------------------------------------------------------------------------

  private handle_error_session<T> (http_request: Observable<T>): Observable<T>
  {
    return http_request.pipe (catchError ((error) => {

      if (error.status == 401)
      {
        var h = this.handle_http_headers (error.headers);

        this.disable ();

        return h;
      }
      else
      {
        return this.handle_http_headers (error.headers);
      }

    }), map (res => {

      this.timer_update ();
      return res;
    }));
  }

  //---------------------------------------------------------------------------

}

class AuthEnjs
{
  url: string;
  params: string;
  header: HttpHeaders;
  vsec: string;
};

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
  vsec: string;

  remote: string;
}

export class AuthRecipient
{
  id: number;
  email: string;
  type: number;

  // fill be used from the component
  used: boolean;
}

export class AuthWorkspace
{
  workspace: string;
  wpid: number;
}

export class AuthLoginItem
{
  constructor (public state: number, public sitem: AuthSessionItem, public list: AuthRecipient[] | AuthWorkspace[] = null, public token: string = null) {}
}

export class AuthUploadItem
{
  constructor (public state: number, public progress: number, public data: object = null) {}
}

export class AuthLoginCreds
{
  constructor (public wpid: number, public user: string, public pass: string, public vault: boolean, public code: string, public browser_info: object, public vsec: string) {}
}

export class AuthWpInfo
{
  domain: string;
  active: number;
  name: string;
}

export class AuthUserContext
{
  wpid: number;
  gpid: number;
  userid: number;
  active: boolean;
  info: AuthWpInfo;
}

export interface ConnStatus
{
  url: string;
  state: number;
  connected: boolean;
}

//=============================================================================

export abstract class AuthSessionLoginWidget
{
  public on_dismiss: any;
}

//=============================================================================

/*

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
*/
/*
class AuthRecipients
{
  id: number;
  email: string;
  type: number;

  // fill be used from the component
  used: boolean;
}

//=============================================================================
*/
/*
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

  public on_password_changed()
  {
    this.mode_next = true;
  }
}
*/
//=============================================================================

/*
@Component({
  selector: 'auth-login-modal-component',
  templateUrl: './modal_login.html'
}) export class AuthLoginModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private auth_session: AuthSession)
  {
    auth_session.session.subscribe (() => {



    });
  }
}
*/

//-----------------------------------------------------------------------------

@Directive({
  selector: '[authSessionRole]'
})
export class AuthSessionRoleDirective {

  private permissions: Array<string>;
  private roles: object;
  private enabled: boolean = false;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private element: ElementRef, private templateRef: TemplateRef<any>, private viewContainer: ViewContainerRef)
  {
    this.enabled = false;
    this.roles = null;

    this.auth_session.roles.subscribe ((roles: object) => {

      this.roles = roles;
      this.updateView ();
    });
  }

  //---------------------------------------------------------------------------

  @Input () set authSessionRole (val: Array<string>)
  {
    this.permissions = val;
    this.enabled = true;
    this.updateView ();
  }

  //---------------------------------------------------------------------------

  private updateView ()
  {
    if (this.enabled && this.roles)
    {
      if (this.permissions)
      {
        if (this.auth_session.contains_role__or (this.roles, this.permissions))
        {
          this.viewContainer.clear();
          this.viewContainer.createEmbeddedView(this.templateRef);
        }
        else
        {
          this.viewContainer.clear();
        }
      }
      else
      {
        this.viewContainer.clear();
        this.viewContainer.createEmbeddedView(this.templateRef);
      }
    }
    else
    {
      this.viewContainer.clear();
    }
  }
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
