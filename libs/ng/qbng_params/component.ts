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

  @Input() model: any;
  @Output() modelChange = new EventEmitter();

  @Input() name: string;
  @Output() nameChange = new EventEmitter<string>();
  public nameUsed = false;

  public type: number;
  public show: boolean;

  public data_map: MapItem[];

  //-----------------------------------------------------------------------------

  constructor ()
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit ()
  {
    this.nameUsed = this.nameChange.observers.length > 0;

    this.show = false;
    this.on_type ();
  }

  //-----------------------------------------------------------------------------

  on_type ()
  {
    if (this.model == undefined)
    {
      this.type = 1;
    }
    else if (typeof this.model === "object")
    {
      if (Object.prototype.toString.call( this.model ) === '[object Array]')
      {
        this.type = 3;
      }
      else
      {
        this.type = 2;

        // convert into list
        this.data_map = [];

        for (var i in this.model)
        {
          this.data_map.push ({key: i, val: this.model[i]});
        }
      }
    }
    else if (typeof this.model === "string")
    {
      this.type = 4;
    }
    else if (typeof this.model === "number")
    {
      this.type = 5;
    }
    else if (typeof this.model === "boolean")
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
        this.model = undefined;
        break;
      }
      case 2:
      {
        this.model = {};
        break;
      }
      case 3:
      {
        this.model = [];
        break;
      }
      case 4:
      {
        this.model = '';
        break;
      }
      case 5:
      {
        this.model = 0;
        break;
      }
      case 6:
      {
        this.model = false;
        break;
      }
    }

    this.modelChange.emit (this.model);
    this.on_type ();
  }

  //-----------------------------------------------------------------------------

  toogle_show ()
  {
    this.show = ! this.show;
  }

  //-----------------------------------------------------------------------------

  build_map ()
  {
    // rebuild the map
    // convert into list
    this.model = {};

    for (var i in this.data_map)
    {
      const data = this.data_map[i];
      this.model [data.key] = data.val;
    }

    this.modelChange.emit (this.model);
  }

  //-----------------------------------------------------------------------------

  add_item ()
  {
    switch (this.type)
    {
      case 2:
      {
        this.show = true;
        this.data_map.push ({key: 'new', val: undefined});
        this.build_map ();
        break;
      }
      case 3:
      {
        this.show = true;
        this.model.push (undefined);
        this.modelChange.emit (this.model);
        break;
      }
    }
  }

  //-----------------------------------------------------------------------------

  on_model_changed (value)
  {
    this.model = value;
    this.modelChange.emit (this.model);
  }

  //-----------------------------------------------------------------------------

  on_name_changed (item, value)
  {
    item.key = value;
    this.build_map ();
  }

  //-----------------------------------------------------------------------------

  on_val_changed (item, value)
  {
    item.val = value;

    console.log(value);
    this.build_map ();
  }

}

//-----------------------------------------------------------------------------

class MapItem
{
  key: string;
  val: any;
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
