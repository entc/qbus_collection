import { Component, OnInit, Injector } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { ActivatedRoute, Params } from '@angular/router';
import { NgForm } from '@angular/forms';

// auth service
import { AuthService } from '@qbus/auth.service';

export class StepFunctions {
   public static STEP_FUNCTIONS = [
     {id: 3, name: "call module's method (syncron)"},
     {id: 4, name: "call module's method (asyncron)"},
     {id: 5, name: "wait for list"},
     {id: 10, name: "split flow"},
     {id: 11, name: "start flow"},
     {id: 12, name: "switch"},
     {id: 13, name: "if"},
     {id: 21, name: "place message"},
     {id: 50, name: "(variable) copy"},
     {id: 51, name: "(variable) create node"},
     {id: 52, name: "(variable) move"}
   ];
}

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-worksteps',
  templateUrl: './component.html',
})

export class FlowWorkstepsComponent implements OnInit
{
  public wfid: number;

  worksteps = new Array<IWorkstep>();

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    // get the workflow id from the url path
    this.wfid = Number(this.route.snapshot.paramMap.get('wfid'));

    // load all steps
    this.workflow_get ();
  }

  //-----------------------------------------------------------------------------

  function_name_get (fctid: number)
  {
    for (var i in StepFunctions.STEP_FUNCTIONS)
    {
      var fct = StepFunctions.STEP_FUNCTIONS[i];
      if (fct.id == fctid)
      {
        return fct.name;
      }
    }

    return "[unknown]";
  }

  //-----------------------------------------------------------------------------

  workstep_mv (ws: IWorkstep, direction: number)
  {
    this.AuthService.json_rpc ('FLOW', 'workstep_mv', {'wfid' : this.wfid, 'wsid' : ws.id, 'sqid' : ws.sqtid, 'direction' : direction}).subscribe(() => {

      this.workflow_get ();
    });
  }

  //-----------------------------------------------------------------------------

  workflow_get ()
  {
    this.AuthService.json_rpc ('FLOW', 'workflow_get', {'wfid' : this.wfid, 'ordered' : true}).subscribe((data: Array<IWorkstep>) => {

      this.worksteps = data;

    });
  }

  //-----------------------------------------------------------------------------

  modal__workstep_del__open (ws: IWorkstep)
  {
    this.modalService.open (FlowWorkstepsRmModalComponent, {ariaLabelledBy: 'modal-basic-title'}).result.then(() => {

      this.AuthService.json_rpc ('FLOW', 'workstep_rm', {'wfid' : this.wfid, 'wsid' : ws.id, 'sqid' : ws.sqtid}).subscribe(() => {

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

      this.AuthService.json_rpc ('FLOW', flow_method, {wfid : this.wfid, wsid : modal_content ? modal_content.id : undefined, sqid : 1, name: step_name, fctid: step_fctid, pdata: result.pdata}).subscribe(() => {

        this.workflow_get ();
      });

    }, () => {

    });
  }

}

//-----------------------------------------------------------------------------

export class IWorkstep {

  public id: number;
  public sqtid: number;
  public fctid: number;
  public name: string;
  public pdata = {};

}

//=============================================================================

@Component({
  selector: 'flow-worksteps-add-modal-component',
  templateUrl: './modal_add.html'
}) export class FlowWorkstepsAddModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  public modal_step_content: IWorkstep;
  public step_functions = StepFunctions.STEP_FUNCTIONS;

  public submit_name: string;

  constructor (public modal: NgbActiveModal, public modal_content: IWorkstep)
  {
    if (modal_content)
    {
      this.modal_step_content = modal_content;

      if (!this.modal_step_content.pdata)
      {
        this.modal_step_content.pdata = {};
      }

      this.submit_name = 'MISC.APPLY';
    }
    else
    {
      this.modal_step_content = new IWorkstep;
      this.submit_name = 'MISC.ADD';
    }
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  add_submit (form: NgForm)
  {
    this.modal.close (this.modal_step_content);
    form.resetForm ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.modal_step_content.pdata[name] = data;
  }


}

//=============================================================================

@Component({
  selector: 'flow-worksteps-rm-modal-component',
  templateUrl: './modal_rm.html'
}) export class FlowWorkstepsRmModalComponent implements OnInit {

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

}
