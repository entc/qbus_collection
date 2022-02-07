import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthUserContext, AuthSessionLoginWidget } from '@qbus/auth_session';
import { Observable, BehaviorSubject, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-session-menu',
  templateUrl: './component.html'
}) export class AuthSessionMenuComponent {

  public session: BehaviorSubject<AuthSessionItem>;
  public ticks: number;

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private modal_service: NgbModal)
  {
    this.session = auth_session.session;
    auth_session.idle.subscribe (s => this.ticks = s);
  }

  //---------------------------------------------------------------------------

  public login (): void
  {
    this.auth_session.show_login ();
  }

  //---------------------------------------------------------------------------

  public open_info (): void
  {
    this.modal_service.open (AuthSessionInfoModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static"}).result.then(() => {

    }, () => {

    });
  }

}

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-session-info-modal-component',
  templateUrl: './modal_info.html'
}) export class AuthSessionInfoModalComponent {

  public sitem: AuthSessionItem;
  public mode: number = 0;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private auth_session: AuthSession)
  {
    auth_session.session.subscribe ((data: AuthSessionItem) => {

console.log(data);

      if (data)
      {
        this.sitem = data;
      }
      else
      {
        this.modal.dismiss ();
      }

    });
  }

  //---------------------------------------------------------------------------

  toogle_edit ()
  {
    this.mode = 1;
  }

  //---------------------------------------------------------------------------

  untoogle_edit ()
  {
    this.mode = 0;
  }

  //---------------------------------------------------------------------------

  logout_submit ()
  {
    this.auth_session.disable ();
    this.modal.dismiss ();
  }

  //---------------------------------------------------------------------------

  change_name ()
  {
    this.mode = 2;
  }

  //---------------------------------------------------------------------------

  change_name_apply ()
  {
    this.auth_session.json_rpc ('AUTH', 'globperson_set', {firstname : this.sitem.firstname, lastname : this.sitem.lastname}).subscribe(() => {

      this.reset_mode ();

    });
  }

  //---------------------------------------------------------------------------

  change_password ()
  {
    this.mode = 3;
  }

  //---------------------------------------------------------------------------

  change_password_apply ()
  {

  }

  //---------------------------------------------------------------------------

  open_messages_modal ()
  {
    this.mode = 5;
  }

  //---------------------------------------------------------------------------

  disabe_account ()
  {
    this.mode = 4;
  }

  //---------------------------------------------------------------------------

  disabe_account_apply ()
  {

  }

  //---------------------------------------------------------------------------

  reset_mode ()
  {
    this.mode = 1;
  }
}
