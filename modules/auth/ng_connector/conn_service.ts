import { Injectable } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders, HttpResponse, HttpEvent, HttpEventType } from '@angular/common/http';
import { Observable, Subscriber, throwError, from, of } from 'rxjs';
import { switchMap, map, catchError } from 'rxjs/operators';
import { AuthLoginItem, AuthUploadItem, AuthSessionItem, AuthLoginCreds, ConnStatus } from '@qbus/auth_session';
import { QbngErrorHolder } from '@qbus/qbng_modals/header';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import * as CryptoJS from 'crypto-js';

//---------------------------------------------------------------------------

interface Crypt4Params
{
    ha: string;
    id: string;
    da: string;
    wpid?: number;
    code?: string;
    vault?: string;
}

type RpcEvent<T> =
  | { type: 'progress'; value: number }
  | { type: 'result'; value: T };

//---------------------------------------------------------------------------

@Injectable() export class ConnService
{
  // the connection status is always available and connected
  public status: Observable<ConnStatus> = new Observable ((subscriber) => subscriber.next({state: 0, url: null, connected: true}));

  // for decryption
  private readonly decoder = new TextDecoder();
  private readonly encoder = new TextEncoder();

  constructor (private http: HttpClient, private modal_service: NgbModal)
  {
  }

  //---------------------------------------------------------------------------

  private b64_to_bytes (b64: string): Uint8Array
  {
      return Uint8Array.from(atob(b64), c => c.charCodeAt(0));
  }

  //-------------------------------------------------------------------------

  private bytes_to_hex (buffer: ArrayBuffer): string
  {
      return Array.from(new Uint8Array(buffer)).map(b => b.toString(16).padStart(2, '0')).join('');
  }

  //-------------------------------------------------------------------------

  private padding (str: string, max: number): string
  {
  	 return str.length < max ? this.padding ("0" + str, max) : str;
  }

  //---------------------------------------------------------------------------

  private async crypt4_promise (creds: AuthLoginCreds): Promise<string>
  {
      const [hash1_buffer, user_buffer] = await Promise.all([
          crypto.subtle.digest("SHA-256", this.encoder.encode(creds.user + ":" + creds.pass)),
          crypto.subtle.digest("SHA-256", this.encoder.encode(creds.user))
      ]);

      // TODO: use a cached value here
      // convert from ArrayBuffer into hex string
      const hash1 = this.bytes_to_hex(hash1_buffer);
      const user = this.bytes_to_hex(user_buffer);

      // get the linux time since 1970 in milliseconds
      const ha: string = this.padding (Date.now().toString(), 16);

      const hash2 = await crypto.subtle.digest ("SHA-256", this.encoder.encode (ha + ":" + hash1));

      const params: Crypt4Params = {"ha": ha, "id": user, "da": this.bytes_to_hex(hash2)};

      if (creds.wpid)
      {
          params.wpid = creds.wpid;
      }

      if (creds.code)
      {
          const hash3 = await crypto.subtle.digest ("SHA-256", this.encoder.encode (creds.code));

          // use the new crypto interface
          params.code = this.bytes_to_hex(hash3);
      }

      if (creds.vault)
      {
          // TODO: replace this
          // ecrypt password with hash1
          // hash1 is known to the auth module
          params.vault = CryptoJS.AES.encrypt (creds.pass, hash1, { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 }).toString();
      }

      // create a Object object as text
      return JSON.stringify (params);
  }

  //---------------------------------------------------------------------------

/*
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
*/
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
            subscriber.next (new AuthLoginItem (1, null, error.error));
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

  private async derive_key (password: string, salt: Uint8Array, iterations: number): Promise<CryptoKey>
  {
      // imports the password as PBKDF2 input material (not a usable cryptographic key yet)
      const base_key = await crypto.subtle.importKey ("raw", this.encoder.encode(password), "PBKDF2", false, ["deriveKey"]);

      // taking a password-derived key (baseKey) and turning it into a real 256-bit AES-GCM encryption key using PBKDF2
      return crypto.subtle.deriveKey ({name: "PBKDF2", salt, iterations: iterations, hash: "SHA-256"}, base_key, {name: "AES-GCM", length: 256}, false, ["decrypt"]);
  }

  //---------------------------------------------------------------------------

  private async decrypt_v1 (payload: Uint8Array, password: string): Promise<string>
  {
      const salt = payload.slice(1, 17);
      const iv = payload.slice(17, 29);
      const ciphertext = payload.slice(29);

      // derive AES-GCM CryptoKey from password + salt
      const key = await this.derive_key(password, salt, 100000);

      try
      {
          // decrypt
          const plaintext_buffer = await crypto.subtle.decrypt({name: "AES-GCM", iv}, key, ciphertext);

          return this.decoder.decode (plaintext_buffer);
      }
      catch
      {
          throw new Error("Invalid password or corrupted encrypted data");
      }
  }

  //---------------------------------------------------------------------------

  private async decrypt_item (payload_base64: string, password: string): Promise<string>
  {
      const payload = this.b64_to_bytes (payload_base64);

      if (payload.length <= 29)
      {
          throw new Error("Invalid encrypted payload");
      }

      switch (payload[0])
      {
          case 0x67:
          {
              return this.decrypt_v1 (payload, password);
          }
          default:
          {
              throw new Error(`Unsupported encryption version`);
          }
      }
  }

  //---------------------------------------------------------------------------

  private session__decrypt_body (body: object, creds: AuthLoginCreds): Promise<AuthSessionItem | null>
  {
      if (!body)
      {
          return Promise.resolve (null);
      }

      const aitem: string = body['aitem'];

      if (!aitem)
      {
          return Promise.resolve (null);
      }

      return this.decrypt_item (aitem, creds.vsec).then (plaintext => {

          console.log(plaintext);

          return JSON.parse(plaintext) as AuthSessionItem

      });
  }

  //---------------------------------------------------------------------------

  private async session__login_request (subscriber: Subscriber<AuthLoginItem>, creds: AuthLoginCreds): Promise<void>
  {
    var header: object;

    if (creds.user && creds.pass)
    {
      const crypt4 = await this.crypt4_promise(creds);

      // use the old crypt4 authentication mechanism
      // to create a session handle in backend
      header = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + crypt4}), observe: 'events', reportProgress: true};
    }
    else
    {
      header = {observe: 'events', reportProgress: true};
    }

    this.login__handle_error (this.http.post('json/AUTH/session_add', JSON.stringify ({type: 1, info: creds.browser_info}), header), subscriber, creds).subscribe (async (event: HttpEvent<AuthSessionItem>) => {

      switch (event.type)
      {
        case HttpEventType.Response:  // final event
        {
          try
          {
            const item = await this.session__decrypt_body(event.body, creds);

            subscriber.next(new AuthLoginItem(0, item));
          }
          catch (error)
          {
            subscriber.next(new AuthLoginItem(0, null));
          }

          break;
        }
      }

    }, (error) => subscriber.next (new AuthLoginItem (0, null)));
  }

  //---------------------------------------------------------------------------

  public session__login (creds: AuthLoginCreds): Observable<AuthLoginItem>
  {
    return new Observable (subscriber => {
      void this.session__login_request (subscriber, creds);
    });
  }

  //---------------------------------------------------------------------------

  public session__logout (session_expired: boolean)
  {
    this.modal_service.dismissAll();
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

  public session__json_rpc<T> (sitem: AuthSessionItem, stoken: string, qbus_module: string, qbus_method: string, qbus_cdata: object, qbus_clist: object): Observable<T>
  {
      const enjs: AuthEnjs = this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_cdata);

      return this.session__convert_error (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).pipe(switchMap((event: HttpEvent<string>) => {

          if (event.type !== HttpEventType.Response)
          {
              return of(null); // ignore intermediate events
          }

          const body = event.body;

          if (!body)
          {
              return of({} as T);
          }

          // encrypted response
          if (enjs.vsec)
          {
              return from(this.decrypt_item (body, enjs.vsec)).pipe(map((plaintext: string) => JSON.parse(plaintext) as T));
          }

          // plain response
          return of(JSON.parse(body) as T);

      }), catchError((err: QbngErrorHolder) => {

          throw err;
      }));
  }

  //---------------------------------------------------------------------------

  public session__json_rpc_upload (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<AuthUploadItem>
  {
      const enjs: AuthEnjs = this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_params);

      return this.session__convert_error(this.http.post(enjs.url,enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).pipe(switchMap((event: HttpEvent<string>) => {

          // Upload progress
          if (event.type === HttpEventType.UploadProgress)
          {
              const percent = event.total ? Math.round(100 * (event.loaded / event.total)) : 0;

              return of(new AuthUploadItem(0, percent));
          }

          // Final response
          if (event.type === HttpEventType.Response)
          {
              const body = event.body;

              if (!body)
              {
                  return of(new AuthUploadItem(1, 0, {}));
              }

              // encrypted response
              if (enjs.vsec)
              {
                  return from(this.decrypt_item(body, enjs.vsec)).pipe(map((plaintext: string) => new AuthUploadItem(1, 0, JSON.parse(plaintext))));
              }

              // plain response
              return of(new AuthUploadItem(1, 0, JSON.parse(body)));
          }

          // ignore other events
          return of(null as any);

      }), switchMap(x => x ? of(x) : of()), catchError((err: QbngErrorHolder) => {

          throw err;
      }));
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
