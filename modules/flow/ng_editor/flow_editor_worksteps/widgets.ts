import { Component, EventEmitter } from '@angular/core';
import { IWorkstep } from './headers';
import { Observable, BehaviorSubject } from 'rxjs';

//-----------------------------------------------------------------------------

export abstract class IFlowEditorWidget
{
  // this is a reference to the one in WorkstepAddModelComponent
  public workstep: BehaviorSubject<IWorkstep>;

  //---------------------------------------------------------------------------

  constructor ()
  {
  }

  //---------------------------------------------------------------------------

  protected on_init ()
  {

  }

  //---------------------------------------------------------------------------

  public set workstep_content (workstep_content: BehaviorSubject<IWorkstep>)
  {
    this.workstep = workstep_content;
    this.on_init ();
  }

  //---------------------------------------------------------------------------

  protected submit_pdata_append (name: string, data: object)
  {
    const workstep: IWorkstep = this.workstep.value;

    workstep.pdata[name] = data;

    this.workstep.next (workstep);
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
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
  }
}

//=============================================================================

@Component({
  selector: 'flow-widget-sleep',
  templateUrl: './widget_sleep.html'
})
export class FlowWidgetSleepComponent extends IFlowEditorWidget {

  //---------------------------------------------------------------------------

  constructor ()
  {
    super ();
  }

  //---------------------------------------------------------------------------

  on_pdata_change (data, name)
  {
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
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
    this.submit_pdata_append (name, data);
  }
}
