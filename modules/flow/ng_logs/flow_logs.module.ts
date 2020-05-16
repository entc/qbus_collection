import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';

import { AuthServiceModule } from '@qbus/auth_service.module';
import { TrloModule } from '@qbus/trlo.module';

import { FlowLogsComponent, FlowChainComponent } from './flow_logs/component';

@NgModule({
  declarations: [ FlowLogsComponent, FlowChainComponent ],
  imports: [CommonModule, FormsModule, TrloModule, AuthServiceModule],
  exports: [RouterModule, FlowLogsComponent],
  entryComponents: []
})
export class FlowLogsModule
{
}