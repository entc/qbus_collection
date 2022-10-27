import { Component, Input, OnInit, Injector, Output, EventEmitter } from '@angular/core';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-password-checker',
  templateUrl: './component.html'
}) export class AuthPasscheckComponent {

  public pass_new1: string;
  public pass_new2: string;
  public err1: string | null = null;
  public err2: string | null = null;
  public strength: number = 0;

  @Output() onChange = new EventEmitter();

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //---------------------------------------------------------------------------

  public password_strength_text (): string
  {
    switch (this.strength)
    {
      case 0: return 'AUTH.PASS_NOPOLICY';
      case 1: return 'AUTH.PASS_WEAK';
      case 2: return 'AUTH.PASS_ENOUGH';
      case 3: return 'AUTH.PASS_STRONG';
      default: return 'ERR.PASS_STRENGTH';
    }
  }

  //---------------------------------------------------------------------------

  public on_change_1 (e: Event)
  {
    const target = e.target as HTMLInputElement;

    if (target && target.value)
    {
      this.pass_new2 = '';
      this.err2 = null;

      this.auth_session.json_rpc ('AUTH', 'ui_pp', {secret : target.value}).subscribe((data: object) => {

        this.strength = data['strength'];
        this.pass_new1 = target.value;
        this.err1 = null;

      }, (err: QbngErrorHolder) => {

        this.err1 = err.text;

      });
    }
  }

  //---------------------------------------------------------------------------

  public on_change_2 (e: Event)
  {
    const target = e.target as HTMLInputElement;

    if (target && target.value)
    {
      this.pass_new2 = target.value;
      this.check_passwords ();
    }
  }

  //---------------------------------------------------------------------------

  private check_passwords ()
  {
    if (this.pass_new1 && this.pass_new2 && this.pass_new1 != this.pass_new2)
    {
      this.err2 = 'AUTH.PASSMISMATCH';
      this.onChange.emit (null);
    }
    else
    {
      this.err2 = null;
      this.onChange.emit (this.pass_new1);
    }
  }
}
