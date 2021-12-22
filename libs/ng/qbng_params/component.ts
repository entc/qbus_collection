import { Component, EventEmitter, OnInit, SimpleChanges, Input, Output, ViewChild } from '@angular/core';
import { Observable } from 'rxjs';
import { Pipe, PipeTransform } from '@angular/core';

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-params',
  templateUrl: './component.html'
})
export class QbngParamsComponent implements OnInit {

  // those variables control the underlaying data structure
  @Input() model: any;
  @Output() modelChange = new EventEmitter();

  // (optional) this controls the name of the structure
  @Input() name: string;
  @Output() nameChange = new EventEmitter<string>();
  public nameUsed = false;

  @Output() remove = new EventEmitter();

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
  }

  //---------------------------------------------------------------------------

  ngOnChanges (changes: SimpleChanges): void
  {
    console.log('QBNG PARAMS: params change from input');

    if(changes['model'])
    {
      this.on_type (this.model);
    }
  }

  //-----------------------------------------------------------------------------

  remove_item ()
  {
    this.remove.emit (true);
  }

  //-----------------------------------------------------------------------------

  on_remove (item: MapItem)
  {
    switch (this.type)
    {
      case 2:
      {
        this.data_map.splice (this.data_map.findIndex (element => element.key == item.key), 1);
        this.build_map ();
        break;
      }
      case 3:
      {
        this.data_map.splice (item.index, 1);
        this.build_list ();
        break;
      }
    }
  }

  //-----------------------------------------------------------------------------

  on_type (model: any)
  {
    if (model == undefined || model == null)
    {
      this.type = 1;
    }
    else if (typeof model === "object")
    {
      if (Object.prototype.toString.call( model ) === '[object Array]')
      {
        this.type = 3;

        // convert into list
        this.data_map = [];

        for (var i in model)
        {
          this.data_map.push ({key: '', index: Number(i), val: model[i]});
        }
      }
      else
      {
        this.type = 2;

        // convert into list
        this.data_map = [];

        for (var i in model)
        {
          this.data_map.push ({key: i, index: 0, val: model[i]});
        }
      }
    }
    else if (typeof model === "string")
    {
      this.type = 4;
    }
    else if (typeof model === "number")
    {
      this.type = 5;
    }
    else if (typeof model === "boolean")
    {
      this.type = 6;
    }
  }

  //-----------------------------------------------------------------------------

  change_type (type_id: number)
  {
    this.show = false;

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
    this.on_type (this.model);
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

  build_list ()
  {
    // rebuild the map
    // convert into list
    this.model = [];

    for (var i in this.data_map)
    {
      const data = this.data_map[i];
      this.model.push(data.val);
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
        this.data_map.push ({key: 'new', index: 0, val: undefined});
        this.build_map ();
        break;
      }
      case 3:
      {
        this.show = true;
        this.data_map.push ({key: '', index: this.data_map.length, val: undefined});
        this.build_list ();
        break;
      }
    }
  }

  //-----------------------------------------------------------------------------

  on_model_changed (value)
  {
    switch (this.type)
    {
      case 4:
      {
        this.model = String(value);
        break;
      }
      case 5:
      {
        this.model = Number(value);
        break;
      }
      case 6:
      {
        this.model = (value === 'true');
        break;
      }
      default:
      {
        this.model = value;
      }
    }

    this.modelChange.emit (this.model);
  }

  //-----------------------------------------------------------------------------

  on_name_changed (item: MapItem, value: string)
  {
    item.key = value;
    this.build_map ();
  }

  //-----------------------------------------------------------------------------

  on_map_changed (item: MapItem, value)
  {
    item.val = value;
    this.build_map ();
  }

  //-----------------------------------------------------------------------------

  on_list_changed (item: MapItem, value)
  {
    item.val = value;
    this.build_list ();
  }
}

//-----------------------------------------------------------------------------

class MapItem
{
  key: string;
  val: any;
  index: number;
}

//-----------------------------------------------------------------------------
