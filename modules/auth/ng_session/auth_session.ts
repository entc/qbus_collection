import { Component, Injectable, Directive, TemplateRef, OnInit, Input, Output, Injector, ElementRef, ViewContainerRef, EventEmitter, Type, ComponentFactoryResolver } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders, HttpResponse, HttpEvent, HttpEventType } from '@angular/common/http';
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

  //-----------------------------------------------------------------------------

  constructor (private http: HttpClient, private conn: ConnService)
  {
    this.roles = new BehaviorSubject(null);

    // get the last update timestamp
    this.wpid = sessionStorage.getItem (SESSION_STORAGE_WPID);
    if (this.wpid)
    {
      this.session = new BehaviorSubject({
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
      this.timer_set (Number(sessionStorage.getItem (SESSION_STORAGE_VP)));
    }
    else
    {
      this.session = new BehaviorSubject(null);
    }
  }

  //---------------------------------------------------------------------------

  private enable__handle_error<T> (http_request: Observable<T>, subscriber: Subscriber<AuthLoginItem>): Observable<T>
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
            if (this.vault == false)
            {
              this.vault = true;
              this.enable__fetch (subscriber, null);
            }
            else
            {
              this.vault = false;
              return throwError (error);
            }
          }
          else if (text == '2f_code')
          {
            subscriber.next (new AuthLoginItem (2, null, error.error['recipients'], error.error['token']));
          }
          else
          {
            subscriber.next (new AuthLoginItem (1, null));
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

  private enable__fetch (subscriber: Subscriber<AuthLoginItem>, code: string = null)
  {
    const navigator = window.navigator;
    const browser_info = {userAgent: navigator.userAgent, vendor: navigator.vendor, geolocation: navigator.geolocation, platform: navigator.platform};

    var header: object;

    if (this.user && this.pass)
    {
      // use the old crypt4 authentication mechanism
      // to create a session handle in backend
      header = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (this.user, this.pass, code)}), observe: 'events', reportProgress: true};
    }
    else
    {
      header = {observe: 'events', reportProgress: true};
    }

    this.enable__handle_error (this.http.post('json/AUTH/session_add', JSON.stringify ({type: 1, info: browser_info}), header), subscriber).subscribe ((event: HttpEvent<AuthSessionItem>) => {

      switch (event.type)
      {
        case HttpEventType.Response:  // final event
        {
          console.log('final response');

          let data: AuthSessionItem = event.body;

          if (data)
          {
            // set the user
            data.user = this.user;

            this.roles.next (data['roles']);
            this.storage_set (data);

            subscriber.next (new AuthLoginItem (0, data));
          }
          else
          {
            subscriber.next (new AuthLoginItem (0, null));
          }

          break;
        }
      }

    }, (error) => subscriber.next (new AuthLoginItem (0, null)));
  }

  //---------------------------------------------------------------------------

  public enable (user: string, pass: string, code: string = null): Observable<AuthLoginItem>
  {
    // set the credentials
    this.user = user;
    this.pass = pass;

    return new Observable ((subscriber) => this.enable__fetch (subscriber, code));
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
    if (s)
    {
      this.timer_idle_countdown = s.vp;
    }
  }

  //---------------------------------------------------------------------------

  private storage_set (sitem: AuthSessionItem): void
  {
    // encode the vsec
    sitem.vsec = CryptoJS.SHA256 (this.user + ":" + this.pass).toString();

    sessionStorage.setItem (SESSION_STORAGE_VSEC, sitem.vsec);
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

  private construct_params (sitem: AuthSessionItem, params: object)
  {
    var h = JSON.stringify (params);

    return CryptoJS.AES.encrypt (h, sitem.vsec, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }).toString();
  }

  //---------------------------------------------------------------------------

  private construct_enjs (qbus_module: string, qbus_method: string, qbus_params: object): AuthEnjs
  {
    var sitem: AuthSessionItem = this.session_get_token ();
    if (sitem)
    {
      // construct other values
      var bearer: string = this.construct_bearer (sitem);
      var enjs: AuthEnjs = new AuthEnjs;

      enjs.url = 'enjs/' + qbus_module + '/' + qbus_method;
      enjs.header = this.construct_header (bearer);
      enjs.params = this.construct_params (sitem, qbus_params);
      enjs.vsec = sitem.vsec;

      return enjs;
    }
    else
    {
      return null;
    }
  }

  //---------------------------------------------------------------------------

  public json_rpc_upload (qbus_module: string, qbus_method: string, qbus_params: object, cb_progress, cb_done, cb_error)
  {
    var enjs: AuthEnjs = this.construct_enjs (qbus_module, qbus_method, qbus_params);
    if (enjs)
    {
      let subscriber = this.handle_error_session (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).subscribe ((event: HttpEvent<string>) => {

        switch (event.type)
        {
          case HttpEventType.UploadProgress:  // update
          {
            cb_progress (Math.round(100 * (event.loaded / event.total)));
            break;
          }
          case HttpEventType.Response:  // final event
          {
            if (event.body)
            {
              cb_done (JSON.parse(CryptoJS.enc.Utf8.stringify (CryptoJS.AES.decrypt (event.body, enjs.vsec, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }))));
            }
            else
            {
              subscriber.unsubscribe();
              cb_done ({});
            }

            break;
          }
        }

      }, (err: QbngErrorHolder) => cb_error (err));
    }
    else
    {
      // construct url
      var url: string = this.session_url (qbus_module, qbus_method);

      // construct other values
      var params: string = JSON.stringify (qbus_params);

      this.handle_error_session (this.http.post(url, params, {observe: 'events', reportProgress: true})).subscribe ((event) => {

alert ('we run out of coffee, so it is not implemented');

      }, (err: QbngErrorHolder) => cb_error (err));
    }
  }

  //---------------------------------------------------------------------------

  public json_rpc<T> (qbus_module: string, qbus_method: string, qbus_params: object): Observable<T>
  {
    return new Observable((subscriber) => {

      var enjs: AuthEnjs = this.construct_enjs (qbus_module, qbus_method, qbus_params);
      if (enjs)
      {
        let obj = this.handle_error_session (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).subscribe ((event: HttpEvent<string>) => {

          switch (event.type)
          {
            case HttpEventType.Response:  // final event
            {
              if (event.body)
              {
                subscriber.next (JSON.parse(CryptoJS.enc.Utf8.stringify (CryptoJS.AES.decrypt (event.body, enjs.vsec, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }))) as T);
              }
              else
              {
                subscriber.next ({} as T);
              }

              subscriber.complete();
              obj.unsubscribe();
              break;
            }
          }
        }, (err: QbngErrorHolder) => subscriber.error (err));
      }
      else
      {
        let obj = this.handle_error_session (this.http.post(this.session_url (qbus_module, qbus_method), JSON.stringify (qbus_params), {responseType: 'text', observe: 'events', reportProgress: true})).subscribe ((event: HttpEvent<string>) => {

          switch (event.type)
          {
            case HttpEventType.Response:  // final event
            {
              if (event.body)
              {
                subscriber.next (JSON.parse(event.body) as T);
              }
              else
              {
                subscriber.next ({} as T);
              }

              subscriber.complete();
              obj.unsubscribe();
              break;
            }
          }
        }, (err: QbngErrorHolder) => subscriber.error (err));
      }

    });
  }

  //---------------------------------------------------------------------------

  public blob_rpc_resp (qbus_module: string, qbus_method: string, qbus_params: object): Observable<HttpResponse<Blob>>
  {
    var enjs: AuthEnjs = this.construct_enjs (qbus_module, qbus_method, qbus_params);
    if (enjs)
    {
      var req = this.handle_error_session (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'blob', observe: 'response'}));

      // decrypt the content
      return req;
    }
    else
    {
      // construct url
      var url: string = this.session_url (qbus_module, qbus_method);

      // construct other values
      var params: string = JSON.stringify (qbus_params);

      return this.handle_error_session (this.http.post(url, params, {responseType: 'blob', observe: 'response'}));
    }
  }

  //---------------------------------------------------------------------------

  public json_rpc_blob (qbus_module: string, qbus_method: string, qbus_params: object): Observable<Blob>
  {
    var enjs: AuthEnjs = this.construct_enjs (qbus_module, qbus_method, qbus_params);
    if (enjs)
    {
      var req = this.handle_error_session<Blob> (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'blob'}));

      // decrypt the content
      return req;
    }
    else
    {
      // construct url
      var url: string = this.session_url (qbus_module, qbus_method);

      // construct other values
      var params: string = JSON.stringify (qbus_params);

      return this.handle_error_session<Blob> (this.http.post(url, params, {responseType: 'blob'}));
    }
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

  //-----------------------------------------------------------------------------

  private crypt4 (user: string, pass: string, code: string): string
  {
    // get the linux time since 1970 in milliseconds
    var iv: string = this.padding ((new Date).getTime().toString(), 16);

    var hash1: string = CryptoJS.SHA256 (user + ":" + pass).toString();
    var hash2: string = CryptoJS.SHA256 (iv + ":" + hash1).toString();

    // default parameters
    var params = {"ha":iv, "id":CryptoJS.SHA256 (user).toString(), "da":hash2};

    if (this.wpid)
    {
      params['wpid'] = this.wpid;
    }

    if (this.vault)
    {
      // ecrypt password with hash1
      // hash1 is known to the auth module
      params['vault'] = CryptoJS.AES.encrypt (pass, hash1, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }).toString();
    }

    if (code)
    {
      params['code'] = CryptoJS.SHA256 (code).toString();
    }

    // create a Object object as text
    return JSON.stringify (params);
  }

  //---------------------------------------------------------------------------

  public json_crypt4_rpc<T> (qbus_module: string, qbus_method: string, params: object, response: EventEmitter<AuthSessionItem>, code: string): Observable<T>
  {
    var header: object;

    if (this.user && this.pass)
    {
      // use the old crypt4 authentication mechanism
      // to create a session handle in backend
      header = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (this.user, this.pass, code)}), observe: 'events', reportProgress: true};
    }
    else
    {
      header = {observe: 'events', reportProgress: true};
    }

    return this.handle_error_login<T> (this.http.post<T>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params), header), response);
  }

  //---------------------------------------------------------------------------

  private fetch_session__handle_event (event: HttpEvent<AuthSessionItem>, response: EventEmitter<AuthSessionItem>)
  {
    switch (event.type)
    {
      case HttpEventType.Response:  // final event
      {
        let data: AuthSessionItem = event.body;

        if (data)
        {
          // set the user
          data.user = this.user;

          this.roles.next (data['roles']);
          this.storage_set (data);

          response.emit (data);
        }
        else
        {
          response.emit (null);
        }

        break;
      }
    }
  }

  //---------------------------------------------------------------------------

  private fetch_session (response: EventEmitter<AuthSessionItem>, code: string)
  {
    const navigator = window.navigator;
    const browser_info = {userAgent: navigator.userAgent, vendor: navigator.vendor, geolocation: navigator.geolocation, platform: navigator.platform};

    let request = this.json_crypt4_rpc ('AUTH', 'session_add', {type: 1, info: browser_info}, response, code).subscribe((event: HttpEvent<AuthSessionItem>) => {

      this.fetch_session__handle_event (event, response);

    }, (err: QbngErrorHolder) => response.emit (null));
  }

  //---------------------------------------------------------------------------

  private show_workspace_selector (error: HttpErrorResponse, response: EventEmitter<AuthSessionItem>)
  {
    /*
    this.modal_service.open (AuthWorkspacesModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: HttpErrorResponse, useValue: error}])}).result.then((result) => {

      this.vault = false;
      this.wpid = String(result);
      this.fetch_session (response, null);

    }, () => {

      this.storage_clear ();

    });
    */
  }

  //---------------------------------------------------------------------------

  private show_2factor_selector (error: HttpErrorResponse, response: EventEmitter<AuthSessionItem>)
  {
    /*
    this.modal_service.open (Auth2FactorModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: HttpErrorResponse, useValue: error}])}).result.then((result) => {

      this.vault = false;
      this.fetch_session (response, result['code']);

    }, () => {

      this.storage_clear ();

    });
    */
  }

  //---------------------------------------------------------------------------

  private apply_vault (response: EventEmitter<AuthSessionItem>)
  {
    this.vault = true;
    this.fetch_session (response, null);
  }

  //---------------------------------------------------------------------------

  private handle_http_headers (headers: HttpHeaders)
  {
    const warning = headers.get('warning');

    if (warning)
    {
      var i = warning.indexOf(',');

      var eh: QbngErrorHolder = new QbngErrorHolder;

      eh.text = warning.substring(i + 1).trim();
      eh.code = Number(warning.substring(0, i));

      return throwError (eh);
    }
    else
    {
      var eh: QbngErrorHolder = new QbngErrorHolder;

      eh.text = 'ERR.UNKNOWN';
      eh.code = 0;

      return throwError (eh);
    }
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
            if (this.vault == false)
            {
              // this will create a new vault entry in the auth module
              this.apply_vault (response);
            }
            else
            {
              this.vault = false;
              return throwError (error);
            }
          }
          else if (text == '2f_code')
          {
            // this will display the selection of the recipients
            // user can enter the code of the 2factor authentication
            this.show_2factor_selector (error, response);
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

export class AuthLoginItem
{
  constructor (public state: number, public sitem: AuthSessionItem, public recipients: AuthRecipient[] = null, public token: string = null) {}
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

//=============================================================================
/*
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
*/
//=============================================================================

/*
@Component({
  selector: 'auth-2factor-modal-component',
  templateUrl: './modal_2factor.html'
}) export class Auth2FactorModalComponent {

  public mode: number = 0;
  public recipients: AuthRecipients[];
  public code: string;

  private token: string;

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, public modal: NgbActiveModal, private response: HttpErrorResponse)
  {
    this.recipients = response.error['recipients'];

    if (this.recipients.length > 0)
    {
      for (var i in this.recipients)
      {
        var item: AuthRecipients = this.recipients[i];
        item.used = false;
      }

      this.recipients[0].used = true;
    }

    this.token = response.error['token'];
  }

  //---------------------------------------------------------------------------

  toogle_used (item: AuthRecipients)
  {
    item.used = !item.used;
  }

  //---------------------------------------------------------------------------

  send ()
  {
    var params = [];

    if (this.recipients.length > 0)
    {
      for (var i in this.recipients)
      {
        var item: AuthRecipients = this.recipients[i];
        if (item.used) params.push (item.id);
      }
    }

    this.auth_session.json_token_rpc (this.token, 'AUTH', 'ui_2f_send', params).subscribe(() => {

      this.mode = 1;
      this.code = '';

    });
  }

  //---------------------------------------------------------------------------

  login ()
  {
    this.modal.close ({code: this.code});
  }
}

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
