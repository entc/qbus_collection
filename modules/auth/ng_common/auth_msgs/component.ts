import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthUserContext } from '@qbus/auth_session';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-msgs',
  templateUrl: './component.html'
}) export class AuthMsgsComponent implements OnInit {

  msgs_list: AuthMsgsItem[] | null = null;
  msgs_list_err: string | null = null;

  new_item: AuthMsgsItem | null = null;

  opt_err: string | null = null;
  opt_msgs: boolean = false;
  opt_2factor: boolean = false;

  @Input() user_ctx: AuthUserContext;

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.auth_session.session.subscribe ((data) => {

      if (data)
      {
        this.fetch_msgs ();
        this.fetch_config ();
      }
      else
      {
        this.msgs_list = [];
      }

    });
  }

  //---------------------------------------------------------------------------

  fetch_msgs ()
  {
    var params = {};

    if (this.user_ctx)
    {
      params = {wpid: this.user_ctx.wpid, gpid: this.user_ctx.gpid}
    }

    this.auth_session.json_rpc ('AUTH', 'msgs_get', params).subscribe ((data: AuthMsgsItem[]) => {

      for (var i in data)
      {
        const item: AuthMsgsItem = data[i];
        item.mode = 0;
      }

      this.msgs_list = data;
      this.msgs_list_err = null;

    }, (err: QbngErrorHolder) => {

      this.msgs_list = null;
      this.msgs_list_err = err.text;

    });
  }

  //---------------------------------------------------------------------------

  fetch_config ()
  {
    var params = {};

    if (this.user_ctx)
    {
      params = {wpid: this.user_ctx.wpid, gpid: this.user_ctx.gpid}
    }

    this.auth_session.json_rpc ('AUTH', 'ui_config_get', params).subscribe ((data: AuthConfigItem) => {

      this.opt_msgs = (data.opt_msgs > 0);
      this.opt_2factor = (data.opt_2factor > 0);
      this.opt_err = null;

    }, (err: QbngErrorHolder) => {

      this.opt_err = err.text;

    });
  }

  //---------------------------------------------------------------------------

  save_config ()
  {
    var params = {opt_msgs: this.opt_msgs ? 1 : 0, opt_2factor: this.opt_2factor ? 1 : 0};

    if (this.user_ctx)
    {
      params['wpid'] = this.user_ctx.wpid;
      params['gpid'] = this.user_ctx.gpid;
    }

    this.auth_session.json_rpc ('AUTH', 'ui_config_set', params).subscribe (() => {

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------

  msgs_active_changed (val: boolean)
  {
    this.opt_msgs = val;
    this.save_config ();
  }

  //---------------------------------------------------------------------------

  msgs_2factor_changed (val: boolean)
  {
    this.opt_2factor = val;
    this.save_config ();
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
      this.fetch_msgs ();

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
    this.fetch_msgs ();
  }

  //---------------------------------------------------------------------------

  apply (item: AuthMsgsItem)
  {
    this.auth_session.json_rpc ('AUTH', 'msgs_set', item).subscribe (() => {

      item.mode = 0;
      this.fetch_msgs ();

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
      this.fetch_msgs ();

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

class AuthConfigItem
{
  opt_msgs: number;
  opt_2factor: number;
}
