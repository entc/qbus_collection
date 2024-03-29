import { Component, OnInit, Input, Output, Injector, EventEmitter } from '@angular/core';
import { ActivatedRoute, Params } from '@angular/router';
import { NgbModal, NgbActiveModal } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { AuthSession } from '@qbus/auth_session';

//-----------------------------------------------------------------------------

@Component({
  selector: 'flow-logs',
  templateUrl: './component.html'
})
export class FlowLogsComponent implements OnInit
{
  @Input('psid') psid: number;
  @Input('tag') tag: string;
  @Input('tools') tools: boolean;

  public chain_items: ChainItem[];
  public filter: boolean;

  private data: ChainItem[] = [];

  //-----------------------------------------------------------------------------

  constructor(private auth_session: AuthSession, private modalService: NgbModal, private route: ActivatedRoute)
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
    var has_state_change = false;

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

        switch (item_chain.state)
        {
          case 0:
          {
            if (has_state_change == false)
            {
              item_chain.between = true;
              has_state_change = true;
            }

            break;
          }
          case 1:  // error
          {
            has_state_change = true;
            break;
          }
          case 2:  // halt
          {
            has_state_change = true;
            break;
          }
          case 3:  // done
          {
            break;
          }
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
    this.auth_session.json_rpc ('FLOW', 'chain_get', {'psid': this.psid}).subscribe((data: ChainItem[]) => {

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
  psid: number;
  wsid: number;
  name: string;
  logs: ChainItem[];
  fctid: number;
  tag: string;
  between: boolean;
  state: number;

  states: ChainItemStatus[];
}

class ChainEvent
{
  err_text: string;
  timestamp: string;
  remote: string;
}

class ChainItemStatus
{
  data: ChainEvent;
  pot: string;
  state: number;
}

@Component({
  selector: 'flow-chain',
  templateUrl: './part_chain.html',
  styleUrls: ['./component.css']
})
export class FlowChainComponent implements OnInit
{
  @Input('logs') logs: ChainItem[];
  @Input('tools') tools: boolean;
  @Output ('refresh') refresh = new EventEmitter<boolean>();

  show_details: boolean;

  //-----------------------------------------------------------------------------

  constructor(private modalService: NgbModal, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.show_details = true;
  }

  //-----------------------------------------------------------------------------

  open_process_modal (item: ChainItem)
  {
    if (this.tools)
    {
      var details: FlowDetails = new FlowDetails;

      details.psid = item.psid;
      details.wsid = item.wsid;

      this.modalService.open (FlowLogDetailsModalComponent, {ariaLabelledBy: 'modal-basic-title', 'size': 'lg', injector: Injector.create([{provide: FlowDetails, useValue: details}])}).result.then((result) => {


      }, () => {

      });
    }
  }

  //-----------------------------------------------------------------------------

  refresh_apply ()
  {
    this.refresh.emit (true);
    console.log('set step done (emit)');
  }

  //-----------------------------------------------------------------------------

}

//=============================================================================

export class FlowDetails
{
  psid: number;
  wsid: number;
}

export class FlowDetailsItem
{
  current_state: number;
  current_step: number;
  sync_pa_id: number;
  sync_pa_cnt: number;
  sync_pa_wsid: number;
  sync_ci_id: number;
  sync_ci_cnt: number;
  sync_ci_wsid: number;

  tdata: object;
  pdata: object;

  tdata_text: string;
  pdata_text: string;
}

@Component({
  selector: 'flow-logs-details-modal-component',
  templateUrl: './modal_details.html'
}) export class FlowLogDetailsModalComponent implements OnInit {

  public data: FlowDetailsItem;
  public psid: number;
  public wsid: number;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, public modal: NgbActiveModal, private details: FlowDetails)
  {
    this.psid = details.psid;
    this.wsid = details.wsid;
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
    this.auth_session.json_rpc ('FLOW', 'process_details', {psid: this.psid}).subscribe((data: FlowDetailsItem) => {

      this.data = data;
      this.data.tdata_text = JSON.stringify(this.data.tdata);
      this.data.pdata_text = JSON.stringify(this.data.pdata);
    });
  }

  //-----------------------------------------------------------------------------

  rerun_next ()
  {
    this.auth_session.json_rpc ('FLOW', 'process_next', {psid: this.psid}).subscribe(() => {

    //  this.refresh.emit (true);
      console.log('set step done');

    });
  }

  //-----------------------------------------------------------------------------

  rerun_once ()
  {
    // this is missleading, this will run the process and also continues
    this.auth_session.json_rpc ('FLOW', 'process_once', {psid: this.psid}).subscribe(() => {

    //  this.refresh.emit (true);

    });
  }

  //-----------------------------------------------------------------------------

  set_step ()
  {
    this.auth_session.json_rpc ('FLOW', 'process_step', {psid: this.psid, wsid: this.wsid}).subscribe(() => {

    });
  }

}
