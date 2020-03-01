import { Component, Injectable, Directive, TemplateRef } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders } from '@angular/common/http';
import { Observable } from 'rxjs';
import { catchError, retry } from 'rxjs/operators'
import { throwError } from 'rxjs';
import * as CryptoJS from 'crypto-js';
import { NgbModal, NgbModalRef } from '@ng-bootstrap/ng-bootstrap';

//-----------------------------------------------------------------------------

@Injectable()
export class LoginState {

  modal: NgbModalRef;
  template: TemplateRef<any>;
}

//-----------------------------------------------------------------------------

@Injectable()
export class ConfigService
{
  constructor(private http: HttpClient, private modalService: NgbModal, private state: LoginState, private reloadService: PageReloadService)
  {
  }

  //-----------------------------------------------------------------------------

  padding (str: string, max: number)
  {
  	return str.length < max ? this.padding ("0" + str, max) : str;
  }

  //-----------------------------------------------------------------------------

  crypt4 (user: string, pass: string): string
  {
    var iv: string = this.padding ((new Date).getTime().toString(), 16);

    var hash1: string = CryptoJS.SHA256 (user + ":" + pass).toString();
    var hash2: string = CryptoJS.SHA256 (iv + ":" + hash1).toString();

    // create a Object object as text
    return JSON.stringify ({"ha":iv, "id":CryptoJS.SHA256 (user).toString(), "da":hash2});
  }

  //---------------------------------------------------------------------------

  rest_PUT<T> (path: string, params: object): Observable<T>
  {
    var user = sessionStorage.getItem ('auth_user');
    var pass = sessionStorage.getItem ('auth_pass');

    const http_options = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (user, pass)})};

    return this.http
      .put<T>('rest/' + path, JSON.stringify (params), http_options)
      .pipe (catchError ((error) => {

        if (error.status == 401)
        {
          sessionStorage.removeItem ('auth_user');
          sessionStorage.removeItem ('auth_pass');


          this.state.modal = this.modalService.open (this.state.template, {ariaLabelledBy: 'modal-basic-title'});

          this.state.modal.result.then((result) => {

          }, () => {

          });

          console.log ('ERROR');

          return throwError (error);
        }
        else
        {
          return throwError (error);
        }

      }));
  }

  //---------------------------------------------------------------------------

  rest_POST<T> (path: string, params: object): Observable<T>
  {
    var user = sessionStorage.getItem ('auth_user');
    var pass = sessionStorage.getItem ('auth_pass');

    const http_options = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (user, pass)})};

    return this.http
      .post<T>('rest/' + path, JSON.stringify (params), http_options)
      .pipe(catchError(error => {

        if (error.status == 401)
        {
          sessionStorage.removeItem ('auth_user');
          sessionStorage.removeItem ('auth_pass');

          this.state.modal = this.modalService.open (this.state.template, {ariaLabelledBy: 'modal-basic-title'});

          this.state.modal.result.then((result) => {

          }, () => {

          });

          return throwError(error);
        }
        else
        {
          return throwError(error);
        }

      }));
  }

  //---------------------------------------------------------------------------

  get<T> (qbus_module: string, qbus_method: string, params: object): Observable<T>
  {
    var user = sessionStorage.getItem ('auth_user');
    var pass = sessionStorage.getItem ('auth_pass');

    const http_options = {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (user, pass)})};

    return this.http
      .post<T>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params), http_options)
      .pipe(catchError(error => {

        if (error.status == 401)
        {
          sessionStorage.removeItem ('auth_user');
          sessionStorage.removeItem ('auth_pass');

          this.state.modal = this.modalService.open (this.state.template, {ariaLabelledBy: 'modal-basic-title'});


          this.state.modal.result.then((result) => {

          }, (data) => {

          });

          return throwError(error);
        }
        else
        {
          return throwError(error);
        }

      }));
  }

  //---------------------------------------------------------------------------

  cre (user: string, pass: string): Observable<Object>
  {
    sessionStorage.setItem ('auth_user', user);
    sessionStorage.setItem ('auth_pass', pass);

    var o = new Observable<Object>();

    this.get ('AUTH', 'globperson_get', {}).subscribe((data: Object) => {


    });

    return o;
  }

  //-----------------------------------------------------------------------------

  loo ()
  {
    sessionStorage.removeItem ('auth_user');
    sessionStorage.removeItem ('auth_pass');

    this.reloadService.run ();
  }

  //-----------------------------------------------------------------------------
}

@Component({
  selector: 'login-modal-component',
  templateUrl: './modal_login.html',
  providers: [ConfigService]
}) export class LoginModalComponent {

  //-----------------------------------------------------------------------------

  constructor (private configService: ConfigService, private state: LoginState)
  {
    console.log ('login modal component initialized');
  }

  //-----------------------------------------------------------------------------

  login_submit (data)
  {
    this.state.modal.close ('login');
    this.configService.cre (data.user, data.pass);
  }
}

@Directive({
  selector: "[login]"
})
export class LoginTemplateDirective
{
  constructor(loginTemplate: TemplateRef<any>, state: LoginState)
  {
    state.template = loginTemplate;
    console.log ('login template initialized');
  }
}

//-----------------------------------------------------------------------------

@Injectable()
export class PageReloadService
{
  cb : CallableFunction = undefined;

  constructor()
  {
  }

  set (cb: CallableFunction)
  {
    this.cb = cb;
  }

  run ()
  {
    if (this.cb)
    {
      this.cb ();
    }
  }
}
