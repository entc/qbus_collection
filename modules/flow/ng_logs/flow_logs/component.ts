import { Component, OnInit, Input } from '@angular/core';
import { ActivatedRoute, Params } from '@angular/router';
import { NgbModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'flow-logs',
  templateUrl: './component.html'
})
export class FlowLogsComponent implements OnInit
{
  @Input('psid') psid: number;

  public chain_items: ChainItem[];

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.AuthService.json_rpc ('FLOW', 'chain_get', {'psid': this.psid}).subscribe((data: ChainItem[]) => {

      this.chain_items = data;
    });
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
  templateUrl: './part_chain.html',
  styleUrls: ['./component.css']
})
export class FlowChainComponent implements OnInit
{
  @Input('logs') logs: ChainItem[];

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
