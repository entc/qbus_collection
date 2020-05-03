import { Component, OnInit } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-process',
  templateUrl: './component.html',
  styleUrls: ['./component.scss']
})

export class FlowProcessComponent implements OnInit {

  processes: Observable<IProcessStep[]>;

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
    this.processes = this.AuthService.json_rpc ('FLOW', 'process_all', {});
  }

  //-----------------------------------------------------------------------------

  show_details(item: IProcessStep)
  {
    
  }

  //-----------------------------------------------------------------------------
}

class IProcessStep
{
  id: number;

}
