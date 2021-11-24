import { Injectable, Component, Directive, AfterViewInit, OnInit, Injector, Input, Output, ComponentFactoryResolver, ViewContainerRef, ViewChild, ElementRef, Type, EventEmitter } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { NgForm } from '@angular/forms';
import { AuthSession } from '@qbus/auth_session';
import { Observable, BehaviorSubject } from 'rxjs';

//-----------------------------------------------------------------------------

export class IWorkstep {

  public id: number;
  public sqtid: number;
  public fctid: number;
  public usrid: number;
  public name: string;
  public pdata = {};

}

//-----------------------------------------------------------------------------

export abstract class FlowEditorWidget
{
  protected event_emitter: EventEmitter<IWorkstep> = undefined;
  public content: IWorkstep;

  //---------------------------------------------------------------------------

  constructor ()
  {
  }

  //---------------------------------------------------------------------------

  public set content_setter (val: IWorkstep)
  {
    this.on_content_change (val);
  }

  //---------------------------------------------------------------------------

  public set emitter (emitter: EventEmitter<IWorkstep>)
  {
    this.event_emitter = emitter;
  }

  //---------------------------------------------------------------------------

  protected emit (content: IWorkstep)
  {
    if (this.event_emitter)
    {
      this.event_emitter.emit (content);
    }
    else
    {
      console.log('no emitter was set');
    }
  }

  //---------------------------------------------------------------------------

  protected on_content_change (val: IWorkstep)
  {
    this.content = val;
  }
}

//-----------------------------------------------------------------------------

@Injectable()
export class FlowUserFormService
{
  public values: StepFct[] = [];

  //-----------------------------------------------------------------------------

  constructor ()
  {
    this.values = [];
  }

  //-----------------------------------------------------------------------------

  add (item: StepFct)
  {
    this.values.push (item);
  }

  //-----------------------------------------------------------------------------

  public userform_name_get (usrid: number): string
  {
    if (usrid)
    {
      const found: StepFct = this.values.find ((element: StepFct) => element.id == usrid);

      return found ? found.name : '[unknown]';
    }
    else
    {
      return 'no form';
    }
  }

  //-----------------------------------------------------------------------------

  get_usrid (index: number): number
  {
    try
    {
      return this.values[index].id;
    }
    catch (e)
    {
      return 0;
    }
  }

  //-----------------------------------------------------------------------------

  public get_val (usrid: number): StepFct
  {
    return this.values.find ((element: StepFct) => element.id == usrid);
  }

}

//-----------------------------------------------------------------------------

@Injectable()
export class FlowFunctionService
{
  public values: StepFct[] = [];

  //-----------------------------------------------------------------------------

  constructor ()
  {
    // define widgets to all background functions
    this.values = [
      {id: 3, name: "call module's method (syncron)", desc: '', type: FlowWidgetSyncronComponent},
      {id: 4, name: "call module's method (asyncron)", desc: '', type: FlowWidgetAsyncronComponent},
      {id: 5, name: "wait for list", desc: 'this step waits until a set was sent to each variable within the wait list node. a code can be specified for security reasons.', type: FlowWidgetWaitforlistComponent},
      {id: 10, name: "split flow", desc: '', type: FlowWidgetSplitComponent},
      {id: 11, name: "start flow", desc: '', type: undefined},
      {id: 12, name: "switch", desc: '', type: FlowWidgetSwitchComponent},
      {id: 13, name: "if", desc: 'checks if a variable exists. if there is an additional params node given, it checks inside this node', type: FlowWidgetIfComponent},
      {id: 21, name: "place message", desc: '', type: undefined},
      {id: 50, name: "(variable) copy", desc: '', type: FlowWidgetCopyComponent},
      {id: 51, name: "(variable) create node", desc: '', type: FlowWidgetCreateNodeComponent},
      {id: 52, name: "(variable) move", desc: 'moves a variable to a new destination and name', type: FlowWidgetMoveComponent}
    ];
  }

  //-----------------------------------------------------------------------------

  add (item: StepFct)
  {
    this.values.push (item);
  }

  //-----------------------------------------------------------------------------

  public get_val (fctid: number): StepFct
  {
    return this.values.find ((element: StepFct) => element.id == fctid);
  }

  //-----------------------------------------------------------------------------

  public function_name_get (fctid: number): string
  {
    const found: StepFct = this.get_val (fctid);

    return found ? found.name : '[unknown]';
  }

  //-----------------------------------------------------------------------------

  get_index (fctid: number): number
  {
    return this.values.findIndex((element: StepFct) => element.id == fctid);
  }

  //-----------------------------------------------------------------------------

  get_desc (index: number): string
  {
    try
    {
      return this.values[index].desc;
    }
    catch (e)
    {
      return '';
    }
  }

  //-----------------------------------------------------------------------------

  get_fctid (index: number): number
  {
    try
    {
      return this.values[index].id;
    }
    catch (e)
    {
      return 0;
    }
  }
}

//-----------------------------------------------------------------------------

export class StepFct
{
  id: number;
  name: string;
  desc: string;
  type: any;
}

//-----------------------------------------------------------------------------

@Component({
  selector: 'flow-editor-worksteps',
  templateUrl: './component.html',
})

export class FlowWorkstepsComponent implements OnInit
{
  @Input('wfid') wfid: number;

  worksteps = new Array<IWorkstep>();

  //-----------------------------------------------------------------------------

  constructor(private auth_session: AuthSession, private modalService: NgbModal, public userform_service: FlowUserFormService, public function_service: FlowFunctionService)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    // load all steps
    this.workflow_get ();
  }

  //-----------------------------------------------------------------------------

  function_name_get (fctid: number)
  {
    return this.function_service.function_name_get (fctid);
  }

  //-----------------------------------------------------------------------------

  userform_name_get (usrid: number)
  {
    return this.userform_service.userform_name_get (usrid);
  }

  //-----------------------------------------------------------------------------

  workstep_mv (ws: IWorkstep, direction: number)
  {
    this.auth_session.json_rpc ('FLOW', 'workstep_mv', {'wfid' : this.wfid, 'wsid' : ws.id, 'sqid' : ws.sqtid, 'direction' : direction}).subscribe(() => {

      this.workflow_get ();
    });
  }

  //-----------------------------------------------------------------------------

  workflow_get ()
  {
    this.auth_session.json_rpc ('FLOW', 'workflow_get', {'wfid' : this.wfid, 'ordered' : true}).subscribe((data: Array<IWorkstep>) => {

      this.worksteps = data;

    });
  }

  //-----------------------------------------------------------------------------

  modal__workstep_del__open (ws: IWorkstep)
  {
    this.modalService.open (FlowWorkstepsRmModalComponent, {ariaLabelledBy: 'modal-basic-title'}).result.then(() => {

      this.auth_session.json_rpc ('FLOW', 'workstep_rm', {'wfid' : this.wfid, 'wsid' : ws.id, 'sqid' : ws.sqtid}).subscribe(() => {

        this.workflow_get ();
      });

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  modal__workstep_add__open (modal_content: IWorkstep)
  {
    var flow_method: string = modal_content ? 'workstep_set' : 'workstep_add';

    this.modalService.open (FlowWorkstepsAddModalComponent, {ariaLabelledBy: 'modal-basic-title', size: 'lg', injector: Injector.create([{provide: IWorkstep, useValue: modal_content}])}).result.then((result) => {

      const step_name = result.name;
      const step_fctid = Number (result.fctid);
      const pdata = result.pdata;

      this.auth_session.json_rpc ('FLOW', flow_method, {wfid : this.wfid, wsid : modal_content ? modal_content.id : undefined, sqid : 1, name: step_name, fctid: step_fctid, pdata: pdata}).subscribe(() => {

        this.workflow_get ();
      });

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------
}

export class WidgetItem
{
  constructor (public component: Type<any>) {}
}

//=============================================================================

class WidgetComponent
{
  // this is the current widget object
  private widget: FlowEditorWidget = null;
  private last_step: StepFct = null;

  constructor (protected view: ViewContainerRef, protected component_factory_resolver: ComponentFactoryResolver)
  {
  }

  //---------------------------------------------------------------------------

  protected update_workstep (workstep: IWorkstep, step: StepFct, on_change: EventEmitter<IWorkstep>)
  {
    console.log ('update workstep');

    if (this.last_step != step)
    {
      this.last_step = step;

      // clear the current view
      this.view.clear();

      if (step)
      {
        this.build_widget (step, on_change);
      }
    }

    if (this.widget)
    {
      this.widget.content_setter = workstep;
    }
  }

  //---------------------------------------------------------------------------

  protected build_widget (step: StepFct, on_change: EventEmitter<IWorkstep>)
  {
    try
    {
      const type = step.type;
      if (type)
      {
        // this will create an instance of our widget type
        var item: WidgetItem = new WidgetItem(type);

        // create a factory to create a new component
        const factory = this.component_factory_resolver.resolveComponentFactory (item.component);

        // use the factory to create the new component
        const compontent = this.view.createComponent(factory);

        // assign the instance to the widget variable
        this.widget = compontent.instance;
        this.widget.emitter = on_change;
      }
    }
    catch (e)
    {

    }
  }

}

//=============================================================================

@Directive({
  selector: 'flow-widget-function-component'
}) export class FlowWidgetFunctionComponent extends WidgetComponent implements OnInit  {

  @Input() workstep_content: BehaviorSubject<IWorkstep>;
  @Output() contentChange: EventEmitter<IWorkstep> = new EventEmitter();

  //---------------------------------------------------------------------------

  constructor (private function_service: FlowFunctionService, view: ViewContainerRef, component_factory_resolver: ComponentFactoryResolver)
  {
    super (view, component_factory_resolver);
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.workstep_content.subscribe ((workstep: IWorkstep) => {

      this.update_workstep (workstep, this.function_service.get_val (workstep.fctid), this.contentChange);

    });
  }

  //---------------------------------------------------------------------------
}

//=============================================================================

@Directive({
  selector: 'flow-widget-usrform-component'
}) export class FlowWidgetUsrFormComponent extends WidgetComponent implements OnInit {

  @Input() workstep_content: BehaviorSubject<IWorkstep>;
  @Output() contentChange: EventEmitter<IWorkstep> = new EventEmitter();

  //---------------------------------------------------------------------------

  constructor (private userform_service: FlowUserFormService, view: ViewContainerRef, component_factory_resolver: ComponentFactoryResolver)
  {
    super (view, component_factory_resolver);
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.workstep_content.subscribe ((workstep: IWorkstep) => {

      this.update_workstep (workstep, this.userform_service.get_val (workstep.usrid), this.contentChange);

    });
  }

  //---------------------------------------------------------------------------
}

//=============================================================================

@Component({
  selector: 'flow-worksteps-add-modal-component',
  templateUrl: './modal_add.html'
}) export class FlowWorkstepsAddModalComponent implements AfterViewInit {

  //---------------------------------------------------------------------------

  public workstep_content: BehaviorSubject<IWorkstep>;
  public submit_name: string;

  public show_usr: boolean = false;

  public index_usrid: number = -1;
  public index_fctid: number = -1;

  constructor (public modal: NgbActiveModal, public modal_content: IWorkstep, public userform_service: FlowUserFormService, public function_service: FlowFunctionService, private component_factory_resolver: ComponentFactoryResolver)
  {
    if (modal_content)
    {
      if (!modal_content.pdata)
      {
        modal_content.pdata = {};
      }

      this.workstep_content = new BehaviorSubject<IWorkstep>(modal_content);
      this.submit_name = 'MISC.APPLY';
    }
    else
    {
      this.workstep_content = new BehaviorSubject<IWorkstep>(new IWorkstep);
      this.submit_name = 'MISC.ADD';
    }
  }

  //---------------------------------------------------------------------------

  ngAfterViewInit ()
  {
    const workstep: IWorkstep = this.workstep_content.value;

    // set the dropdown index
    this.on_index_change (this.function_service.get_index (workstep.fctid));
  }

  //---------------------------------------------------------------------------

  on_usrform_change (i: number)
  {
    const workstep: IWorkstep = this.workstep_content.value;

    if (i != -1)
    {
      workstep.usrid = this.userform_service.get_usrid (i);
      this.show_usr = true;
    }
    else
    {
      workstep.usrid = null;
      this.show_usr = false;
    }

    // apply changes
    this.workstep_content.next (workstep);
    this.index_usrid = i;
  }

  //---------------------------------------------------------------------------

  on_index_change (i: number)
  {
    const workstep: IWorkstep = this.workstep_content.value;

    workstep.fctid = this.function_service.get_fctid (i);

    // apply changes
    this.workstep_content.next (workstep);
    this.index_fctid = i;
  }

  //---------------------------------------------------------------------------

  submit ()
  {
    this.modal.close (this.workstep_content.value);
  }

  //---------------------------------------------------------------------------

  on_content_change (workstep: IWorkstep)
  {
console.log('set content');

    this.workstep_content.next (workstep);
  }

  //---------------------------------------------------------------------------

  on_name_change (value: string)
  {
    const workstep: IWorkstep = this.workstep_content.value;

    workstep.name = value;

    this.workstep_content.next (workstep);
  }

  //---------------------------------------------------------------------------

  on_pdata_change (value: string, pdata_name: string)
  {
    const workstep: IWorkstep = this.workstep_content.value;

    workstep.pdata[pdata_name] = value;

    this.workstep_content.next (workstep);
  }
}

//=============================================================================

@Component({
  selector: 'flow-worksteps-rm-modal-component',
  templateUrl: './modal_rm.html'
}) export class FlowWorkstepsRmModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-syncron',
  templateUrl: './widget_call.html'
})
export class FlowWidgetSyncronComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    console.log('emit changes from form');

    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-asyncron',
  templateUrl: './widget_call.html'
})
export class FlowWidgetAsyncronComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-waitforlist',
  templateUrl: './widget_waitforlist.html'
})
export class FlowWidgetWaitforlistComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-split',
  templateUrl: './widget_split.html'
})
export class FlowWidgetSplitComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-switch',
  templateUrl: './widget_switch.html'
})
export class FlowWidgetSwitchComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-if',
  templateUrl: './widget_if.html'
})
export class FlowWidgetIfComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-copy',
  templateUrl: './widget_copy.html'
})
export class FlowWidgetCopyComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-create-node',
  templateUrl: './widget_create_node.html'
})
export class FlowWidgetCreateNodeComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-move',
  templateUrl: './widget_move.html'
})
export class FlowWidgetMoveComponent extends FlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}
