import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';
import { AuthUserContext } from '@qbus/auth_users/component';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-perm',
  templateUrl: './component.html'
}) export class AuthPermComponent implements OnInit {

  public info: AuthPermInfo;
  public days: number = 1;

  @Input() apid: number;

  private session_obj;

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.session_obj = this.auth_session.session.subscribe ((data) => {

      if (data)
      {
        this.fetch ();
      }
      else
      {
        this.info = null;
      }

    });
  }

  //-----------------------------------------------------------------------------

  ngOnDestroy()
  {
    this.session_obj.unsubscribe();
  }

  //---------------------------------------------------------------------------

  private fetch ()
  {
    this.auth_session.json_rpc ('AUTH', 'token_perm_info', {apid: this.apid}).subscribe((data: AuthPermInfo) => {

      this.info = data;

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------

  public activate ()
  {
    this.auth_session.json_rpc ('AUTH', 'token_perm_put', {apid: this.apid, days: this.days, active: 1}).subscribe(() => {

      this.fetch ();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------
}

class AuthPermInfo
{
  active: number;
  rinfo: object;
  cdata: object;
  toc: string;
  toi: string;
  toe: string;
}
