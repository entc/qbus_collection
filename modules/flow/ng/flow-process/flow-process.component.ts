import { Component, OnInit } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-process',
  templateUrl: './flow-process.component.html',
  styleUrls: ['./flow-process.component.scss'],
  providers: [AuthService]
})

export class FlowProcessComponent implements OnInit {

  processes = new Array<IProcessStep>();

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal)
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
    this.AuthService.json_rpc ('FLOW', 'process_get', {}).subscribe((data: Array<IProcessStep>) => {

      this.processes = data;

    });
  }

  //-----------------------------------------------------------------------------
}

class IProcessStep
{


}
