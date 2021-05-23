import { Component, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder } from './header';

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
