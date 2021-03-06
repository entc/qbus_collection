import { Component, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from './header';

//=============================================================================

@Component({
  selector: 'qbng-success-modal-component',
  templateUrl: './modal_success.html'
}) export class QbngSuccessModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

}

//=============================================================================

@Component({
  selector: 'qbng-error-modal-component',
  templateUrl: './modal_error.html'
}) export class QbngErrorModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, public err: QbngErrorHolder)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

}

//=============================================================================

@Component({
  selector: 'qbng-warn-option-modal-component',
  templateUrl: './modal_warn_option.html'
}) export class QbngWarnOptionModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, public option: QbngOptionHolder)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

}
