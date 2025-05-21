import { Component, Input, Output, OnInit, Injector, EventEmitter } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthLoginItem, AuthRecipient, AuthWorkspace } from '@qbus/auth_session';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-login',
  templateUrl: './component.html'
}) export class AuthLoginComponent implements OnInit {

  public mode: number = 0;

  public user: string;
  public pass: string;
  public code: string;

  public sitem: AuthSessionItem = null;
  public err: boolean = false;

  public running: boolean = false;
  private session_obj;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.session_obj = this.auth_session.session.subscribe ((data: AuthSessionItem) => {

      if (data)
      {
        this.sitem = data;
      }
      else
      {
        this.sitem = null;
      }

    });
  }

  //-----------------------------------------------------------------------------

  ngOnDestroy()
  {
    this.session_obj.unsubscribe();
  }

  //---------------------------------------------------------------------------

  private login (user, pass, code, wpid = 0)
  {
    this.running = true;

    this.auth_session.enable (user, pass, code, wpid).subscribe ((litem: AuthLoginItem) => {

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
          const workspace_list: AuthWorkspace[] = litem.list as AuthWorkspace[];

          // consider special case
          if (workspace_list.length == 1)
          {
            this.login (user, pass, '', workspace_list[0].wpid);
          }
          else
          {
            let val = new AuthWorkspacesInjector (workspace_list);
            this.modal_service.open (AuthWorkspacesModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: AuthWorkspacesInjector, useValue: val}])}).result.then((wpid: number) => this.login (user, pass, '', wpid), () => {});
          }

          break;
        }
        case 2:  // 2factor
        {
          let val = new AuthRecipientsInjector (litem.list as AuthRecipient[], litem.token);
          this.modal_service.open (Auth2FactorModalComponent, {ariaLabelledBy: 'modal-basic-title', backdrop: "static", injector: Injector.create([{provide: AuthRecipientsInjector, useValue: val}])}).result.then((result) => this.login (user, pass, result['code']), () => {});

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

class AuthWorkspacesInjector
{
  constructor (public workspaces: AuthWorkspace[]) {}
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

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-wpace-modal-component',
  templateUrl: './modal_workspaces.html'
}) export class AuthWorkspacesModalComponent {

  workspaces: any;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, workspaces_injector: AuthWorkspacesInjector)
  {
    this.workspaces = workspaces_injector.workspaces;
  }

  //---------------------------------------------------------------------------

  select_workspace (wpid: number)
  {
    this.modal.close (wpid);
  }
}

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-login-modal-component',
  templateUrl: './modal_login.html'
}) export class AuthLoginModalComponent {

  constructor (public auth_session: AuthSession, public modal: NgbActiveModal)
  {
  }

}
