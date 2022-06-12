import { Injectable, Component, Directive, AfterViewInit, OnInit, Injector, Input, Output, ComponentFactoryResolver, ViewContainerRef, ViewChild, ElementRef, Type, EventEmitter } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { NgForm } from '@angular/forms';
import { AuthSession } from '@qbus/auth_session';
import { Observable, BehaviorSubject } from 'rxjs';
import { FlowUserFormService, FlowFunctionService, StepFct } from './services';
import { IWorkstep } from './headers';
import { IFlowEditorWidget } from './widgets';
import { QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';

//-----------------------------------------------------------------------------

@Component({
  selector: 'flow-editor-worksteps',
  templateUrl: './component.html',
})
export class FlowWorkstepsComponent implements OnInit
{
  @Input('wfid') wfid: number;

  worksteps = new Array<IWorkstep>();
  public sqtid: number = 1;

  //-----------------------------------------------------------------------------

  constructor(private auth_session: AuthSession, private modal_service: NgbModal, public userform_service: FlowUserFormService, public function_service: FlowFunctionService)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    // load all steps
    this.fetch ();
  }

  //-----------------------------------------------------------------------------

  public on_sqt_change(e: Event)
  {
    const target = e.target as HTMLInputElement;

    if (target && target.value)
    {
      this.sqtid = Number(target.value);
      this.fetch ();
    }
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
    this.auth_session.json_rpc ('FLOW', 'workstep_mv', {wfid : this.wfid, wsid : ws.id, sqid : ws.sqtid, direction : direction}).subscribe(() => {

      this.fetch ();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //-----------------------------------------------------------------------------

  private fetch ()
  {
    this.auth_session.json_rpc ('FLOW', 'workflow_get', {wfid : this.wfid, sqtid : this.sqtid, ordered : true}).subscribe((data: Array<IWorkstep>) => {

      this.worksteps = data;

    });
  }

  //-----------------------------------------------------------------------------

  private workstep_del (ws: IWorkstep)
  {
    this.auth_session.json_rpc ('FLOW', 'workstep_rm', {wfid : this.wfid, wsid : ws.id, sqid : ws.sqtid}).subscribe(() => {

      this.fetch ();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //-----------------------------------------------------------------------------

  modal__workstep_del__open (ws: IWorkstep)
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('MISC.DELETE', 'FLOW.WORKSTEPDELETEINFO', 'MISC.DELETE');

    this.modal_service.open(QbngWarnOptionModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])}).result.then(() => this.workstep_del (ws));
  }

  //-----------------------------------------------------------------------------

  modal__workstep_add__open (modal_content: IWorkstep)
  {
    var workstep_ctx: FlowWorkstepCtx = new FlowWorkstepCtx (this.wfid, this.sqtid);

    this.modal_service.open (FlowWorkstepsAddModalComponent, {ariaLabelledBy: 'modal-basic-title', size: 'lg', injector: Injector.create([{provide: IWorkstep, useValue: modal_content}, {provide: FlowWorkstepCtx, useValue: workstep_ctx}])}).result.then(() => this.fetch ());
  }
}

//-----------------------------------------------------------------------------

export class WidgetItem
{
  constructor (public component: Type<any>) {}
}

//-----------------------------------------------------------------------------

export class FlowWorkstepCtx
{
  constructor (public wfid: number, public sqid: number) {}
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

  protected rebuild_workstep (step: StepFct, workstep_content: BehaviorSubject<IWorkstep>)
  {
    console.log ('update workstep');

    if (this.last_step != step)
    {
      this.last_step = step;

      // clear the current view
      this.view.clear();

      if (step)
      {
        this.build_widget (step, workstep_content);
      }
    }
  }

  //---------------------------------------------------------------------------

  protected build_widget (step: StepFct, workstep_content: BehaviorSubject<IWorkstep>)
  {
    try
    {
      const type = step.type;
      if (type)
      {
        console.log ('create a new widget');

        // this will create an instance of our widget type
        var item: WidgetItem = new WidgetItem(type);

        // create a factory to create a new component
        const factory = this.component_factory_resolver.resolveComponentFactory (item.component);

        // use the factory to create the new component
        const compontent = this.view.createComponent(factory);

        // assign the instance to the widget variable
        this.widget = compontent.instance;
        this.widget.workstep_content = workstep_content;
      }
    }
    catch (e)
    {
      console.log(e);
    }
  }

}

//=============================================================================

@Directive({
  selector: 'flow-widget-function-component'
}) export class FlowWidgetFunctionComponent extends WidgetComponent implements OnInit  {

  @Input() workstep_content: BehaviorSubject<IWorkstep>;

  //---------------------------------------------------------------------------

  constructor (private function_service: FlowFunctionService, view: ViewContainerRef, component_factory_resolver: ComponentFactoryResolver)
  {
    super (view, component_factory_resolver);
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.workstep_content.subscribe ((workstep: IWorkstep) => {

      // rebuild the widget with a different workstep widget
      this.rebuild_workstep (this.function_service.get_val (workstep.fctid), this.workstep_content);

    });
  }

  //---------------------------------------------------------------------------
}

//=============================================================================

@Directive({
  selector: 'flow-widget-usrform-component'
}) export class FlowWidgetUsrFormComponent extends WidgetComponent implements OnInit {

  @Input() workstep_content: BehaviorSubject<IWorkstep>;

  //---------------------------------------------------------------------------

  constructor (private userform_service: FlowUserFormService, view: ViewContainerRef, component_factory_resolver: ComponentFactoryResolver)
  {
    super (view, component_factory_resolver);
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.workstep_content.subscribe ((workstep: IWorkstep) => {

      // rebuild the widget with a different workstep widget
      this.rebuild_workstep (this.userform_service.get_val (workstep.usrid), this.workstep_content);

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

  constructor (private auth_session: AuthSession, private modal_service: NgbModal, public modal: NgbActiveModal, public modal_content: IWorkstep, public userform_service: FlowUserFormService, public function_service: FlowFunctionService, private component_factory_resolver: ComponentFactoryResolver, private workstep_ctx: FlowWorkstepCtx)
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

  public apply ()
  {
    const workstep: IWorkstep = this.workstep_content.value;

    if (this.modal_content)
    {
      this.auth_session.json_rpc ('FLOW', 'workstep_set', {wfid : this.workstep_ctx.wfid, sqid : this.workstep_ctx.sqid, wsid : this.modal_content.id, name: workstep.name, tag: workstep.tag, fctid: workstep.fctid, pdata: workstep.pdata}).subscribe(() => {

        this.modal.close ();

      }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
    }
    else
    {
      this.auth_session.json_rpc ('FLOW', 'workstep_add', {wfid : this.workstep_ctx.wfid, sqid : this.workstep_ctx.sqid, name: workstep.name, tag: workstep.tag, fctid: workstep.fctid, pdata: workstep.pdata}).subscribe(() => {

        this.modal.close ();

      }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
    }
  }

  //---------------------------------------------------------------------------

  public on_name_change (e: Event)
  {
    const target = e.target as HTMLInputElement;

    if (target && target.value)
    {
      const workstep: IWorkstep = this.workstep_content.value;

      workstep.name = target.value;

      this.workstep_content.next (workstep);
    }
  }

  //---------------------------------------------------------------------------

  public on_tag_change (e: Event)
  {
    const target = e.target as HTMLInputElement;

    if (target && target.value)
    {
      const workstep: IWorkstep = this.workstep_content.value;

      workstep.tag = target.value;

      this.workstep_content.next (workstep);
    }
  }

  //---------------------------------------------------------------------------

  on_pdata_change (e: Event, pdata_name: string)
  {
    const target = e.target as HTMLInputElement;

    if (target && target.value)
    {
      const workstep: IWorkstep = this.workstep_content.value;

      workstep.pdata[pdata_name] = target.value;

      this.workstep_content.next (workstep);
    }
  }
}
