import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthUserContext } from '@qbus/auth_session';
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

  public users: Observable<AuthUserItem[]>;
  private _wpid: number = null;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.fetch ();
  }

  //-----------------------------------------------------------------------------

  @Input() set wpid (wpid: number)
  {
    this._wpid = wpid;
    this.fetch ();
  }

  //-----------------------------------------------------------------------------

  fetch ()
  {
    this.users = this.auth_session.json_rpc ('AUTH', 'ui_users', {wpid: this._wpid});
  }

  //-----------------------------------------------------------------------------

  open_settings (item: AuthUserItem)
  {
    var ctx: AuthUserContext | null = null;

    if (this._wpid)
    {
      ctx = new AuthUserContext;

      ctx.wpid = Number(this._wpid);
      ctx.gpid = item.gpid;
      ctx.userid = item.userid;
    }

    this.modal_service.open (AuthUsersSettingsModalComponent, {ariaLabelledBy: 'modal-basic-title', 'size': 'lg', injector: Injector.create([{provide: AuthUserContext, useValue: ctx}])}).result.then(() => {

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  open_roles (item: AuthUserItem)
  {
    var ctx: AuthUserContext | null = null;

    if (this._wpid)
    {
      ctx = new AuthUserContext;

      ctx.wpid = Number(this._wpid);
      ctx.gpid = item.gpid;
      ctx.userid = item.userid;
    }

    this.modal_service.open (AuthUsersRolesModalComponent, {ariaLabelledBy: 'modal-basic-title', 'size': 'lg', injector: Injector.create([{provide: AuthUserContext, useValue: ctx}])}).result.then(() => {

    }, () => {

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
  templateUrl: './modal_settings.html'
}) export class AuthUsersSettingsModalComponent {

  public input_pass: string;
  public input_user: string;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private modal_service: NgbModal, private auth_session: AuthSession, public ctx: AuthUserContext)
  {
  }

  //---------------------------------------------------------------------------

  public change_password ()
  {
    this.auth_session.json_rpc ('AUTH', 'ui_set', {wpid: this.ctx.wpid, gpid: this.ctx.gpid, userid: this.ctx.userid, user: this.input_user, pass: this.input_pass}).subscribe (() => {

      this.modal_service.open (QbngSpinnerOkModalComponent, {ariaLabelledBy: 'modal-basic-title', size: 'sm'});

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }
}

//=============================================================================

@Component({
  selector: 'auth-users-roles-modal',
  templateUrl: './modal_roles.html'
}) export class AuthUsersRolesModalComponent {

  public input_pass: string;
  public input_user: string;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private modal_service: NgbModal, private auth_session: AuthSession, public ctx: AuthUserContext)
  {
  }

}
