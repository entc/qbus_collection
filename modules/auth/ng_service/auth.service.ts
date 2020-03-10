import { Component, Injectable, Directive, TemplateRef, OnInit, Injector } from '@angular/core';
import { HttpClient, HttpErrorResponse, HttpHeaders } from '@angular/common/http';
import { Observable, BehaviorSubject } from 'rxjs';
import { catchError, retry } from 'rxjs/operators'
import { throwError } from 'rxjs';
import * as CryptoJS from 'crypto-js';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { NgForm } from '@angular/forms';

//-----------------------------------------------------------------------------

@Injectable()
export class AuthService
{
  fetch_login: boolean;
  public auth_credentials: BehaviorSubject<AuthCredential>;

  constructor (private http: HttpClient, private modalService: NgbModal)
  {
    this.fetch_login = false;
    this.auth_credentials = new BehaviorSubject<AuthCredential> ({firstname: undefined, lastname: undefined});

    this.gpg ();
  }

  //-----------------------------------------------------------------------------

  padding (str: string, max: number)
  {
  	return str.length < max ? this.padding ("0" + str, max) : str;
  }

  //-----------------------------------------------------------------------------

  crypt4 (user: string, pass: string, wpid: string): string
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

  http_crypt4 ()
  {
    var user = sessionStorage.getItem ('auth_user');
    var pass = sessionStorage.getItem ('auth_pass');
    var wpid = sessionStorage.getItem ('auth_wpid');

    if (user && pass)
    {
      return {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (user, pass, wpid)})};
    }
    else
    {
      return {};
    }
  }

  //---------------------------------------------------------------------------

  handle_errors<T> (http_request: Observable<T>): Observable<T>
  {
    return http_request.pipe (catchError ((error) => {

      if (error.status == 401 && this.fetch_login == false)
      {
        this.loo ();

        this.fetch_login = true;

        var modalRef = this.modalService.open (AuthLoginModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static"});

        modalRef.result.then((result) => {

          this.cre (result.user, result.pass);

        }, () => {

        });

        return throwError (error);
      }
      else if (error.status == 428 && this.fetch_login == true)
      {
        var modalRef = this.modalService.open (AuthWorkspacesModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: HttpErrorResponse, useValue: error}])});

        modalRef.result.then((result) => {

          sessionStorage.setItem ('auth_wpid', String(result));
          this.gpg ();

        }, () => {

          this.loo ();

        });

        return throwError (error);
      }
      else
      {
        return throwError (error);
      }
    }));
  }

  //---------------------------------------------------------------------------

  rest_GET<T> (path: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.get<T>('rest/' + path, this.http_crypt4()));
  }

  //---------------------------------------------------------------------------

  rest_PUT<T> (path: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.put<T>('rest/' + path, JSON.stringify (params), this.http_crypt4()));
  }

  //---------------------------------------------------------------------------

  rest_POST<T> (path: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.post<T>('rest/' + path, JSON.stringify (params), this.http_crypt4()));
  }

  //---------------------------------------------------------------------------

  rest_PATCH<T> (path: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.patch<T>('rest/' + path, JSON.stringify (params), this.http_crypt4()));
  }

  //---------------------------------------------------------------------------

  json_rpc<T> (qbus_module: string, qbus_method: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.post<T>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params), this.http_crypt4()));
  }

  //-----------------------------------------------------------------------------

  gpg ()
  {
    this.json_rpc ('AUTH', 'globperson_get', {}).subscribe((data: Array<AuthCredential>) => {

      this.fetch_login = false;
      this.auth_credentials.next (data[0]);

    });
  }

  //---------------------------------------------------------------------------

  cre (user: string, pass: string): Observable<Object>
  {
    sessionStorage.setItem ('auth_user', user);
    sessionStorage.setItem ('auth_pass', pass);

    var o = new Observable<Object>();

    this.gpg ();

    return o;
  }

  //-----------------------------------------------------------------------------

  loo ()
  {
    sessionStorage.removeItem ('auth_user');
    sessionStorage.removeItem ('auth_pass');
    sessionStorage.removeItem ('auth_wpid');

    this.fetch_login = false;
    this.auth_credentials.next ({firstname: undefined, lastname: undefined});
  }

  //-----------------------------------------------------------------------------

  logout ()
  {
    var modalRef = this.modalService.open (AuthLogoutModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static"});

    modalRef.result.then((result) => {

      this.loo ();

    }, () => {


    });
  }

  //-----------------------------------------------------------------------------
}

class AuthCredential
{
  firstname: string;
  lastname: string;
}

//=============================================================================

@Component({
  selector: 'auth-login-modal-component',
  templateUrl: './modal_login.html'
}) export class AuthLoginModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public activeModal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  login_submit (form: NgForm)
  {
    this.activeModal.close (form.value);
    form.resetForm ();
  }
}

//=============================================================================

@Component({
  selector: 'auth-logout-modal-component',
  templateUrl: './modal_logout.html'
}) export class AuthLogoutModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public activeModal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  logout_submit ()
  {
    this.activeModal.close ();
  }

  //---------------------------------------------------------------------------

  logout_cancel ()
  {
    this.activeModal.dismiss ();
  }
}

//=============================================================================

@Component({
  selector: 'auth-wpace-modal-component',
  templateUrl: './modal_workspaces.html'
}) export class AuthWorkspacesModalComponent implements OnInit {

  workspaces: any;

  //---------------------------------------------------------------------------

  constructor (public activeModal: NgbActiveModal, private response: HttpErrorResponse)
  {
    this.workspaces = response.error;
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  select_workspace (wpid: number)
  {
    this.activeModal.close (wpid);
  }

  //---------------------------------------------------------------------------

  select_cancel ()
  {
    this.activeModal.dismiss ();
  }
}

//=============================================================================

@Component({
  selector: 'auth-service-component',
  templateUrl: './component_service.html'
}) export class AuthServiceComponent {

  public auth_credentials: BehaviorSubject<AuthCredential>;

  //---------------------------------------------------------------------------

  constructor (public auth_service: AuthService)
  {
    this.auth_credentials = auth_service.auth_credentials;
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  logout ()
  {
    this.auth_service.logout ();
  }
}
