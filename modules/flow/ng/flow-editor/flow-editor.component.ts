import { Component, OnInit } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';

// auth service
import { ConfigService, PageReloadService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow',
  templateUrl: './flow-editor.component.html',
  styleUrls: ['./flow-editor.component.scss'],
  providers: [ConfigService, PageReloadService]
})

export class FlowEditorComponent implements OnInit {

  workflows = new Array<IWorkflow>();

//-----------------------------------------------------------------------------

  workflow_get ()
  {
    this.configService.get ('FLOW', 'workflow_get', {}).subscribe((data: Array<IWorkflow>) => {

      this.workflows = data;

    });
  }

//-----------------------------------------------------------------------------

  constructor (private configService: ConfigService, private reloadService: PageReloadService, private modalService: NgbModal)
  {
  }

//-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.reloadService.set (() => {

      this.workflow_get ();

    });

    this.reloadService.run ();
  }

//-----------------------------------------------------------------------------

  modal__workflow_add__open (content)
  {
    this.modalService.open(content, {ariaLabelledBy: 'modal-basic-title'}).result.then((result) => {

      if (result)
      {
        this.configService.get ('FLOW', 'workflow_add', {'name' : result.workflow_name}).subscribe(() => {

          this.workflow_get ();

        });
      }

    }, () => {

    });
  }

//-----------------------------------------------------------------------------

  modal__workflow_del__open (modal_id, wfid)
  {
    this.modalService.open(modal_id, {ariaLabelledBy: 'modal-basic-title'}).result.then((result) => {

      this.configService.get ('FLOW', 'workflow_rm', {'wfid' : wfid}).subscribe(() => {

        this.workflow_get ();

      });

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  modal__process_add (wfid: number)
  {
    this.configService.get ('SASA', 'flow_test', {'wfid' : wfid, 'sqtid' : 1}).subscribe(() => {



    });
  }

//-----------------------------------------------------------------------------

}

class IWorkflow {

  public id: number;
  public name: string;

}
