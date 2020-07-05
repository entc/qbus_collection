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
