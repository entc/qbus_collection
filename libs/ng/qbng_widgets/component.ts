import { Component, Input, Output, EventEmitter, Injector, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-switch',
  templateUrl: './widget_switch.html'
})
export class QbngSwitchComponent {

  @Input('val') val: boolean;
  @Output('onVal') onChange = new EventEmitter ();

  //-----------------------------------------------------------------------------

  constructor ()
  {
  }

  //-----------------------------------------------------------------------------

  on_toggle ()
  {
    this.val = !this.val;
    this.onChange.emit (this.val);
  }
}

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-switch-multi',
  templateUrl: './widget_switch_multi.html'
})
export class QbngSwitchMultiComponent implements OnInit {

  @Input('vals') vals: QbngSwitchItem[];
  @Output('onVal') onChange = new EventEmitter ();

  //-----------------------------------------------------------------------------

  constructor ()
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.onChange.emit (this.vals);
  }

  //-----------------------------------------------------------------------------

  switch_changed (item: QbngSwitchItem, switch_value: boolean)
  {
    item.switch_val = switch_value;
    this.onChange.emit (this.vals);
  }
}

export class QbngSwitchItem
{
  name: string;
  switch_val: boolean;
}

//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-upload',
  templateUrl: './widget_upload.html'
})
export class QbngUploadComponent {

  @Input('size_warn') size_warn: number;
  @Input('size_max') size_max: number;
  @Input('mimes') mimes: string[];
  @Output('onFile') onChange = new EventEmitter ();
  @Input('class_names') class_names: string = 'btn btn-primary mb-0';

  public upload_id: string;

  //-----------------------------------------------------------------------------

  constructor (private modal_service: NgbModal)
  {
    this.upload_id = 'upload-' + Math.round(Math.random() * 10000);
  }

  //-----------------------------------------------------------------------------

  open_modal_upload_apply (item: QbngUploadFile)
  {
    this.modal_service.open (QbngUploadModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngUploadFile, useValue: item}])}).result.then(() => this.onChange.emit (item.file));
  }

  //-----------------------------------------------------------------------------

  handle_updload (e: Event)
  {
    const files = e.target['files'];
    if (files.length > 0)
    {
      this.open_modal_upload_apply (new QbngUploadFile (files[0], this.mimes, this.size_warn, this.size_max));
      e.srcElement['value'] = '';
    }
  }
}

//-----------------------------------------------------------------------------

export class QbngUploadFile {

  public file: File;
  public header: string[];

  public size_status: number;
  public type_status: number;

  constructor (file: File, mimes: string[], size_warn: number, size_max: number)
  {
    this.file = file;
    this.type_status = 0;

    if (mimes)
    {
      this.type_status = mimes.find (element => element == file.type) ? 1 : 2;
    }

    this.size_status = 0;

    if (size_warn)
    {
      if (file.size > size_warn)
      {
        this.size_status = 1;
      }
    }

    if (size_max)
    {
      if (file.size > size_max)
      {
        this.size_status = 2;
      }
    }
  }
}

//=============================================================================

@Component({
  selector: 'qbng-upload-modal-component',
  templateUrl: './modal_upload.html'
}) export class QbngUploadModalComponent {

  public file: File;
  public type_status: number;
  public size_status: number;

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private item: QbngUploadFile)
  {
    this.file = item.file;
    this.type_status = item.type_status;
    this.size_status = item.size_status;
  }
}

//=============================================================================

@Component({
  selector: 'qbng-timeperiod-modal-component',
  templateUrl: './modal_timeperiod.html'
}) export class QbngTimePeriodModalComponent implements OnInit {

  public timeperiod: number;
  public timeperiod_in_seconds: number;
  public multiplier_id: number;

  private multiplier: number;
  private mmatrix: number[] = [1, 60, 3600, 86400, 604800];

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.timeperiod = 1;
    this.multiplier_id = 2;
    this.calculate_time_period ();
  }

  //---------------------------------------------------------------------------

  public calculate_time_period ()
  {
    this.timeperiod_in_seconds = this.timeperiod * this.mmatrix[this.multiplier_id];
  }

  //---------------------------------------------------------------------------

  public set_multiplier (id: number)
  {
    this.multiplier_id = id;
    this.calculate_time_period ();
  }

  //---------------------------------------------------------------------------

  public apply ()
  {
    this.modal.close (this.timeperiod_in_seconds);
  }
}
