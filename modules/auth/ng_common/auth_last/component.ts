import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';
import { AuthUserContext } from '@qbus/auth_users/component';
import { Observable, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-last',
  templateUrl: './component.html'
}) export class AuthLastComponent {

  @Input() sitem;

  //---------------------------------------------------------------------------

  constructor (public auth_session: AuthSession, private modal_service: NgbModal)
  {
  }

}
