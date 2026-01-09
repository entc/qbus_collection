import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthUserContext, AuthWpInfo } from '@qbus/auth_session';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-users',
  templateUrl: './component.html'
})
export class AuthUsersComponent implements OnInit {

  public users: Observable<AuthUserItem[]> = null;
  public info: Observable<AuthWpInfo> = of();

  private _wpid: number = null;
  private _auth: boolean = false;
  private session_obj;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  @Input() set wpid (wpid: number)
  {
    this._wpid = wpid;
    this.fetch();
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //-----------------------------------------------------------------------------

  ngAfterViewInit()
  {
    this.session_obj = this.auth_session.session.subscribe ((data) => {

      if (data)
      {
        this._auth = true;
        this.fetch_info ();
      }
      else
      {
        this._auth = false;
      }

      this.fetch();

    });
  }

  //-----------------------------------------------------------------------------

  ngOnDestroy()
  {
    this.session_obj.unsubscribe();
  }

  //-----------------------------------------------------------------------------

  private fetch_info ()
  {
    this.info = this.auth_session.json_rpc ('AUTH', 'wp_info_get', {wpid: this._wpid});
  }

  //-----------------------------------------------------------------------------

  public fetch ()
  {
    if (this._auth)
    {
      this.users = this.auth_session.json_rpc ('AUTH', 'ui_users', {wpid: this._wpid});
    }
    else
    {
      this.users = null;
    }
  }

  //-----------------------------------------------------------------------------

  public open_settings (item: AuthUserItem)
  {
    var ctx: AuthUserContext = new AuthUserContext;

    ctx.wpid = Number(this._wpid);
    ctx.gpid = item.gpid;
    ctx.userid = item.userid;
    ctx.active = item.active == 1 ? true: false;
    ctx.info = null;

    this.modal_service.open (AuthUsersSettingsModalComponent, {ariaLabelledBy: 'modal-basic-title', 'size': 'lg', injector: Injector.create([{provide: AuthUserContext, useValue: ctx}])}).result.then(() => {

    }, () => {

      this.fetch ();

    });
  }

  //-----------------------------------------------------------------------------

/*
  public open_sessions (item: AuthUserItem, info: AuthWpInfo)
  {
    var ctx: AuthUserContext = new AuthUserContext;

    ctx.wpid = Number(this._wpid);
    ctx.gpid = item.gpid;
    ctx.userid = item.userid;
    ctx.info = info;

    this.modal_service.open (AuthUsersSessionsModalComponent, {ariaLabelledBy: 'modal-basic-title', 'size': 'lg', injector: Injector.create([{provide: AuthUserContext, useValue: ctx}])});
  }
*/

  //-----------------------------------------------------------------------------

  public open_add (info: AuthWpInfo)
  {
    var ctx: AuthUserContext = new AuthUserContext;

    ctx.wpid = Number(this._wpid);
    ctx.gpid = null;
    ctx.userid = null;
    ctx.info = info;

    this.modal_service.open (AuthUsersAddModalComponent, {ariaLabelledBy: 'modal-basic-title', size: 'xl', injector: Injector.create([{provide: AuthUserContext, useValue: ctx}])}).result.then(() => {

      this.fetch ();

    });
  }

  //-----------------------------------------------------------------------------

  public open_add_merge (info: AuthWpInfo)
  {
    var ctx: AuthUserContext = new AuthUserContext;

    ctx.wpid = Number(this._wpid);
    ctx.gpid = null;
    ctx.userid = null;
    ctx.info = info;

    this.modal_service.open (AuthUsersMergeModalComponent, {ariaLabelledBy: 'modal-basic-title', size: 'xl', injector: Injector.create([{provide: AuthUserContext, useValue: ctx}])}).result.then(() => {

      this.fetch ();

    });
  }
}

//-----------------------------------------------------------------------------

export class AuthUserItem
{
  id: number;
  userid: number;
  gpid: number;
  firstname: string;
  lastname: string;
  active: number;
  state: number;
  opt_msgs: number;
  opt_2factor: number;
  opt_locale: string;
  opt_ttl: number;
  last: string;
  ip: string;
  logins: number;
}

//=============================================================================

@Component({
  selector: 'auth-users-settings-modal',
  templateUrl: './modal_settings.html',
  styleUrls: ['./modal_settings.css']
}) export class AuthUsersSettingsModalComponent {

  public mode: number = 1;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private modal_service: NgbModal, private auth_session: AuthSession, public ctx: AuthUserContext)
  {
  }

  //---------------------------------------------------------------------------

  public auth_active_changed (val: boolean)
  {
    this.auth_session.json_rpc ('AUTH', 'ui_set', {wpid: this.ctx.wpid, gpid: this.ctx.gpid, userid: this.ctx.userid, active: val}).subscribe (() => {

      this.modal_service.open (QbngSpinnerOkModalComponent, {ariaLabelledBy: 'modal-basic-title', size: 'sm'});

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------

  public open_change_password ()
  {
    this.modal_service.open (AuthUsersPasswdModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: AuthUserContext, useValue: this.ctx}])});
  }

  //---------------------------------------------------------------------------

  public open_account_delete ()
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('MISC.ACCOUNT_DELETE', 'AUTH.ACCOUNT_DELETE_INFO', 'MISC.ACCOUNT_DELETE');

    this.modal_service.open(QbngWarnOptionModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])}).result.then(() => {

      this.account_delete ();

    }, () => {});
  }

  //---------------------------------------------------------------------------

  private account_delete ()
  {
    this.auth_session.json_rpc ('AUTH', 'ui_rm', {wpid: this.ctx.wpid, gpid: this.ctx.gpid, userid: this.ctx.userid}).subscribe (() => {

      this.modal.close();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }
}

//=============================================================================

@Component({
  selector: 'auth-passwd-modal',
  templateUrl: './modal_passwd.html'
}) export class AuthUsersPasswdModalComponent {

  public input_pass: string;
  public input_user: string;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private modal_service: NgbModal, private auth_session: AuthSession, public ctx: AuthUserContext)
  {
  }

  //---------------------------------------------------------------------------

  public apply()
  {
    this.auth_session.json_rpc ('AUTH', 'ui_set', {wpid: this.ctx.wpid, gpid: this.ctx.gpid, userid: this.ctx.userid, user: this.input_user, pass: this.input_pass}).subscribe (() => {

      this.modal_service.open (QbngSpinnerOkModalComponent, {ariaLabelledBy: 'modal-basic-title', size: 'sm'});

      this.modal.close();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }

}

//=============================================================================

@Component({
  selector: 'auth-users-add-modal',
  templateUrl: './modal_add.html'
}) export class AuthUsersAddModalComponent {

  public name: string;
  public pass: string;

  public title: string;
  public firstname: string;
  public lastname: string;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private modal_service: NgbModal, private auth_session: AuthSession, public ctx: AuthUserContext)
  {
  }

  //---------------------------------------------------------------------------

  public apply ()
  {
    this.auth_session.json_rpc ('AUTH', 'ui_add', {wpid: this.ctx.wpid, username: this.name, password: this.pass, gpdata: {title: this.title, firstname: this.firstname, lastname: this.lastname}}).subscribe (() => {

      this.modal.close();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------

  public on_check_password (val: string): void
  {
    this.pass = val;
  }

}

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-users-merge-modal',
  templateUrl: './modal_merge.html'
}) export class AuthUsersMergeModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private modal_service: NgbModal, private auth_session: AuthSession, public ctx: AuthUserContext)
  {
  }

  //---------------------------------------------------------------------------

  public apply ()
  {

  }

}

//-----------------------------------------------------------------------------
