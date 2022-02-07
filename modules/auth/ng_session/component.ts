import { Component, EventEmitter, Directive, Input, Output, ElementRef, TemplateRef, ViewContainerRef, OnInit, Injector } from '@angular/core';
import { HttpErrorResponse } from '@angular/common/http';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { BehaviorSubject, of } from 'rxjs';
import { AuthSession, AuthSessionItem, AuthUserContext } from '@qbus/auth_session';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngErrorModalComponent } from '@qbus/qbng_modals/component';

//=============================================================================

//=============================================================================


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
