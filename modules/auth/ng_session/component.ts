import { Component, EventEmitter } from '@angular/core';
import { HttpErrorResponse } from '@angular/common/http';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { BehaviorSubject } from 'rxjs';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';

//=============================================================================

@Component({
  selector: 'auth-session-menu',
  templateUrl: './component_menu.html'
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

//=============================================================================

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

  logout_submit ()
  {
    this.auth_session.disable ();
    this.modal.dismiss ();
  }

  //---------------------------------------------------------------------------

  change_name ()
  {
    this.mode = 1;
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
    this.mode = 2;
  }

  //---------------------------------------------------------------------------

  change_password_apply ()
  {

  }

  //---------------------------------------------------------------------------

  disabe_account ()
  {
    this.mode = 3;
  }

  //---------------------------------------------------------------------------

  disabe_account_apply ()
  {

  }

  //---------------------------------------------------------------------------

  reset_mode ()
  {
    this.mode = 0;
  }
}
