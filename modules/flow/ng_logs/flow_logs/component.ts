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
  @Input('tag') tag: string;

  public chain_items: ChainItem[];
  public filter: boolean;

  private data: ChainItem[];

  //-----------------------------------------------------------------------------

  constructor(private AuthService: AuthService, private modalService: NgbModal, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.filter = true;
    this.fetch ();
  }

  //-----------------------------------------------------------------------------

  grab_items (data: ChainItem[])
  {
    for (var i in data)
    {
      const item: ChainItem = data[i];

      if (item.tag == this.tag)
      {
        const item_chain = Object.assign(item);

        if (item_chain.logs)
        {
          item_chain.logs = undefined;
        }

        this.chain_items.push (item_chain);
      }

      if (item.logs)
      {
        this.grab_items (item.logs);
      }
    }
  }

  //-----------------------------------------------------------------------------

  apply_data ()
  {
    if (this.filter)
    {
      this.chain_items = [];
      this.grab_items (this.data);
    }
    else
    {
      this.chain_items = this.data;
    }
  }

  //-----------------------------------------------------------------------------

  fetch ()
  {
    this.AuthService.json_rpc ('FLOW', 'chain_get', {'psid': this.psid}).subscribe((data: ChainItem[]) => {

      this.data = data;
      this.apply_data ();

    });
  }

  //-----------------------------------------------------------------------------

  filter_acivate ()
  {
    this.filter = true;
    this.apply_data ();
  }

  //-----------------------------------------------------------------------------

  filter_deactivate ()
  {
    this.filter = false;
    this.apply_data ();
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
  tag: string;

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
