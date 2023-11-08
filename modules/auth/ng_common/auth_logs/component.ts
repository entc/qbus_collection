import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';
import { AuthUserContext } from '@qbus/auth_users/component';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-logs',
  templateUrl: './component.html'
}) export class AuthLogsComponent implements OnInit {

  public log_list: AuthLogsItem[];
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
        this.log_list = [];
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
    this.auth_session.json_rpc ('AUTH', 'ui_login_logs', {}).subscribe ((data: AuthLogsItem[]) => {

      this.log_list = data;

    }, (err: QbngErrorHolder) => {

      this.log_list = [];

    });
  }

  //---------------------------------------------------------------------------
}

class AuthLogsItem
{
  id: number;
  ltime: string;
  ip: string;
  info: object;
}
