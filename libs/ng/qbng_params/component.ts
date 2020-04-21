import { Component, OnInit, SimpleChanges, Input, ViewChild } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { NgForm } from '@angular/forms';
import { Pipe, PipeTransform } from '@angular/core';

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-params',
  templateUrl: './component.html'
})
export class QbngParamsComponent implements OnInit {

  @Input('params') params: any;

  //-----------------------------------------------------------------------------

  constructor()
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    console.log('B');
    console.log(this.params);
    console.log('C');
  }

  //-----------------------------------------------------------------------------

  ngOnChanges (changes: SimpleChanges)
  {

    console.log(changes);

  }

  //-----------------------------------------------------------------------------

  param_add()
  {
    this.params.push ({key : 'key_name', val : 'value'});
  }

  //-----------------------------------------------------------------------------

  param_rm (index: number)
  {
    this.params.splice (index, 1);
  }

  //-----------------------------------------------------------------------------

  get_data (data: any)
  {

  }

}
