import { Component, Injectable, Directive, TemplateRef, OnInit, Input, Injector, ElementRef, ViewContainerRef } from '@angular/core';
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

  private secret: string;
  private globpersons: Array<AuthGlobalPerson>;

  //-----------------------------------------------------------------------------

  constructor (private http: HttpClient, private modalService: NgbModal)
  {
    this.fetch_login = false;
    this.auth_credentials = new BehaviorSubject<AuthCredential> ({wpid: undefined, gpid: undefined, firstname: undefined, lastname: undefined, workspace: undefined, secret: undefined, roles: undefined});

    this.fetch_login = true;
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

  http_crypt4 (page: string)
  {
    var user = sessionStorage.getItem ('auth_user');
    var pass = sessionStorage.getItem ('auth_pass');
    var wpid = sessionStorage.getItem ('auth_wpid');

    console.log('got session storage');

    if (user && pass)
    {
      console.log('use credentials = ' + page);
      return {headers: new HttpHeaders ({'Authorization': "Crypt4 " + this.crypt4 (user, pass, wpid)})};
    }
    else
    {
      console.log('no credentials = ' + page);
      return {};
    }
  }

  //---------------------------------------------------------------------------

  handle_errors<T> (http_request: Observable<T>): Observable<T>
  {
    return http_request.pipe (catchError ((error) => {

      console.log(this.fetch_login);

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

        this.fetch_login = false;


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
    return this.handle_errors<T> (this.http.get<T>('rest/' + path, this.http_crypt4(path)));
  }

  //---------------------------------------------------------------------------

  rest_PUT<T> (path: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.put<T>('rest/' + path, JSON.stringify (params), this.http_crypt4(path)));
  }

  //---------------------------------------------------------------------------

  rest_POST<T> (path: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.post<T>('rest/' + path, JSON.stringify (params), this.http_crypt4(path)));
  }

  //---------------------------------------------------------------------------

  rest_PATCH<T> (path: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.patch<T>('rest/' + path, JSON.stringify (params), this.http_crypt4(path)));
  }

  //---------------------------------------------------------------------------

  json_rpc<T> (qbus_module: string, qbus_method: string, params: object): Observable<T>
  {
    return this.handle_errors<T> (this.http.post<T>('json/' + qbus_module + '/' + qbus_method, JSON.stringify (params), this.http_crypt4(qbus_module + '/' + qbus_method)));
  }

  //-----------------------------------------------------------------------------

  public encrypt (text: string): string
  {
    var secmode = { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 };

    if (this.secret)
		{
			return CryptoJS.AES.encrypt(text, this.secret, secmode).toString();
		}
		else
		{
			throw "{encrypt} secret is empty";
		}
  }

  //-----------------------------------------------------------------------------

  decrypt (encrypted_text: string, secret: string): string
  {
    var secmode = { mode: CryptoJS.mode.CFB, padding: CryptoJS.pad.AnsiX923 };

		try
		{
			return CryptoJS.enc.Utf8.stringify (CryptoJS.AES.decrypt (encrypted_text, secret, secmode));
		}
		catch (e)
		{
			return '[???]';
		}
  }

  //-----------------------------------------------------------------------------

  fetch_globalpersons ()
  {
    this.json_rpc ('AUTH', 'globperson_get', {}).subscribe((data: Array<AuthGlobalPerson>) => {

      if (data.length > 0)
      {
        this.globpersons = [];

        for (var i in data)
        {
          const gpid: AuthGlobalPerson = data[i];
          this.globpersons.push ({gpid: gpid.gpid, firstname: this.decrypt (gpid.firstname, this.secret), lastname: this.decrypt (gpid.lastname, this.secret)});
        }
      }

    });
  }

  //-----------------------------------------------------------------------------

  gpg ()
  {
    this.json_rpc ('AUTH', 'account_get', {}).subscribe((data: Array<AuthCredential>) => {

      if (data.length > 0)
      {
        var c: AuthCredential = data[0];

        // decrypt the secret with our password
        this.secret = this.decrypt (c.secret, sessionStorage.getItem ('auth_pass'));

        // decrypt the name
        c.firstname = this.decrypt (c.firstname, this.secret);
        c.lastname = this.decrypt (c.lastname, this.secret);

        this.auth_credentials.next (c);

        this.fetch_globalpersons ();
      }

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
    console.log ('unset authentication');

    sessionStorage.removeItem ('auth_user');
    sessionStorage.removeItem ('auth_pass');
    sessionStorage.removeItem ('auth_wpid');

    this.fetch_login = false;
    this.auth_credentials.next ({wpid: undefined, gpid: undefined, firstname: undefined, lastname: undefined, workspace: undefined, secret: undefined, roles: undefined});
  }

  //-----------------------------------------------------------------------------

  login ()
  {
    var modalRef = this.modalService.open (AuthLoginModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static"});

    modalRef.result.then((result) => {

      this.fetch_login = true;
      this.cre (result.user, result.pass);

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  info ()
  {
    var modalRef = this.modalService.open (AuthInfoModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", centered: true});

    modalRef.result.then((result) => {

      this.loo ();

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  name_gpid (gpid: number): string
  {
    try
    {
      const gpid_node = this.globpersons.find ((item: AuthGlobalPerson) => item.gpid == gpid);
      return gpid_node.firstname + ' ' + gpid_node.lastname;
    }
    catch (e)
    {
      return "[unknown]";
    }
  }

  //-----------------------------------------------------------------------------
}

class AuthGlobalPerson
{
  gpid: number;
  firstname: string;
  lastname: string;
}

class AuthCredential
{
  gpid: number;
  wpid: number;
  firstname: string;
  lastname: string;
  workspace: string;
  secret: string;
  roles: object;
}

//=============================================================================

@Component({
  selector: 'auth-login-modal-component',
  templateUrl: './modal_login.html'
}) export class AuthLoginModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private AuthService: AuthService)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  login_submit (form: NgForm)
  {
    this.modal.close (form.value);
    form.resetForm ();
  }

  //---------------------------------------------------------------------------

  login_cancel ()
  {
    this.modal.dismiss ();
  }

}

//=============================================================================

@Component({
  selector: 'auth-logout-modal-component',
  templateUrl: './modal_info.html'
}) export class AuthInfoModalComponent implements OnInit {

  public auth_credentials: BehaviorSubject<AuthCredential>;

  //---------------------------------------------------------------------------

  constructor (public auth_service: AuthService, public modal: NgbActiveModal)
  {
    this.auth_credentials = auth_service.auth_credentials;
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  logout_submit ()
  {
    this.modal.close ();
  }

  //---------------------------------------------------------------------------

  logout_cancel ()
  {
    this.modal.dismiss ();
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

  info ()
  {
    this.auth_service.info ();
  }

  //---------------------------------------------------------------------------

  login ()
  {
    this.auth_service.login ();
  }
}

//=============================================================================

@Directive({
  selector: '[authRole]'
})
export class AuthRoleDirective {

  private auth_credentials: AuthCredential = {wpid: undefined, gpid: undefined, firstname: undefined, lastname: undefined, workspace: undefined, secret: undefined, roles: undefined};
  private permissions: Array<string>;

  //---------------------------------------------------------------------------

  constructor (public auth_service: AuthService, private element: ElementRef, private templateRef: TemplateRef<any>, private viewContainer: ViewContainerRef)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit ()
  {
    this.auth_service.auth_credentials.subscribe ((val: AuthCredential) => {
      this.auth_credentials = val;
      this.updateView();
    });
  }

  //---------------------------------------------------------------------------

  @Input ()
  set authRole (val: Array<string>)
  {
    this.permissions = val;
    this.updateView ();
  }

  //---------------------------------------------------------------------------

  private updateView ()
  {
    if (this.has_role__or__permission())
    {
      this.viewContainer.createEmbeddedView(this.templateRef);
    }
    else
    {
      this.viewContainer.clear();
    }
  }

  //---------------------------------------------------------------------------

  private has_role__or__permission (): boolean
  {
    if (this.permissions.length)
    {
      if (this.auth_credentials.roles)
      {
        // check if permissions are in the roles
        for (var i in this.permissions)
        {
          if (this.auth_credentials.roles [this.permissions[i]] != undefined)
          {
            return true;
          }
        }
      }

      return false;
    }
    else
    {
      return this.auth_credentials.wpid > 0;
    }
  }
}
