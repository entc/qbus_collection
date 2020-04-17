import { Component, OnInit, SimpleChanges, Input, ViewChild } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { NgForm } from '@angular/forms';
import { Pipe, PipeTransform } from '@angular/core';
import { JsonEditorComponent, JsonEditorOptions } from 'ang-jsoneditor';

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-params',
  templateUrl: './component.html',
  styleUrls: ['./component.scss']
})
export class QbngParamsComponent implements OnInit {

  @Input('params') params: any;
  @ViewChild(JsonEditorComponent, { static: true }) editor: JsonEditorComponent;

  public editorOptions: JsonEditorOptions;

  //-----------------------------------------------------------------------------

  constructor()
  {
    this.editorOptions = new JsonEditorOptions();
    this.editorOptions.modes = ['code', 'text', 'tree', 'view'];
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
