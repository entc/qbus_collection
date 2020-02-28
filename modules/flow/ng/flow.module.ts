import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

// flow modules
import { FlowRoutingModule } from '@qbus/flow-routing.module';
import { FlowListComponent } from '@qbus/flow-list/flow-list.component';
import { FlowEditorComponent } from '@qbus/flow-editor/flow-editor.component';
import { FlowProcessComponent } from '@qbus/flow-process/flow-process.component';

@NgModule({
  declarations: [FlowListComponent, FlowEditorComponent, FlowProcessComponent],
  imports: [
    CommonModule,
    FormsModule,
    FlowRoutingModule
  ]
})
export class FlowModule { }
