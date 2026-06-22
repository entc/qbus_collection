import { Injectable } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders, HttpResponse, HttpEvent, HttpEventType } from '@angular/common/http';
import { Observable, Subscriber, throwError, from, of } from 'rxjs';
import { switchMap, map, catchError, filter } from 'rxjs/operators';
import { AuthLoginItem, AuthUploadItem, AuthSessionItem, AuthLoginCreds, ConnStatus } from '@qbus/auth_session';
import { QbngErrorHolder } from '@qbus/qbng_modals/header';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QCrypt } from '@qbus/qcrypt';
//---------------------------------------------------------------------------

type RpcEvent<T> =
  | { type: 'progress'; value: number }
  | { type: 'result'; value: T };

//---------------------------------------------------------------------------

@Injectable() export class ConnService
{
  // the connection status is always available and connected
  public status: Observable<ConnStatus> = new Observable ((subscriber) => subscriber.next({state: 0, url: null, connected: true}));

  private qcrypt: QCrypt = new QCrypt;

  constructor (private http: HttpClient, private modal_service: NgbModal)
  {
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

      return this.qcrypt.decrypt_item (aitem, creds.vsec).then (plaintext => {

          return JSON.parse(plaintext) as AuthSessionItem

      });
  }

  //---------------------------------------------------------------------------

  private async session__login_request (subscriber: Subscriber<AuthLoginItem>, creds: AuthLoginCreds): Promise<void>
  {
    var header: object;

    if (creds.user && creds.pass)
    {
      const crypt4 = await this.qcrypt.crypt4_authentication(creds);

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

  private async construct_enjs (sitem: AuthSessionItem, stoken: string, qbus_module: string, qbus_method: string, qbus_params: object): Promise<AuthEnjs>
  {
      if (sitem)
      {
          const bearer = await this.qcrypt.header_base64 (sitem);

          return {

            url: 'enjs/' + qbus_module + '/' + qbus_method,
            header: new HttpHeaders ({'Authorization': "Bearer " + bearer, 'Cache-Control': 'no-cache', 'Pragma': 'no-cache'}),
            params: await this.qcrypt.encrypt_object (sitem, qbus_params),
            vsec: sitem.vsec

          } as AuthEnjs;
      }

      return {

        url: (stoken ? `json/${qbus_module}/${qbus_method}/__P/${stoken}` : `json/${qbus_module}/${qbus_method}`),
        header: new HttpHeaders({'Cache-Control': 'no-cache', Pragma: 'no-cache'}),
        params: JSON.stringify(qbus_params),
        vsec: null

      } as AuthEnjs;
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
      return from(this.construct_enjs(sitem, stoken, qbus_module, qbus_method, qbus_cdata)).pipe(switchMap((enjs: AuthEnjs) =>

          this.session__convert_error (this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).pipe(switchMap((event: HttpEvent<string>) => {

              if (event.type !== HttpEventType.Response)
              {
                  return of(null); // ignore intermediate events
              }

              const body = event.body;

              if (!body)
              {
                  return of({} as T);
              }

              if (enjs.vsec)
              {
                  return from(this.qcrypt.decrypt_item(body, enjs.vsec)).pipe(map((plaintext: string) => JSON.parse(plaintext) as T));
              }

              return of(JSON.parse(body) as T);

          }))

      ));
  }

  //---------------------------------------------------------------------------

  public session__json_rpc_upload (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<AuthUploadItem>
  {
      return from(this.construct_enjs (sitem, stoken, qbus_module, qbus_method, qbus_params)).pipe(switchMap((enjs: AuthEnjs) =>

          this.session__convert_error(this.http.post(enjs.url,enjs.params, {headers: enjs.header, responseType: 'text', observe: 'events', reportProgress: true})).pipe(switchMap((event: HttpEvent<string>) => {

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
                      return from(this.qcrypt.decrypt_item(body, enjs.vsec)).pipe(map((plaintext: string) => new AuthUploadItem(1, 0, JSON.parse(plaintext))));
                  }

                  // plain response
                  return of(new AuthUploadItem(1, 0, JSON.parse(body)));
              }

              // ignore other events
              return of(null as any);

          }))

      ));
  }

  //---------------------------------------------------------------------------

  public session__json_rpc_blob (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<Blob>
  {
      return from(this.construct_enjs(sitem, stoken, qbus_module, qbus_method, qbus_params)).pipe(switchMap((enjs: AuthEnjs) =>
          this.session__convert_error(this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'blob'}))
      ));
  }

  //---------------------------------------------------------------------------

  public session__json_rpc_resp (qbus_module: string, qbus_method: string, qbus_params: object, sitem: AuthSessionItem, stoken: string): Observable<HttpResponse<Blob>>
  {
      return from(this.construct_enjs(sitem, stoken, qbus_module, qbus_method, qbus_params)).pipe(switchMap((enjs: AuthEnjs) =>
          this.session__convert_error(this.http.post(enjs.url, enjs.params, {headers: enjs.header, responseType: 'blob', observe: 'response'}))
      ));
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
