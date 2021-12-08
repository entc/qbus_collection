import { Component, EventEmitter } from '@angular/core';
import { IWorkstep } from './headers';

//-----------------------------------------------------------------------------

export abstract class IFlowEditorWidget
{
  protected event_emitter: EventEmitter<IWorkstep> = undefined;
  public content: IWorkstep;

  //---------------------------------------------------------------------------

  constructor ()
  {
  }

  //---------------------------------------------------------------------------

  public set content_setter (val: IWorkstep)
  {
    this.on_content_change (val);
  }

  //---------------------------------------------------------------------------

  public set emitter (emitter: EventEmitter<IWorkstep>)
  {
    this.event_emitter = emitter;
  }

  //---------------------------------------------------------------------------

  protected emit (content: IWorkstep)
  {
    console.log('new content emitted');

    if (this.event_emitter)
    {
      this.event_emitter.emit (content);
    }
    else
    {
      console.log('no emitter was set');
    }
  }

  //---------------------------------------------------------------------------

  protected on_content_change (val: IWorkstep)
  {
    this.content = val;
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-syncron',
  templateUrl: './widget_call.html'
})
export class FlowWidgetSyncronComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    console.log('emit changes from form');

    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-asyncron',
  templateUrl: './widget_call.html'
})
export class FlowWidgetAsyncronComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-waitforlist',
  templateUrl: './widget_waitforlist.html'
})
export class FlowWidgetWaitforlistComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-split',
  templateUrl: './widget_split.html'
})
export class FlowWidgetSplitComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-switch',
  templateUrl: './widget_switch.html'
})
export class FlowWidgetSwitchComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-if',
  templateUrl: './widget_if.html'
})
export class FlowWidgetIfComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-copy',
  templateUrl: './widget_copy.html'
})
export class FlowWidgetCopyComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-create-node',
  templateUrl: './widget_create_node.html'
})
export class FlowWidgetCreateNodeComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-move',
  templateUrl: './widget_move.html'
})
export class FlowWidgetMoveComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.content.pdata[name] = data;
    this.emit (this.content);
  }
}
