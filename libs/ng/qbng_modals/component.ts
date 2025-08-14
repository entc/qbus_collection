import { Component, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { QbngErrorHolder, QbngOptionHolder } from './header';

//=============================================================================

@Component({
  selector: 'qbng-spinner-modal-component',
  templateUrl: './modal_spinner.html'
}) export class QbngSpinnerModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }
}

//=============================================================================

@Component({
  selector: 'qbng-spinner-ok-modal-component',
  templateUrl: './modal_spinner_ok.html'
}) export class QbngSpinnerOkModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }
}

//=============================================================================

@Component({
  selector: 'qbng-success-modal-component',
  templateUrl: './modal_success.html'
}) export class QbngSuccessModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, public option: QbngOptionHolder)
  {
  }

}

//=============================================================================

@Component({
  selector: 'qbng-error-modal-component',
  templateUrl: './modal_error.html'
}) export class QbngErrorModalComponent implements OnInit {

  public err_code_name: string = 'ERR.CODE_UNKNOWN';
  public err_text_list: string[] = [];

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, public err: QbngErrorHolder)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    try
    {
      var json_obj = JSON.parse(this.err.text);

      if (Array.isArray (json_obj))
      {
        this.err_text_list = json_obj;
      }
      else
      {
        this.err_text_list = [this.err.text];
      }
    }
    catch (e)
    {
      this.err_text_list = [this.err.text];
    }

    switch (this.err.code)
    {
      case  2: this.err_code_name = 'ERR.CODE_NOT_FOUND'; break;
      case  3: this.err_code_name = 'ERR.CODE_NOT_SUPPORTED'; break;
      case  4: this.err_code_name = 'ERR.CODE_RUNTIME'; break;
      case  9: this.err_code_name = 'ERR.CODE_NO_OBJECT'; break;
      case 10: this.err_code_name = 'ERR.CODE_NO_ROLE'; break;
      case 13: this.err_code_name = 'ERR.CODE_MISSING_PARAM'; break;
      case 22: this.err_code_name = 'ERR.CODE_OUT_OF_BOUNDS'; break;
    }
  }

}

//=============================================================================

@Component({
  selector: 'qbng-warn-option-modal-component',
  templateUrl: './modal_warn_option.html'
}) export class QbngWarnOptionModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, public option: QbngOptionHolder)
  {
  }

}

//=============================================================================

@Component({
  selector: 'qbng-info-option-modal-component',
  templateUrl: './modal_info_option.html'
}) export class QbngInfoOptionModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, public option: QbngOptionHolder)
  {
  }

}
