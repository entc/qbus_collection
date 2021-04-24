import { Component, OnInit, Injector } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable, of } from 'rxjs';
import { ActivatedRoute, Router } from '@angular/router';

// auth service
import { AuthSession } from '@qbus/auth_session';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-process',
  templateUrl: './component.html'
})
export class FlowProcessComponent {

  public processes: Observable<ProcessStep[]> = of();

  //-----------------------------------------------------------------------------

  constructor(private auth_session: AuthSession, private router: Router, private modalService: NgbModal, private route: ActivatedRoute)
  {
    this.auth_session.session.subscribe ((data) => {

      if (data)
      {
        this.processes = this.auth_session.json_rpc ('FLOW', 'process_all', {});
      }
      else
      {
        this.processes = of();
      }

    });
  }

  //-----------------------------------------------------------------------------

  show_details (item: ProcessStep)
  {
    this.router.navigate(['../flow_process', item.id], {relativeTo: this.route});
  }

  //-----------------------------------------------------------------------------

  show_data (item: ProcessStep)
  {
    this.modalService.open (FlowProcessDataModalComponent, {ariaLabelledBy: 'modal-basic-title', 'size': 'lg', injector: Injector.create([{provide: ProcessStep, useValue: item}])}).result.then((result) => {


    }, () => {

    });
  }

  //-----------------------------------------------------------------------------
}

export class ProcessStep
{
  id: number;

}

//=============================================================================

@Component({
  selector: 'flow-process-data-modal-component',
  templateUrl: './modal_data.html'
}) export class FlowProcessDataModalComponent implements OnInit {

  content: string;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, public modal: NgbActiveModal, private process_step: ProcessStep)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.auth_session.json_rpc ('FLOW', 'process_get', {'psid': this.process_step.id}).subscribe ((data: any) => {

      this.content = JSON.stringify(data.content);

    });
  }

}
