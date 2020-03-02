import { Component, OnInit } from '@angular/core';
import { NgbModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';

// auth service
import { ConfigService, PageReloadService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow',
  templateUrl: './flow-editor.component.html',
  styleUrls: ['./flow-editor.component.scss'],
  providers: [ConfigService, PageReloadService, NgbModal]
})
export class FlowEditorComponent implements OnInit {

  workflows: Observable<Array<IWorkflow>>;

  //-----------------------------------------------------------------------------

  constructor (private configService: ConfigService, private reloadService: PageReloadService, private modalService: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.workflows = this.configService.json_get ('FLOW', 'workflow_get', {});

    this.reloadService.set (() => {

      this.workflow_get ();

    });

    this.reloadService.run ();
  }

  //-----------------------------------------------------------------------------

  workflow_get ()
  {
    this.workflows = this.configService.json_get ('FLOW', 'workflow_get', {});
  }

  //-----------------------------------------------------------------------------

  modal__workflow_add__open (content)
  {
    this.modalService.open(content, {ariaLabelledBy: 'modal-basic-title'}).result.then((result) => {

      if (result)
      {
        this.configService.json_get ('FLOW', 'workflow_add', {'name' : result.workflow_name}).subscribe(() => {

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

      this.configService.json_get ('FLOW', 'workflow_rm', {'wfid' : wfid}).subscribe(() => {

        this.workflow_get ();

      });

    }, () => {

    });
  }

  //-----------------------------------------------------------------------------

  modal__process_add (wfid: number)
  {
    this.configService.json_get ('SASA', 'flow_test', {'wfid' : wfid, 'sqtid' : 1}).subscribe(() => {



    });
  }

//-----------------------------------------------------------------------------

}

class IWorkflow {

  public id: number;
  public cnt: number;
  public name: string;

}
