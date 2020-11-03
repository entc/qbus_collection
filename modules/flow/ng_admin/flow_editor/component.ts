import { Component, OnInit } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { NgForm } from '@angular/forms';
import { ActivatedRoute, Router } from '@angular/router';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-editor',
  templateUrl: './component.html'
})
export class FlowEditorComponent implements OnInit {

  workflows: Observable<IWorkflow[]>;

  //-----------------------------------------------------------------------------

  constructor (private AuthService: AuthService, private modalService: NgbModal, private router: Router, private route: ActivatedRoute)
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
    this.AuthService.auth_credentials.subscribe (() => {

      this.workflows = this.AuthService.json_rpc ('FLOW', 'workflow_get', {});

    });
  }

  //-----------------------------------------------------------------------------

  modal__workflow_add__open ()
  {
    this.modalService.open(FlowEditorAddModalComponent, {ariaLabelledBy: 'modal-basic-title'}).result.then((result) => {

      if (result)
      {
        this.AuthService.json_rpc ('FLOW', 'workflow_add', {'name' : result.workflow_name}).subscribe(() => {

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

      this.AuthService.json_rpc ('FLOW', 'workflow_rm', {'wfid' : wf.id}).subscribe(() => {

        this.workflow_get ();

      });

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  modal__process_add (wfid: number)
  {
    this.AuthService.json_rpc ('SASA', 'flow_test', {'wfid' : wfid, 'sqtid' : 1}).subscribe(() => {



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

  constructor (public modal: NgbActiveModal, private AuthService: AuthService)
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

  constructor (public modal: NgbActiveModal, private AuthService: AuthService)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

}

//=============================================================================
