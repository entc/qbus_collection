import { Component, OnInit, SimpleChanges, Input } from '@angular/core';
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

  @Input('params') params: Array<object>;

  //-----------------------------------------------------------------------------

  constructor()
  {
    this.params = [];
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
}

//-----------------------------------------------------------------------------

@Pipe({
  name: "qbngSort"
})
export class QbngSortPipe implements PipeTransform {
  transform(items: any[], field: string, reverse: boolean = false): any[] {
    if (!items) return [];

    if (field) items.sort((a, b) => (a[field] > b[field] ? 1 : -1));
    else items.sort((a, b) => (a > b ? 1 : -1));

    if (reverse) items.reverse();

    return items;
  }
}

//-----------------------------------------------------------------------------
