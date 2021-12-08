import { Injectable } from '@angular/core';
import { FlowWidgetSyncronComponent, FlowWidgetAsyncronComponent, FlowWidgetWaitforlistComponent, FlowWidgetSplitComponent, FlowWidgetSwitchComponent, FlowWidgetIfComponent, FlowWidgetCopyComponent, FlowWidgetCreateNodeComponent, FlowWidgetMoveComponent } from './widgets';

//-----------------------------------------------------------------------------

export class StepFct
{
  id: number;
  name: string;
  desc: string;
  type: any;
}

//-----------------------------------------------------------------------------

@Injectable()
export class FlowUserFormService
{
  public values: StepFct[] = [];

  //---------------------------------------------------------------------------

  constructor ()
  {
    this.values = [];
  }

  //---------------------------------------------------------------------------

  add (item: StepFct)
  {
    this.values.push (item);
  }

  //---------------------------------------------------------------------------

  public userform_name_get (usrid: number): string
  {
    if (usrid)
    {
      const found: StepFct = this.values.find ((element: StepFct) => element.id == usrid);

      return found ? found.name : '[unknown]';
    }
    else
    {
      return 'no form';
    }
  }

  //---------------------------------------------------------------------------

  get_usrid (index: number): number
  {
    try
    {
      return this.values[index].id;
    }
    catch (e)
    {
      return 0;
    }
  }

  //---------------------------------------------------------------------------

  public get_val (usrid: number): StepFct
  {
    return this.values.find ((element: StepFct) => element.id == usrid);
  }

}

//-----------------------------------------------------------------------------

@Injectable()
export class FlowFunctionService
{
  public values: StepFct[] = [];

  //---------------------------------------------------------------------------

  constructor ()
  {
    // define widgets to all background functions
    this.values = [
      {id: 3, name: "call module's method (syncron)", desc: '', type: FlowWidgetSyncronComponent},
      {id: 4, name: "call module's method (asyncron)", desc: '', type: FlowWidgetAsyncronComponent},
      {id: 5, name: "wait for list", desc: 'this step waits until a set was sent to each variable within the wait list node. a code can be specified for security reasons.', type: FlowWidgetWaitforlistComponent},
      {id: 10, name: "split flow", desc: '', type: FlowWidgetSplitComponent},
      {id: 11, name: "start flow", desc: '', type: undefined},
      {id: 12, name: "switch", desc: '', type: FlowWidgetSwitchComponent},
      {id: 13, name: "if", desc: 'checks if a variable exists. if there is an additional params node given, it checks inside this node', type: FlowWidgetIfComponent},
      {id: 21, name: "place message", desc: '', type: undefined},
      {id: 50, name: "(variable) copy", desc: '', type: FlowWidgetCopyComponent},
      {id: 51, name: "(variable) create node", desc: '', type: FlowWidgetCreateNodeComponent},
      {id: 52, name: "(variable) move", desc: 'moves a variable to a new destination and name', type: FlowWidgetMoveComponent}
    ];
  }

  //---------------------------------------------------------------------------

  add (item: StepFct)
  {
    this.values.push (item);
  }

  //---------------------------------------------------------------------------

  public get_val (fctid: number): StepFct
  {
    return this.values.find ((element: StepFct) => element.id == fctid);
  }

  //---------------------------------------------------------------------------

  public function_name_get (fctid: number): string
  {
    const found: StepFct = this.get_val (fctid);

    return found ? found.name : '[unknown]';
  }

  //---------------------------------------------------------------------------

  get_index (fctid: number): number
  {
    return this.values.findIndex((element: StepFct) => element.id == fctid);
  }

  //---------------------------------------------------------------------------

  get_desc (index: number): string
  {
    try
    {
      return this.values[index].desc;
    }
    catch (e)
    {
      return '';
    }
  }

  //---------------------------------------------------------------------------

  get_fctid (index: number): number
  {
    try
    {
      return this.values[index].id;
    }
    catch (e)
    {
      return 0;
    }
  }
}
