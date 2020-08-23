import { Component, EventEmitter, OnInit, SimpleChanges, Input, Output, ViewChild } from '@angular/core';
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

  @Input('model') data_const: any;
  @Output('model') data_change = new EventEmitter();

  public type: number;
  public show: boolean;

  //-----------------------------------------------------------------------------

  constructor ()
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit ()
  {
    this.show = false;
    this.on_type ();
  }

  //-----------------------------------------------------------------------------

  on_type ()
  {
    if (this.data_const == undefined)
    {
      this.type = 1;
    }
    else if (typeof this.data_const === "object")
    {
      if (Object.prototype.toString.call( this.data_const ) === '[object Array]')
      {
        this.type = 3;
      }
      else
      {
        this.type = 2;
      }
    }
    else if (typeof this.data_const === "string")
    {
      this.type = 4;
    }
    else if (typeof this.data_const === "number")
    {
      this.type = 5;
    }
    else if (typeof this.data_const === "boolean")
    {
      this.type = 6;
    }
  }

  //-----------------------------------------------------------------------------

  change_type (type_id: number)
  {
    switch (type_id)
    {
      case 1:
      {
        this.data_const = undefined;
        break;
      }
      case 2:
      {
        this.data_const = {};
        break;
      }
      case 3:
      {
        this.data_const = [];
        break;
      }
      case 4:
      {
        this.data_const = '';
        break;
      }
      case 5:
      {
        this.data_const = 0;
        break;
      }
      case 6:
      {
        this.data_const = false;
        break;
      }
    }

    this.data_change.emit (this.data_const);
    this.on_type ();
  }

  //-----------------------------------------------------------------------------

  toogle_show ()
  {
    this.show = ! this.show;
  }

  //-----------------------------------------------------------------------------

  add_item ()
  {
    switch (this.type)
    {
      case 2:
      {
        this.data_const['new'] = undefined;
        this.show = true;
        this.data_change.emit (this.data_const);
        break;
      }
      case 3:
      {
        this.data_const.push (undefined);
        this.show = true;
        this.data_change.emit (this.data_const);
        break;
      }
    }
  }

  //-----------------------------------------------------------------------------

  change_key (item, value)
  {
    console.log(item);
    console.log(value);
  }

}

//-----------------------------------------------------------------------------

/*
@Component({
  selector: 'qbng-params',
  template:
  `
  <a (click)="selectEvents.emit( node )" class="label">
    {{ node.label }}
  </a>

  <div *ngIf="node.children.length" class="children">

    <ng-template ngFor let-child [ngForOf]="node.children">

      <my-tree-node
        [node]="child"
        [selectedNode]="selectedNode"
        (select)="selectEvents.emit( $event )">
      </my-tree-node>

    </ng-template>

  </div>

  `
})
export class QbngParamsNodeComponent implements OnInit {

  @Input('data') data_const: any;
  @Output('data') data_change = new EventEmitter();

  //-----------------------------------------------------------------------------

  constructor()
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
  }

}
*/
