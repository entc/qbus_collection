import { Component, OnInit, Injector } from '@angular/core';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { ActivatedRoute, Router } from '@angular/router';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-process',
  templateUrl: './component.html'
})
export class FlowProcessComponent implements OnInit {

  processes: Observable<ProcessStep[]>;

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private router: Router, private modalService: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.processes_get ();
  }

  //-----------------------------------------------------------------------------

  processes_get ()
  {
    this.processes = this.AuthService.json_rpc ('FLOW', 'process_all', {});
  }

  //-----------------------------------------------------------------------------

  show_details (item: ProcessStep)
  {
    this.router.navigate(['/flow_process', item.id]);
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

  constructor (private auth_service: AuthService, public modal: NgbActiveModal, private process_step: ProcessStep)
  {
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.auth_service.json_rpc ('FLOW', 'process_get', {'psid': this.process_step.id}).subscribe ((data: any) => {

      this.content = JSON.stringify(data.content);

    });
  }

}
