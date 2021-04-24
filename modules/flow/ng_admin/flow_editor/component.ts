import { Component, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable, of } from 'rxjs';
import { NgForm } from '@angular/forms';
import { ActivatedRoute, Router } from '@angular/router';
import { AuthSession } from '@qbus/auth_session';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-editor',
  templateUrl: './component.html'
})
export class FlowEditorComponent implements OnInit {

  workflows: Observable<IWorkflow[]>;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private modalService: NgbModal, private router: Router, private route: ActivatedRoute)
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

  modal__workflow_add__open ()
  {
    this.modalService.open(FlowEditorAddModalComponent, {ariaLabelledBy: 'modal-basic-title'}).result.then((result) => {

      if (result)
      {
        this.auth_session.json_rpc ('FLOW', 'workflow_add', {'name' : result.workflow_name}).subscribe(() => {

          this.workflow_get ();

        });
      }

    }, () => {

    });
  }

//-----------------------------------------------------------------------------

  modal__workflow_del__open (wf: IWorkflow)
  {
    this.modalService.open (FlowEditorRmModalComponent, {ariaLabelledBy: 'modal-basic-title'}).result.then((result) => {

      this.auth_session.json_rpc ('FLOW', 'workflow_rm', {'wfid' : wf.id}).subscribe(() => {

        this.workflow_get ();

      });

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  modal__process_add (wfid: number)
  {
    this.auth_session.json_rpc ('SASA', 'flow_test', {'wfid' : wfid, 'sqtid' : 1}).subscribe(() => {



    });
  }

  //-----------------------------------------------------------------------------

  edit_details (wf: IWorkflow)
  {
    this.router.navigate(['../flow_editor', wf.id], {relativeTo: this.route});
  }

//-----------------------------------------------------------------------------

}

class IWorkflow {

  public id: number;
  public cnt: number;
  public name: string;

}

//=============================================================================

@Component({
  selector: 'flow-add-modal-component',
  templateUrl: './modal_add.html'
}) export class FlowEditorAddModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  add_submit (form: NgForm)
  {
    this.modal.close (form.value);
    form.resetForm ();
  }

}

//=============================================================================

@Component({
  selector: 'flow-rm-modal-component',
  templateUrl: './modal_rm.html'
}) export class FlowEditorRmModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

}

//=============================================================================
