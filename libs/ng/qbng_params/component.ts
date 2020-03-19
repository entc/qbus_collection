import { Component, OnInit, SimpleChanges } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { NgForm } from '@angular/forms';

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-params',
  templateUrl: './component.html'
})
export class QbngParamsComponent implements OnInit {

  public params: Array<object>;

  //-----------------------------------------------------------------------------

  constructor()
  {
    this.params = [];
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
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
}
