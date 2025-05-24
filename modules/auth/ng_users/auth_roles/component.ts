import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthUserContext } from '@qbus/auth_session';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-roles',
  templateUrl: './component.html'
})
export class AuthRolesComponent implements OnInit {

  @Input() user_ctx: AuthUserContext;

  public roles_items_ui: AuthRolesItem[];
  public roles_items_wp: AuthRolesItem[];
  public roles_items: AuthRolesItem[] | null = null;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.fetch ();
  }

  //-----------------------------------------------------------------------------

  fetch ()
  {
    this.auth_session.json_rpc ('AUTH', 'roles_ui_get', {wpid: this.user_ctx.wpid, gpid: this.user_ctx.gpid, userid: this.user_ctx.userid}).subscribe ((data: AuthRolesItem[]) => this.roles_items_ui = data);
    this.auth_session.json_rpc ('AUTH', 'roles_wp_get', {wpid: this.user_ctx.wpid, gpid: this.user_ctx.gpid, userid: this.user_ctx.userid}).subscribe ((data: AuthRolesItem[]) => this.roles_items_wp = data);
  }

  //-----------------------------------------------------------------------------

  public open_role_add ()
  {
    this.auth_session.json_rpc ('AUTH', 'roles_get', {}).subscribe ((data: AuthRolesItem[]) => {

      var roles: AuthRolesItem[] = [];

      data.forEach ((data_item: AuthRolesItem) => {

        var found = this.roles_items_ui.find((e: AuthRolesItem) => e.roleid == data_item.id);
        if (found == null)
        {
          found = this.roles_items_wp.find((e: AuthRolesItem) => e.roleid == data_item.id);
          if (found == null)
          {
            roles.push (data_item);
          }
        }

      });

      this.roles_items = roles;

    });
  }

  //-----------------------------------------------------------------------------

  public close_role_add ()
  {
    this.roles_items = null;
  }

  //-----------------------------------------------------------------------------

  public role_rm (item: AuthRolesItem)
  {
    this.auth_session.json_rpc ('AUTH', 'roles_ui_rm', {userid: this.user_ctx.userid, urid: item.id, roleid: item.roleid}).subscribe ((data: AuthRolesItem[]) => {

      this.fetch ();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //-----------------------------------------------------------------------------

  public open_role_rm (item: AuthRolesItem)
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('MISC.DELETE', 'AUTH.ROLE_REMOVE', 'MISC.DELETE');

    this.modal_service.open (QbngWarnOptionModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])}).result.then(() => {

      this.role_rm (item);

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  public role_add (item: AuthRolesItem)
  {
    this.auth_session.json_rpc ('AUTH', 'roles_ui_add', {userid: this.user_ctx.userid, roleid: item.id}).subscribe ((data: AuthRolesItem[]) => {

      this.close_role_add ();
      this.fetch ();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //-----------------------------------------------------------------------------

  public warn_role_add (item: AuthRolesItem)
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('MISC.ADD', 'AUTH.ROLE_ADD', 'MISC.ADD');

    this.modal_service.open (QbngWarnOptionModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])}).result.then(() => {

      this.role_add (item);

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------
}

class AuthRolesItem
{
  id: number;
  roleid: number;
  name: string;
  description: string;
}
