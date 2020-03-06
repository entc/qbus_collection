import { Component, OnInit } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';

// auth service
import { ConfigService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-jobs-list',
  templateUrl: './component.html',
  providers: [ConfigService]
})

export class JobsListComponent implements OnInit {

  processes = new Array<JobItem>();

  //-----------------------------------------------------------------------------

  constructor(private configService: ConfigService, private modalService: NgbModal)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //-----------------------------------------------------------------------------
}

class JobItem
{


}
