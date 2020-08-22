import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { Routes, RouterModule } from '@angular/router';
import { FlowEditorComponent, FlowEditorRmModalComponent, FlowEditorAddModalComponent } from './flow_editor/component';
import { FlowWorkstepsComponent } from './flow_worksteps/component';
import { FlowProcessComponent, FlowProcessDataModalComponent } from './flow_process/component';
import { FlowProcessDetailsComponent } from './flow_process_details/component';

import { AuthServiceModule } from '@qbus/auth_service.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';
import { FlowLogsModule } from '@qbus/flow_logs.module';

const routes: Routes = [
  { path: 'flow_editor', component: FlowEditorComponent },
  { path: 'flow_editor/:wfid', component: FlowWorkstepsComponent },
  { path: 'flow_process', component: FlowProcessComponent },
  { path: 'flow_process/:psid', component: FlowProcessDetailsComponent }
];

@NgModule({
  declarations:
  [
    FlowWorkstepsComponent,
    FlowEditorComponent,
    FlowProcessComponent,
    FlowEditorRmModalComponent,
    FlowEditorAddModalComponent,
    FlowProcessDetailsComponent,
    FlowProcessDataModalComponent
  ],
  imports: [
    CommonModule,
    FormsModule,
    FlowLogsModule,
    PageToolbarModule,
    NgbModule,
    TrloModule,
    AuthServiceModule,
    RouterModule.forChild(routes)
  ],
  exports: [RouterModule],
  entryComponents: [ FlowEditorRmModalComponent, FlowEditorAddModalComponent, FlowProcessDataModalComponent ]
})
export class FlowAdminModule
{
}
