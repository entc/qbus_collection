import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';

import { AuthServiceModule } from '@qbus/auth_service.module';
import { TrloModule } from '@qbus/trlo.module';

import { FlowLogsComponent, FlowChainComponent, FlowLogDetailsModalComponent } from './flow_logs/component';

@NgModule({
  declarations: [ FlowLogsComponent, FlowChainComponent, FlowLogDetailsModalComponent ],
  imports: [CommonModule, FormsModule, TrloModule, AuthServiceModule],
  exports: [RouterModule, FlowLogsComponent],
  entryComponents: [FlowLogDetailsModalComponent]
})
export class FlowLogsModule
{
}
