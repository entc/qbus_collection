import { Injectable } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders, HttpResponse, HttpEvent, HttpEventType } from '@angular/common/http';
import { Observable, Subscriber, throwError } from 'rxjs';
import { catchError } from 'rxjs/operators';
import { AuthLoginItem, AuthSessionItem, AuthLoginCreds } from '@qbus/auth_session';
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
          console.log('final response');

          let data: AuthSessionItem = event.body;

          if (data)
          {
            // set the user
            data.user = creds.user;

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
}

//---------------------------------------------------------------------------
