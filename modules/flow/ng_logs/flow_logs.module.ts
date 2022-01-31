import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';

import { AuthSessionModule } from '@qbus/auth_session.module';
import { TrloModule } from '@qbus/trlo.module';
import { TranslocoModule } from '@ngneat/transloco';

import { FlowLogsComponent, FlowChainComponent, FlowLogDetailsModalComponent } from './flow_logs/component';

@NgModule({
  declarations: [ FlowLogsComponent, FlowChainComponent, FlowLogDetailsModalComponent ],
  imports: [CommonModule, FormsModule, TranslocoModule, TrloModule, AuthSessionModule],
  exports: [RouterModule, FlowLogsComponent],
  entryComponents: [FlowLogDetailsModalComponent]
})
export class FlowLogsModule
{
}
