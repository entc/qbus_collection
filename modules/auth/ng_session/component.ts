import { Component, EventEmitter, Directive, Input, Output, ElementRef, TemplateRef, ViewContainerRef, OnInit, Injector } from '@angular/core';
import { HttpErrorResponse } from '@angular/common/http';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { BehaviorSubject, of } from 'rxjs';
import { AuthSession, AuthSessionItem, AuthUserContext } from '@qbus/auth_session';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngErrorModalComponent } from '@qbus/qbng_modals/component';

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

//=============================================================================

@Directive({
  selector: '[authSessionRole]'
})
export class AuthSessionRoleDirective {

  private permissions: Array<string>;
  private roles: object;
  private enabled: boolean = false;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private element: ElementRef, private templateRef: TemplateRef<any>, private viewContainer: ViewContainerRef)
  {
    this.enabled = false;
    this.roles = null;

    this.auth_session.roles.subscribe ((roles: object) => {

      this.roles = roles;
      this.updateView ();
    });
  }

  //---------------------------------------------------------------------------

  @Input () set authSessionRole (val: Array<string>)
  {
    this.permissions = val;
    this.enabled = true;
    this.updateView ();
  }

  //---------------------------------------------------------------------------

  private updateView ()
  {
    if (this.enabled && this.roles)
    {
      if (this.permissions)
      {
        if (this.auth_session.contains_role__or (this.roles, this.permissions))
        {
          this.viewContainer.clear();
          this.viewContainer.createEmbeddedView(this.templateRef);
        }
        else
        {
          this.viewContainer.clear();
        }
      }
      else
      {
        this.viewContainer.clear();
        this.viewContainer.createEmbeddedView(this.templateRef);
      }
    }
    else
    {
      this.viewContainer.clear();
    }
  }
}

//=============================================================================

@Component({
  selector: 'auth-session-content',
  templateUrl: './component_content.html'
}) export class AuthSessionContentComponent {

  public sitem: AuthSessionItem | null = null;

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession)
  {
    this.auth_session.session.subscribe ((data: AuthSessionItem) => {

      this.sitem = data;

    });
  }

  //---------------------------------------------------------------------------

  on_close ()
  {
  }
}

//=============================================================================

@Component({
  selector: 'auth-session-passreset',
  templateUrl: './component_passreset.html'
}) export class AuthSessionPassResetComponent {

  @Output() onChange = new EventEmitter();

  public pass_old: string;
  public pass_new: string;
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

//=============================================================================

@Component({
  selector: 'auth-session-lang',
  templateUrl: './component_lang.html'
}) export class AuthSessionLangComponent {

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private modal_service: NgbModal)
  {
  }
}

//=============================================================================

@Component({
  selector: 'auth-session-password-checker',
  templateUrl: './component_passcheck.html'
}) export class AuthSessionPasscheckComponent {

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

//=============================================================================
