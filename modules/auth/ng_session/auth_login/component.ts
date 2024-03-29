import { Component, Input, Output, OnInit, Injector, EventEmitter } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthSessionLoginWidget } from '@qbus/auth_session';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-login',
  templateUrl: './component.html'
}) export class AuthLoginComponent {

  public user: string;
  public pass: string;
  public code: string;

  public sitem: AuthSessionItem = null;
  public err: boolean = false;

  // this will be set from the auth component directive
  @Output() onClose: EventEmitter<boolean> = new EventEmitter();
  public mode: number = 0;

  public running: boolean = false;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession)
  {
  }

  //---------------------------------------------------------------------------

  login_submit ()
  {
    this.running = true;

    this.auth_session.enable (this.user, this.pass).subscribe ((sitem: AuthSessionItem) => {

      this.running = false;

      if (sitem)
      {
        this.sitem = sitem;
      }
      else
      {
        this.err = true;
      }

    });

    this.sitem = null;
    this.err = false;
    this.user = '';
    this.pass = '';
  }

  //---------------------------------------------------------------------------

  login_again ()
  {
    this.err = false;
  }

  //---------------------------------------------------------------------------

  close ()
  {
    this.onClose.emit (true);
  }

  //---------------------------------------------------------------------------

  open_forgot_password ()
  {
    this.mode = 1;
  }

  //---------------------------------------------------------------------------

  cancel ()
  {
    this.mode = 0;
    this.err = false;
  }

  //---------------------------------------------------------------------------

  send_forgot_password ()
  {
    this.auth_session.json_none_rpc ('AUTH', 'ui_fp_send', {user: this.user}).subscribe(() => {

      this.mode = 2;
      this.code = '';

    }, () => {

      this.err = true;

    });
  }

  //---------------------------------------------------------------------------

  public on_check_password (val: string): void
  {
    this.pass = val;
  }

  //---------------------------------------------------------------------------

  code_forgot_password ()
  {
    this.auth_session.json_none_rpc ('AUTH', 'ui_set', {pass: this.pass, user: this.user, code: this.code}).subscribe(() => {

      this.mode = 3;

    }, () => {

      this.err = true;

    });
  }

  //---------------------------------------------------------------------------

  done_forgot_password ()
  {
    this.mode = 0;
    this.err = false;
  }
}
