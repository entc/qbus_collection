import { Injectable } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders, HttpResponse, HttpEvent, HttpEventType } from '@angular/common/http';
import { Observable, Subscriber, throwError } from 'rxjs';
import { catchError } from 'rxjs/operators';
import { AuthLoginItem, AuthSessionItem, AuthLoginCreds } from '@qbus/auth_session';
import { QbngErrorHolder } from '@qbus/qbng_modals/header';
import * as CryptoJS from 'crypto-js';

//---------------------------------------------------------------------------

@Injectable() export class ConnService
{
  constructor (private http: HttpClient)
  {
  }

  //---------------------------------------------------------------------------

  private padding (str: string, max: number): string
  {
  	return str.length < max ? this.padding ("0" + str, max) : str;
  }

  //---------------------------------------------------------------------------

  private crypt4 (creds: AuthLoginCreds): string
  {
    // get the linux time since 1970 in milliseconds
    var iv: string = this.padding ((new Date).getTime().toString(), 16);

    var hash1: string = CryptoJS.SHA256 (creds.user + ":" + creds.pass).toString();
    var hash2: string = CryptoJS.SHA256 (iv + ":" + hash1).toString();

    // default parameters
    var params = {"ha": iv, "id": CryptoJS.SHA256 (creds.user).toString(), "da": hash2};

    if (creds.wpid)
    {
      params['wpid'] = creds.wpid;
    }

    if (creds.vault)
    {
      // ecrypt password with hash1
      // hash1 is known to the auth module
      params['vault'] = CryptoJS.AES.encrypt (creds.pass, hash1, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }).toString();
    }

    if (creds.code)
    {
      params['code'] = CryptoJS.SHA256 (creds.code).toString();
    }

    // create a Object object as text
    return JSON.stringify (params);
  }

  //---------------------------------------------------------------------------

  private login__handle_error<T> (http_request: Observable<T>, subscriber: Subscriber<AuthLoginItem>, creds: AuthLoginCreds): Observable<T>
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
            if (creds.vault == false)
            {
              creds.vault = true;
              this.login (subscriber, creds);
            }
            else
            {
              creds.vault = false;
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

  public login (subscriber: Subscriber<AuthLoginItem>, creds: AuthLoginCreds)
  {
    var header: object;

    if (creds.user && creds.pass)
    {
      // use the old crypt4 authentication mechanism
      // to create a session handle in backend
      header = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (creds)}), observe: 'events', reportProgress: true};
    }
    else
    {
      header = {observe: 'events', reportProgress: true};
    }

    this.login__handle_error (this.http.post('json/AUTH/session_add', JSON.stringify ({type: 1, info: creds.browser_info}), header), subscriber, creds).subscribe ((event: HttpEvent<AuthSessionItem>) => {

      switch (event.type)
      {
        case HttpEventType.Response:  // final event
        {
          subscriber.next (new AuthLoginItem (0, event.body));
          break;
        }
      }

    }, (error) => subscriber.next (new AuthLoginItem (0, null)));
  }

  //---------------------------------------------------------------------------

  private construct_header (sitem: AuthSessionItem): HttpHeaders
  {
    // get the linux time since 1970 in milliseconds
    var iv: string = this.padding ((new Date).getTime().toString(), 16);
    var da: string = CryptoJS.SHA256 (iv + ":" + sitem.vsec).toString();

    var bearer: string = btoa(JSON.stringify ({token: sitem.token, ha: iv, da: da}));

    return new HttpHeaders ({'Authorization': "Bearer " + bearer, 'Cache-Control': 'no-cache', 'Pragma': 'no-cache'});
  }

  //---------------------------------------------------------------------------

  private construct_params (sitem: AuthSessionItem, params: object): string
  {
    var h = JSON.stringify (params);

    return CryptoJS.AES.encrypt (h, sitem.vsec, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }).toString();
  }

  //---------------------------------------------------------------------------

  private construct_enjs (sitem: AuthSessionItem, stoken: string, qbus_module: string, qbus_method: string, qbus_params: object): AuthEnjs
  {
    if (sitem)
    {
      var enjs: AuthEnjs = new AuthEnjs;

      enjs.url = 'enjs/' + qbus_module + '/' + qbus_method;
      enjs.header = this.construct_header (sitem);
      enjs.params = this.construct_params (sitem, qbus_params);
      enjs.vsec = sitem.vsec;

      return enjs;
    }
    else if (stoken)
    {
      var enjs: AuthEnjs = new AuthEnjs;

      enjs.url = 'json/' + qbus_module + '/' + qbus_method + '/__P/' + stoken;
      enjs.header = new HttpHeaders ({'Cache-Control': 'no-cache', 'Pragma': 'no-cache'});
      enjs.params = JSON.stringify (qbus_params);
      enjs.vsec = null;

      return enjs;
    }
    else
    {
      enjs.url = 'json/' + qbus_module + '/' + qbus_method;
      enjs.header = new HttpHeaders ({'Cache-Control': 'no-cache', 'Pragma': 'no-cache'});
      enjs.params = JSON.stringify (qbus_params);
      enjs.vsec = null;

      return enjs;
    }
  }

  //---------------------------------------------------------------------------

  public json_rpc<T> (subscriber: Subscriber<T>, qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string)
  {
    var enjs: AuthEnjs = this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_params);
  //  if (enjs)
    {
      let obj = this.handle_error_session (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).subscribe ((event: HttpEvent<string>) => {

        switch (event.type)
        {
          case HttpEventType.Response:  // final event
          {
            if (event.body)
            {
              if (enjs.vsec)
              {
                subscriber.next (JSON.parse(CryptoJS.enc.Utf8.stringify (CryptoJS.AES.decrypt (event.body, enjs.vsec, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }))) as T);
              }
              else
              {
                subscriber.next (JSON.parse(event.body) as T);
              }
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
    /*
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
    */
  }

}

//---------------------------------------------------------------------------

class AuthEnjs
{
  url: string;
  params: string;
  header: HttpHeaders;
  vsec: string;
};

//---------------------------------------------------------------------------
