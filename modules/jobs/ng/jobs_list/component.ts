import { Component, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { NgForm } from '@angular/forms';
import { AuthSession } from '@qbus/auth_session';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-jobs-list',
  templateUrl: './component.html'
})

export class JobsListComponent implements OnInit {

  job_list: Observable<Array<JobItem>>;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modalService: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.jobs_get ();
  }

  //-----------------------------------------------------------------------------

  jobs_get ()
  {
    this.job_list = this.auth_session.json_rpc ('JOBS', 'list_get', {});
  }

  //-----------------------------------------------------------------------------

  jobs_add__modal_open ()
  {
    var modal = this.modalService.open (JobsAddModalComponent, {ariaLabelledBy: 'modal-basic-title'});

    modal.result.then((result) => {

      // convert from boostrap to default javscript Date object
      var d: Date = new Date (result.start_date.year, result.start_date.month - 1, result.start_date.day, result.start_time.hour, result.start_time.minute, result.start_time.second, 0);

      this.auth_session.json_rpc ('JOBS', 'list_add', {'event_start': d.toISOString()}).subscribe((data: Object) => {

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
  templateUrl: './modal_add.html'
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

console.log(date);

    this.start_date = {day: date.getUTCDate(), month: date.getUTCMonth()+ 1, year: date.getUTCFullYear()};
    this.start_time = {hour: date.getHours(), minute: date.getMinutes(), second: date.getSeconds()};
  }

  //-----------------------------------------------------------------------------

  add_submit (form: NgForm)
  {
    this.modal.close (form.value);
    form.resetForm ();
  }
}
