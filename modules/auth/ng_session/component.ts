import { Component, EventEmitter, Directive, Input, Output, ElementRef, TemplateRef, ViewContainerRef } from '@angular/core';
import { HttpErrorResponse } from '@angular/common/http';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { BehaviorSubject, Observable, of } from 'rxjs';
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

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private element: ElementRef, private templateRef: TemplateRef<any>, private viewContainer: ViewContainerRef)
  {
    this.auth_session.roles.subscribe ((roles: object) => {

      this.roles = roles;
      this.updateView ();
    });
  }

  //---------------------------------------------------------------------------

  @Input () set authSessionRole (val: Array<string>)
  {
    this.permissions = val;
    this.updateView ();
  }

  //---------------------------------------------------------------------------

  private updateView ()
  {
    if (this.auth_session.contains_role__or (this.roles, this.permissions))
    {
      this.viewContainer.createEmbeddedView(this.templateRef);
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

  public sitem: AuthSessionItem = null;

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

  @Input() old: boolean;
  @Output() onChange = new EventEmitter();

  public pass_old: string;
  public pass_new1: string;
  public pass_new2: string;

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

  public on_change_1 (val: string)
  {
    this.pass_new1 = val;
    this.check_passwords ();
  }

  //---------------------------------------------------------------------------

  public on_change_2 (val: string)
  {
    this.pass_new2 = val;
    this.check_passwords ();
  }

  //---------------------------------------------------------------------------

  private check_passwords ()
  {
    if (this.pass_new1 && this.pass_new2 && this.pass_new1 != this.pass_new2)
    {
      this.err = 'AUTH.PASSMISMATCH';
    }
    else
    {
      this.err = null;
    }
  }

  //---------------------------------------------------------------------------

  public apply ()
  {
    var params = {secret: this.pass_new1, user: this.user_value};

    if (this.old)
    {
      params['pass'] = this.pass_old;
    }

    this.auth_session.json_rpc ('AUTH', 'ui_set', params).subscribe(() => {

      this.was_set = true;
      this.onChange.emit (true);

    }, () => {

      this.err = 'AUTH.WRONGPASS';

    });
  }

  //---------------------------------------------------------------------------

  generate_password ()
  {

  }
}

//=============================================================================

@Component({
  selector: 'auth-session-msgs',
  templateUrl: './component_msgs.html'
}) export class AuthSessionMsgsComponent {

  msgs_list: AuthMsgsItem[];
  new_item: AuthMsgsItem = null;

  msgs_active: boolean = false;

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private modal_service: NgbModal)
  {
    this.auth_session.session.subscribe ((data) => {

      if (data)
      {
        this.fetch ();
      }
      else
      {
        this.msgs_list = [];
      }

    });
  }

  //---------------------------------------------------------------------------

  fetch ()
  {
    this.auth_session.json_rpc ('AUTH', 'msgs_get', {}).subscribe ((data: AuthMsgsItem[]) => {

      for (var i in data)
      {
        const item: AuthMsgsItem = data[i];
        item.mode = 0;
      }

      this.msgs_list = data;

    });
  }

  //---------------------------------------------------------------------------

  msgs_active_changed (val: boolean)
  {
    this.msgs_active = val;
  }

  //---------------------------------------------------------------------------

  new_start ()
  {
    this.new_item = new AuthMsgsItem;
  }

  //---------------------------------------------------------------------------

  new_cancel ()
  {
    this.new_item = null;
  }

  //---------------------------------------------------------------------------

  new_apply ()
  {
    this.auth_session.json_rpc ('AUTH', 'msgs_add', this.new_item).subscribe (() => {

      this.new_item = null;
      this.fetch ();

    });
  }

  //---------------------------------------------------------------------------

  open_edit (item: AuthMsgsItem)
  {
    item.mode = 1;
  }

  //---------------------------------------------------------------------------

  cancel_edit (item: AuthMsgsItem)
  {
    item.mode = 0;
    this.fetch ();
  }

  //---------------------------------------------------------------------------

  apply (item: AuthMsgsItem)
  {
    this.auth_session.json_rpc ('AUTH', 'msgs_set', item).subscribe (() => {

      item.mode = 0;
      this.fetch ();

    });
  }

  //---------------------------------------------------------------------------

  open_rm (item: AuthMsgsItem)
  {
    item.mode = 2;
  }

  //---------------------------------------------------------------------------

  rm_apply (item: AuthMsgsItem)
  {
    this.auth_session.json_rpc ('AUTH', 'msgs_rm', item).subscribe (() => {

      item.mode = 0;
      this.fetch ();

    });
  }

  //---------------------------------------------------------------------------

  rm_cancel (item: AuthMsgsItem)
  {
    item.mode = 0;
  }

  //---------------------------------------------------------------------------
}

class AuthMsgsItem
{
  id: number;
  email: string;
  type: number;

  mode: number;
}

//=============================================================================
