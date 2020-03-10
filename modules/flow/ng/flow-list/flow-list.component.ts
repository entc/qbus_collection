import { Component, OnInit } from '@angular/core';
import { ActivatedRoute, Params } from '@angular/router';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-list',
  templateUrl: './flow-list.component.html',
  styleUrls: ['./flow-list.component.scss'],
  providers: [AuthService]
})

export class FlowListComponent implements OnInit
{
  private wfid: number;

  modal_step_content: IWorkstep;

  public step_functions = [
    {id: 3, name: "call module's method (syncron)"},
    {id: 4, name: "call module's method (asyncron)"},
    {id: 10, name: "split flow"},
    {id: 11, name: "start flow"},
    {id: 12, name: "switch"},
    {id: 21, name: "place message"},
    {id: 50, name: "(variable) copy"}
  ];

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
    for (var i in this.step_functions)
    {
      var fct = this.step_functions[i];
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

  modal__workstep_del__open (modal_name, modal_content: IWorkstep)
  {
    const modal = this.modalService.open(modal_name, {ariaLabelledBy: 'modal-basic-title'});

    modal.result.then((result) => {

      if (modal_content)
      {
        this.AuthService.json_rpc ('FLOW', 'workstep_rm', {'wfid' : this.wfid, 'wsid' : modal_content.id, 'sqid' : modal_content.sqtid}).subscribe(() => {

          this.workflow_get ();
        });
      }

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  modal__workstep_add__open (modal_name, modal_content: IWorkstep)
  {
    const modal = this.modalService.open(modal_name, {ariaLabelledBy: 'modal-basic-title', size: 'lg'});
    var flow_method: string;

    if (modal_content)
    {
      this.modal_step_content = modal_content;
      flow_method = 'workstep_set';

      if (!this.modal_step_content.pdata)
      {
        this.modal_step_content.pdata = {};
      }
    }
    else
    {
      this.modal_step_content = new IWorkstep;
      flow_method = 'workstep_add';
    }

    modal.result.then((result) => {

      if (result)
      {
        var step_name = result.step_name;
        var step_fctid = Number(result.step_fctid);

        delete result.step_name;
        delete result.step_fctid;

        this.AuthService.json_rpc ('FLOW', flow_method, {'wfid' : this.wfid, 'wsid' : modal_content ? modal_content.id : undefined, 'sqid' : 1, 'name': step_name, 'fctid': step_fctid, 'pdata': result}).subscribe(() => {

          this.workflow_get ();
        });
      }

    }, () => {

    });
  }

}

//-----------------------------------------------------------------------------

class IWorkstep {

  public id: number;
  public sqtid: number;
  public fctid: number;
  public name: string;
  public pdata = {};

}
