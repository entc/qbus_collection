import { Component, OnInit, Output, EventEmitter } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable, of } from 'rxjs';
import { NgForm } from '@angular/forms';
import { ActivatedRoute, Router } from '@angular/router';
import { AuthSession } from '@qbus/auth_session';
import { QbngErrorModalComponent, QbngWarnOptionModalComponent } from '@qbus/qbng_modals/component';
import { QbngErrorHolder, QbngOptionHolder } from '@qbus/qbng_modals/header';

//-----------------------------------------------------------------------------

@Component({
  selector: 'flow-workflow-selector',
  templateUrl: './component.html'
})
export class FlowWorkflowSelectorComponent implements OnInit {

  @Output() onChange = new EventEmitter();

  workflows: Observable<IWorkflow[]>;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modal_service: NgbModal, private router: Router, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.fetch ();
  }

  //-----------------------------------------------------------------------------

  fetch ()
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

  public select_workflow (item)
  {
    this.onChange.emit (item.id);
  }

//-----------------------------------------------------------------------------

}

//=============================================================================

export class IWorkflow {

  public id: number;
  public cnt: number;
  public name: string;

}

//=============================================================================

@Component({
  selector: 'flow-workflow-selector-modal-component',
  templateUrl: './modal_selector.html'
}) export class FlowWorkflowSelectorModalComponent {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  public on_apply (wfid: number)
  {
    this.modal.close (wfid);
  }

}

//=============================================================================
