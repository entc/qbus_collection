import { Component, OnInit } from '@angular/core';
import { NgbModal, ModalDismissReasons } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { ActivatedRoute, Router } from '@angular/router';
import { AuthSession } from '@qbus/auth_session';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow-process-details',
  templateUrl: './component.html'
})

export class FlowProcessDetailsComponent implements OnInit {

  public psid: number;

  //-----------------------------------------------------------------------------

  constructor(private auth_session: AuthSession, private modalService: NgbModal, private router: Router, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.psid = Number(this.route.snapshot.paramMap.get('psid'));
  }

  //-----------------------------------------------------------------------------

  return_page ()
  {
    this.router.navigate(['../../flow_process'], {relativeTo: this.route});
  }

  //-----------------------------------------------------------------------------
}
