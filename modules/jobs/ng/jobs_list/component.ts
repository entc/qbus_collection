import { Component, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { NgForm } from '@angular/forms';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-jobs-list',
  templateUrl: './component.html',
  providers: [AuthService]
})

export class JobsListComponent implements OnInit {

  job_list: Observable<Array<JobItem>>;

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //-----------------------------------------------------------------------------

  jobs_get ()
  {
    this.job_list = this.AuthService.json_rpc ('JOBS', 'list_get', {});
  }

  //-----------------------------------------------------------------------------

  jobs_add__modal_open ()
  {
    var modal = this.modalService.open (JobsAddModalComponent, {ariaLabelledBy: 'modal-basic-title'});

    modal.result.then((result) => {

      this.AuthService.json_rpc ('JOBS', 'list_add', {}).subscribe((data: Object) => {

        this.jobs_get ();
      });

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------
}

class JobItem
{
  repeated: boolean;
  event_start: string;
  event_stop: string;
  active: boolean;
  modp: string;
  ref: number;
  data: string;
}

@Component({
  selector: 'jobs-add-modal-component',
  templateUrl: './modal_add.html',
  providers: [AuthService]
}) export class JobsAddModalComponent implements OnInit {

  //-----------------------------------------------------------------------------

  displayMonths = 2;
  navigation = 'select';
  showWeekNumbers = false;
  outsideDays = 'visible';
  start_time: NgbTimeStruct;
  start_date: NgbDateStruct;
  seconds = true;

  //-----------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    var date = new Date();

    this.start_date = {day: date.getUTCDay() + 1, month: date.getUTCMonth()+ 1, year: date.getUTCFullYear()};
    this.start_time = {hour: date.getHours(), minute: date.getMinutes(), second: date.getSeconds()};
  }

  //-----------------------------------------------------------------------------

  add_submit (form: NgForm)
  {
    this.modal.close (form.value);
    form.resetForm ();
  }
}
