import { Component, OnInit } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { ActivatedRoute, Router } from '@angular/router';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-process-details',
  templateUrl: './component.html'
})

export class FlowProcessDetailsComponent implements OnInit {

  public psid: number;

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.psid = Number(this.route.snapshot.paramMap.get('psid'));
  }

  //-----------------------------------------------------------------------------
}
