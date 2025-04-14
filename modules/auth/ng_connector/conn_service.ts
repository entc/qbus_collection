import { Injectable } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders, HttpResponse, HttpEvent, HttpEventType } from '@angular/common/http';
import { Observable, Subscriber, throwError } from 'rxjs';
import { catchError } from 'rxjs/operators';
import { AuthLoginItem, AuthUploadItem, AuthSessionItem, AuthLoginCreds } from '@qbus/auth_session';
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

  private crypt4_promise (creds: AuthLoginCreds): Promise<ArrayBuffer>
  {
    // get the linux time since 1970 in milliseconds
    var iv: string = this.padding ((new Date).getTime().toString(), 16);

    const hash1_array = new TextEncoder().encode (creds.user + ":" + creds.pass);

    return window.crypto.subtle.digest ("SHA-256", hash1_array).then ((hash1_buffer: ArrayBuffer) => {

      const hash1: string = new TextDecoder().decode (hash1_buffer);
      const hash2_array = new TextEncoder().encode (iv + ":" + hash1);

      return window.crypto.subtle.digest ("SHA-256", hash2_array);

    });
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
      // use the new crypto interface
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
              this.session__login_request (subscriber, creds);
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

  private session__login_request (subscriber: Subscriber<AuthLoginItem>, creds: AuthLoginCreds): void
  {
    var header: object;

    if (creds.user && creds.pass)
    {
      /*
      this.crypt4_promise (creds).then ((hash2_buffer: ArrayBuffer) => {

        const hash2: string = new TextDecoder().decode (hash2_buffer);

        console.log('hash2 #2 = ' + hash2);

      });
      */

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

  public session__login (creds: AuthLoginCreds): Observable<AuthLoginItem>
  {
    return new Observable ((subscriber) => this.session__login_request (subscriber, creds));
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
      var enjs: AuthEnjs = new AuthEnjs;

      enjs.url = 'json/' + qbus_module + '/' + qbus_method;
      enjs.header = new HttpHeaders ({'Cache-Control': 'no-cache', 'Pragma': 'no-cache'});
      enjs.params = JSON.stringify (qbus_params);
      enjs.vsec = null;

      return enjs;
    }
  }

  //---------------------------------------------------------------------------

  private session__convert_error<T> (http_request: Observable<T>): Observable<T>
  {
    return http_request.pipe (catchError ((error) => {

      const headers: HttpHeaders = error.headers;
      const warning = headers.get('warning');

      if (warning)
      {
        var i = warning.indexOf(',');
        return throwError (new QbngErrorHolder (Number(warning.substring (0, i)), warning.substring (i + 1).trim()));
      }
      else
      {
        return throwError (new QbngErrorHolder (0, 'ERR.UNKNOWN'));
      }

    }));
  }

  //---------------------------------------------------------------------------

  public session__json_rpc<T> (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<T>
  {
    return new Observable<T>((subscriber) => {

      var enjs: AuthEnjs = this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_params);
      let obj = this.session__convert_error (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).subscribe ((event: HttpEvent<string>) => {

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

    });
  }

  //---------------------------------------------------------------------------

  public session__json_rpc_blob (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<Blob>
  {
    return new Observable((subscriber) => {

      var enjs: AuthEnjs = this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_params);
      let obj = this.session__convert_error (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'blob'})).subscribe ((data: Blob) => subscriber.next (data));

    });
  }

  //---------------------------------------------------------------------------

  public session__json_rpc_resp (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<HttpResponse<Blob>>
  {
    return new Observable((subscriber) => {

      var enjs: AuthEnjs = this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_params);
      let obj = this.session__convert_error (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'blob', observe: 'response'})).subscribe ((response: HttpResponse<Blob>) => subscriber.next (response));

    });
  }

  //---------------------------------------------------------------------------

  public session__json_rpc_upload (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<AuthUploadItem>
  {
    return new Observable ((subscriber) => {

      var enjs: AuthEnjs = this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_params);
      let obj = this.session__convert_error (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).subscribe ((event: HttpEvent<string>) => {

        switch (event.type)
        {
          case HttpEventType.UploadProgress:  // update
          {
            subscriber.next (new AuthUploadItem (0, Math.round(100 * (event.loaded / event.total))));
            break;
          }
          case HttpEventType.Response:  // final event
          {
            if (event.body)
            {
              subscriber.next (new AuthUploadItem (1, 0, JSON.parse(CryptoJS.enc.Utf8.stringify (CryptoJS.AES.decrypt (event.body, enjs.vsec, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 })))));
            }
            else
            {
              subscriber.next (new AuthUploadItem (1, 0));
            }

            subscriber.complete();
            break;
          }
        }
      }, (err: QbngErrorHolder) => subscriber.error (err));

    });
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
