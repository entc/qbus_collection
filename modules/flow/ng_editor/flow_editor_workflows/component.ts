import { Component, OnInit, Injector } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable, of } from 'rxjs';
import { NgForm } from '@angular/forms';
import { ActivatedRoute, Router } from '@angular/router';
import { AuthSession } from '@qbus/auth_session';
import { QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-editor',
  templateUrl: './component.html'
})
export class FlowEditorComponent implements OnInit {

  public workflows: Observable<IWorkflow[]>;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal, private router: Router, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.workflow_get ();
  }

  //-----------------------------------------------------------------------------

  workflow_get ()
  {
    this.auth_session.session.subscribe ((data) => {

      if (data)
      {
        this.workflows = this.auth_session.json_rpc ('FLOW', 'workflow_get', {});
      }
      else
      {
        this.workflows = of();
      }

    });
  }

  //-----------------------------------------------------------------------------

  public modal__workflow_add__open ()
  {
    this.modal_service.open(FlowEditorAddModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: IWorkflow, useValue: null}])}).result.then(() => {

      this.workflow_get ();

    });
  }

  //---------------------------------------------------------------------------

  public modal__workflow_perm__open (wf: IWorkflow)
  {
    this.modal_service.open (FlowEditorPermModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: IWorkflow, useValue: wf}])}).result.then(() => {

    }, () => {

    });
  }

  //---------------------------------------------------------------------------

  public workflow_del (wf: IWorkflow)
  {
    this.auth_session.json_rpc ('FLOW', 'workflow_rm', {'wfid' : wf.id}).subscribe(() => {

      this.workflow_get ();

    }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
  }

  //---------------------------------------------------------------------------

  public modal__workflow_del__open (wf: IWorkflow)
  {
    var holder: QbngOptionHolder = new QbngOptionHolder ('MISC.DELETE', 'FLOW.WORKFLOWDELETE', 'MISC.DELETE');

    this.modal_service.open(QbngWarnOptionModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngOptionHolder, useValue: holder}])}).result.then(() => {

      this.workflow_del (wf);

    });
  }

  //-----------------------------------------------------------------------------

  public edit_details (wf: IWorkflow)
  {
    this.router.navigate(['../flow_editor', wf.id], {relativeTo: this.route});
  }

  //-----------------------------------------------------------------------------

  public workflow_edit (item: IWorkflow)
  {
    this.modal_service.open (FlowEditorAddModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: IWorkflow, useValue: item}])}).result.then(() => {

      this.workflow_get ();

    });
  }

  //-----------------------------------------------------------------------------

  public workstep_sequence_name (sqtid: number): string
  {
    switch (sqtid)
    {
      case 1: return 'FLOW.SQT_DEFAULT';
      case 2: return 'FLOW.SQT_ABORT';
      case 3: return 'FLOW.SQT_FAILOVER';
      default: return 'FLOW.SQT_USER';
    }
  }

//-----------------------------------------------------------------------------

}

//=============================================================================

export class IWorkflow {

  public id: number;
  public steps: FlowWorkflowStep[];
  public name: string;

}

class FlowWorkflowStep {
  sqtid: number;
  steps: number;
}

//=============================================================================

@Component({
  selector: 'flow-add-modal-component',
  templateUrl: './modal_add.html'
}) export class FlowEditorAddModalComponent implements OnInit {

  public name: string;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal, public modal: NgbActiveModal, private item: IWorkflow)
  {
    if (this.item)
    {
      this.name = item.name;
    }
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  public apply ()
  {
    if (this.item)
    {
      this.auth_session.json_rpc ('FLOW', 'workflow_set', {wfid: this.item.id, name : this.name}).subscribe(() => {

        this.modal.close ();

      }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
    }
    else
    {
      this.auth_session.json_rpc ('FLOW', 'workflow_add', {name : this.name}).subscribe(() => {

        this.modal.close ();

      }, (err: QbngErrorHolder) => this.modal_service.open (QbngErrorModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: QbngErrorHolder, useValue: err}])}));
    }
  }

}

//=============================================================================

@Component({
  selector: 'flow-perm-modal-component',
  templateUrl: './modal_perm.html'
}) export class FlowEditorPermModalComponent implements OnInit {

  public workspace_list: WorkspaceItem[];
  public permission_list: Observable<FlowPermissionItem[]>;
  public wpid: number = 0;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, public modal: NgbActiveModal, private wf: IWorkflow)
  {
  }

  //---------------------------------------------------------------------------

  private fetch ()
  {
    this.permission_list = this.auth_session.json_rpc ('FLOW', 'workflow_perm_get', {wfid: this.wf.id});
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.auth_session.json_rpc ('AUTH', 'workspaces_get', {}).subscribe ((data: WorkspaceItem[]) => {

      this.workspace_list = data;
      this.fetch ();
    });
  }

  //---------------------------------------------------------------------------

  public get_workspace_name (wpid: number)
  {
    const wp: WorkspaceItem = this.workspace_list.find ((e: WorkspaceItem) => e.id == wpid);
    if (wp)
    {
      return wp.name;
    }
    else
    {
      return '[unknown workspace]';
    }
  }

  //-----------------------------------------------------------------------------

  public workspace_select (e: Event)
  {
    const target = e.target as HTMLSelectElement;

    if (target && target.value)
    {
      this.wpid = Number(target.value);
    }
  }

  //-----------------------------------------------------------------------------

  public workspace_add ()
  {
    this.auth_session.json_rpc ('FLOW', 'workflow_perm_set', {wfid: this.wf.id, wpid: this.wpid, perm: true}).subscribe (() => {

      this.fetch ();

    });
  }

  //-----------------------------------------------------------------------------

  public workspace_rm (item: FlowPermissionItem)
  {
    this.auth_session.json_rpc ('FLOW', 'workflow_perm_set', {wfid: this.wf.id, wpid: item.wpid, perm: false}).subscribe (() => {

      this.fetch ();

    });
  }

  //-----------------------------------------------------------------------------

}

//-----------------------------------------------------------------------------

class WorkspaceItem
{
  id: number;
  name: string;
}

class FlowPermissionItem
{
  id: number;
  wpid: number;
}

//=============================================================================
