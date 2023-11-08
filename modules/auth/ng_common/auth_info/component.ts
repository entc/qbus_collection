import { Component, Input, OnInit, Injector } from '@angular/core';
import { AuthSession, AuthSessionItem, AuthSessionLoginWidget } from '@qbus/auth_session';
import { AuthUserContext } from '@qbus/auth_users/component';
import { Observable, BehaviorSubject, of } from 'rxjs';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-info',
  templateUrl: './component.html',
  styleUrls: ['./component.css']
}) export class AuthSessionInfoComponent {

  public sitem: AuthSessionItem;
  public mode: number = 1;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private auth_session: AuthSession, private modal_service: NgbModal)
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

  public open_rename ()
  {
    this.modal_service.open (AuthSessionInfoNameModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: AuthSessionItem, useValue: this.sitem}])});
  }

  //---------------------------------------------------------------------------

  public open_change_password ()
  {
    this.modal_service.open (AuthSessionInfoPasswordModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: AuthSessionItem, useValue: this.sitem}])});
  }

  //---------------------------------------------------------------------------

  private disable_account ()
  {

  }

  //---------------------------------------------------------------------------

  public open_disable_account ()
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('AUTH.ACCDEACTIVATE', 'AUTH.ACCDEACTIVATE_INFO', 'AUTH.ACCDEACTIVATE');

    this.modal_service.open (QbngWarnOptionModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])}).result.then(() => this.disable_account (), () => {});
  }
}

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-session-info-modal-component',
  templateUrl: './modal_info.html'
}) export class AuthSessionInfoModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private auth_session: AuthSession)
  {
  }

  //---------------------------------------------------------------------------

  public logout_submit ()
  {
    this.auth_session.disable ();
    this.modal.dismiss ();
  }

};

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-session-info-name-modal-component',
  templateUrl: './modal_name.html'
}) export class AuthSessionInfoNameModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private auth_session: AuthSession, private modal_service: NgbModal, public sitem: AuthSessionItem)
  {
  }

  //---------------------------------------------------------------------------

  public change_name_apply ()
  {
    this.auth_session.json_rpc ('AUTH', 'globperson_set', {firstname : this.sitem.firstname, lastname : this.sitem.lastname}).subscribe(() => {

      this.modal.close();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

};

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-session-info-password-modal-component',
  templateUrl: './modal_password.html'
}) export class AuthSessionInfoPasswordModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private auth_session: AuthSession, public sitem: AuthSessionItem)
  {
  }

};
