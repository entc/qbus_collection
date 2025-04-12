import { Component, Input, Output, OnInit, Injector, EventEmitter } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthLoginItem, AuthRecipient } from '@qbus/auth_session';
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

  constructor (private auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //---------------------------------------------------------------------------

  private login (user, pass, code)
  {
    this.running = true;

    this.auth_session.enable (user, pass, code).subscribe ((litem: AuthLoginItem) => {

      switch (litem.state)
      {
        case 0:  // default
        {
          if (litem.sitem)
          {
            this.sitem = litem.sitem;
          }
          else
          {
            this.sitem = null;
            this.err = true;
          }

          break;
        }
        case 1:  // workspace
        {

          break;
        }
        case 2:  // 2factor
        {
          let val = new AuthRecipientsInjector (litem.recipients, litem.token);

          this.modal_service.open (Auth2FactorModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: AuthRecipientsInjector, useValue: val}])}).result.then((result) => {

            this.login (user, pass, result['code']);

          }, () => {

          });

          break;
        }
      }

      this.running = false;

    }, (error) => {

      this.sitem = null;
      this.err = true;

      this.running = false;

    });
  }

  //---------------------------------------------------------------------------

  login_submit ()
  {
    this.login (this.user, this.pass, null);
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

//-----------------------------------------------------------------------------

class AuthRecipientsInjector
{
  constructor (public recipients: AuthRecipient[], public token: string) {}
}

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-2factor-modal-component',
  templateUrl: './modal_2factor.html'
}) export class Auth2FactorModalComponent {

  public mode: number = 0;
  public code: string;

  private token: string;

  public recipients: AuthRecipient[];
  public params: number[] = [];

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, public modal: NgbActiveModal, recipients_injector: AuthRecipientsInjector)
  {
    this.recipients = recipients_injector.recipients;
    this.token = recipients_injector.token;

    if (this.recipients.length > 0)
    {
      for (var i in this.recipients)
      {
        var item: AuthRecipient = this.recipients[i];
        item.used = false;
      }

      this.recipients[0].used = true;
    }

    this.calculate_params ();
  }

  //---------------------------------------------------------------------------

  private calculate_params ()
  {
    this.params = [];

    if (this.recipients.length > 0)
    {
      for (var i in this.recipients)
      {
        var item: AuthRecipient = this.recipients[i];
        if (item.used)
        {
          this.params.push (item.id);
        }
      }
    }
  }

  //---------------------------------------------------------------------------

  public toogle_used (item: AuthRecipient)
  {
    item.used = !item.used;
    this.calculate_params ();
  }

  //---------------------------------------------------------------------------

  public send ()
  {
    if (this.params.length > 0)
    {
      this.mode = 1;

      this.auth_session.json_token_rpc (this.token, 'AUTH', 'ui_2f_send', this.params).subscribe(() => {

        this.mode = 2;
        this.code = '';

      });
    }
  }

  //---------------------------------------------------------------------------

  public login ()
  {
    this.modal.close ({code: this.code});
  }
}
