import { Component, Input, OnInit, Injector, Output, EventEmitter } from '@angular/core';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-session-passreset',
  templateUrl: './component.html'
}) export class AuthSessionPassResetComponent {

  @Output() onChange = new EventEmitter();

  public pass_old: string;
  public pass_new: string = null;
  public err: string = null;

  public was_set: boolean = false;
  public user_readonly: boolean = false;
  public user_value: string;

  //---------------------------------------------------------------------------

  @Input () set user (val: string)
  {
    this.user_value = val;
    this.user_readonly = this.user_value != null;
  }

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession)
  {
  }

  //---------------------------------------------------------------------------

  public on_check_password (val: string): void
  {
    this.pass_new = val;
  }

  //---------------------------------------------------------------------------

  public apply ()
  {
    var params = {secret: this.pass_new, user: this.user_value};

    params['pass'] = this.pass_old;

    this.auth_session.json_rpc ('AUTH', 'ui_set', params).subscribe(() => {

      this.was_set = true;
      this.onChange.emit (true);

    }, () => {

      this.err = 'AUTH.WRONGPASS';

    });
  }

  //---------------------------------------------------------------------------
}
