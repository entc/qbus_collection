import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { Routes, RouterModule } from '@angular/router';
import { QbngModule } from '@qbus/qbng.module';
import { FlowEditorComponent, FlowEditorRmModalComponent, FlowEditorAddModalComponent } from './flow_editor/component';
import { FlowWorkstepsComponent, FlowWorkstepsRmModalComponent, FlowWorkstepsAddModalComponent } from './flow_worksteps/component';
import { FlowProcessComponent, FlowProcessDataModalComponent } from './flow_process/component';
import { FlowProcessDetailsComponent } from './flow_process_details/component';

import { AuthSessionModule } from '@qbus/auth_session.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';
import { FlowLogsModule } from '@qbus/flow_logs.module';

const routes: Routes = [
  /*
  { path: 'flow-editor', component: FlowEditorComponent },
  { path: 'flow-editor/:wfid', component: FlowWorkstepsComponent },
  { path: 'flow-process', component: FlowProcessComponent },
  { path: 'flow-process/:psid', component: FlowProcessDetailsComponent }
  */
];

@NgModule({
  declarations:
  [
    FlowWorkstepsComponent,
    FlowWorkstepsRmModalComponent,
    FlowWorkstepsAddModalComponent,
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
    QbngModule,
    AuthSessionModule,
    RouterModule.forChild(routes)
  ],
  exports: [RouterModule],
  entryComponents: [
    FlowWorkstepsRmModalComponent,
    FlowWorkstepsAddModalComponent,
    FlowEditorRmModalComponent,
    FlowEditorAddModalComponent,
    FlowProcessDataModalComponent
  ]
})
export class FlowAdminModule
{
}
