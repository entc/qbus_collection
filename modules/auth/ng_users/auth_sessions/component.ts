import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';
import { AuthUserContext } from '@qbus/auth_users/component';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-sessions',
  templateUrl: './component.html'
})
export class AuthSessionsComponent implements OnInit {

  @Input() user_ctx: AuthUserContext;

  public token: string = '';
  public session_list: Observable<AuthUsersSessionItem[]>;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.fetch ();
  }

  //---------------------------------------------------------------------------

  private fetch ()
  {
    this.session_list = this.auth_session.json_rpc ('AUTH', 'session_wp_get', {wpid: this.user_ctx.wpid, gpid: this.user_ctx.gpid});
  }

  //---------------------------------------------------------------------------

  private open_session_created (data: AuthUsersSessionItem)
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('MISC.SUCCESS', 'AUTH.SESSION_ADD_INFO', data.token);

    this.modal_service.open(QbngSuccessModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])});

    this.fetch ();
  }

  //---------------------------------------------------------------------------

  public create_new_session ()
  {
    this.auth_session.json_rpc ('AUTH', 'session_add', {wpid: this.user_ctx.wpid, gpid: this.user_ctx.gpid, type: 2, session_ttl: 0}).subscribe ((data: AuthUsersSessionItem) => {

      this.open_session_created (data);

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------

  private session_rm (item: AuthUsersSessionItem)
  {
    this.auth_session.json_rpc ('AUTH', 'session_rm', {wpid: this.user_ctx.wpid, gpid: this.user_ctx.gpid, seid: item.id}).subscribe (() => {

      this.fetch();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create ([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------

  public open_session_rm (item: AuthUsersSessionItem)
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('MISC.DELETE', 'AUTH.SESSION_RM_INFO', 'MISC.DELETE');

    this.modal_service.open(QbngWarnOptionModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])}).result.then(() => {

      this.session_rm (item);

    });
  }

  //---------------------------------------------------------------------------

  public get_type_description (type: number): string
  {
    switch (type)
    {
      case 1: return 'user session';
      case 2: return 'api session';
      default: return 'unknown';
    }
  }

  //---------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------

class AuthUsersSessionItem
{
  id: number;
  lt: string;
  lu: string;
  vp: number;
  type: number;
  token: string;
}
