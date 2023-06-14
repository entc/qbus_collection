import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { AuthSessionModule } from '@qbus/auth_session.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';

//-----------------------------------------------------------------------------

// components
import { FlowWidgetUsrFormComponent, FlowWorkstepsComponent, FlowWidgetFunctionComponent, FlowWorkstepsAddModalComponent } from './flow_editor_worksteps/component';
import { FlowEditorComponent, FlowEditorAddModalComponent, FlowEditorPermModalComponent } from './flow_editor_workflows/component';
import { FlowUserFormService, FlowFunctionService } from './flow_editor_worksteps/services';
import { FlowWidgetSyncronComponent, FlowWidgetAsyncronComponent, FlowWidgetWaitforlistComponent, FlowWidgetSleepComponent, FlowWidgetSplitComponent, FlowWidgetSwitchComponent, FlowWidgetIfComponent, FlowWidgetCopyComponent, FlowWidgetCreateNodeComponent, FlowWidgetMoveComponent } from './flow_editor_worksteps/widgets';

//-----------------------------------------------------------------------------

@NgModule({
  declarations:
  [
    FlowEditorComponent,
    FlowEditorAddModalComponent,
    FlowEditorPermModalComponent,
    FlowWorkstepsComponent,
    FlowWorkstepsAddModalComponent,
    FlowWidgetFunctionComponent,
    FlowWidgetUsrFormComponent,
    FlowWidgetSyncronComponent,
    FlowWidgetAsyncronComponent,
    FlowWidgetWaitforlistComponent,
    FlowWidgetSleepComponent,
    FlowWidgetSplitComponent,
    FlowWidgetSwitchComponent,
    FlowWidgetIfComponent,
    FlowWidgetCopyComponent,
    FlowWidgetCreateNodeComponent,
    FlowWidgetMoveComponent
  ],
  imports: [
    CommonModule,
    FormsModule,
    PageToolbarModule,
    NgbModule,
    TrloModule,
    QbngModule,
    AuthSessionModule
  ],
  providers: [
    FlowFunctionService,
    FlowUserFormService
  ],
  exports: [
    FlowWorkstepsComponent,
    FlowWidgetFunctionComponent,
    FlowWidgetUsrFormComponent
  ],
  entryComponents: [
    FlowEditorAddModalComponent,
    FlowEditorPermModalComponent,
    FlowWorkstepsAddModalComponent,
    FlowWidgetSyncronComponent,
    FlowWidgetAsyncronComponent,
    FlowWidgetWaitforlistComponent,
    FlowWidgetSleepComponent,
    FlowWidgetSplitComponent,
    FlowWidgetSwitchComponent,
    FlowWidgetIfComponent,
    FlowWidgetCopyComponent,
    FlowWidgetCreateNodeComponent,
    FlowWidgetMoveComponent
  ]
})
export class FlowEditorModule
{
}
