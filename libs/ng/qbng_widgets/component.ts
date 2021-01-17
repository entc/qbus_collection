import { Component, Input, Output, EventEmitter } from '@angular/core';

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-switch',
  templateUrl: './component.html'
})
export class QbngSwitchComponent {

  @Input('val') val: boolean;
  @Output('onVal') onChange = new EventEmitter ();

  //-----------------------------------------------------------------------------

  constructor ()
  {
  }

  //-----------------------------------------------------------------------------

  on_toggle ()
  {
    this.val = !this.val;
    this.onChange.emit (this.val);
  }
}

//-----------------------------------------------------------------------------
