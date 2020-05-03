import { Component, OnInit, Input } from '@angular/core';
import { ActivatedRoute, Params } from '@angular/router';
import { NgbModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'flow-logs',
  templateUrl: './component.html',
  styleUrls: ['./component.css']
})
export class FlowLogsComponent implements OnInit
{
  @Input('psid') psid: number;

  chain_items: Observable<ChainItem[]>;

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.chain_items = this.AuthService.json_rpc ('FLOW', 'chain_get', {'psid': this.psid});
  }

  //-----------------------------------------------------------------------------

}

class ChainItem
{
  state: number;
  name: string;
  created: string;
  dataset_name: string;
  logs: ChainItem[];
  fctid: number;
  party: string;

  vdata: VDataItem;
}

class VDataItem
{
  events: ChainEvent[];
  access: ChainEvent[];
}

class ChainEvent
{
  err_text: string;
  timestamp: string;
}

@Component({
  selector: 'flow-chain',
  templateUrl: './part_chain.html'
})
export class FlowChainComponent implements OnInit
{
  @Input('chain') log: ChainItem;

  show_details: boolean;

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.show_details = true;
  }

  //-----------------------------------------------------------------------------

}
