import { Injectable, Component, Directive, AfterViewInit, OnInit, Injector, Input, Output, ComponentFactoryResolver, ViewContainerRef, ViewChild, ElementRef, Type, EventEmitter } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { NgForm } from '@angular/forms';
import { AuthSession } from '@qbus/auth_session';
import { Observable, BehaviorSubject } from 'rxjs';
import { FlowUserFormService, FlowFunctionService, StepFct } from './services';
import { IWorkstep } from './headers';
import { IFlowEditorWidget } from './widgets';

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
  private widget: IFlowEditorWidget = null;
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
