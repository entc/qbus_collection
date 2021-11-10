import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { AuthSessionModule } from '@qbus/auth_session.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';

// components
import { FlowUserFormService, FlowFunctionService, FlowWidgetUsrFormComponent, FlowWorkstepsComponent, FlowWidgetFunctionComponent, FlowWorkstepsRmModalComponent, FlowWorkstepsAddModalComponent, FlowWidgetSyncronComponent, FlowWidgetAsyncronComponent, FlowWidgetWaitforlistComponent, FlowWidgetSplitComponent, FlowWidgetSwitchComponent, FlowWidgetIfComponent, FlowWidgetCopyComponent, FlowWidgetCreateNodeComponent, FlowWidgetMoveComponent } from './flow_editor_worksteps/component';
import { FlowEditorComponent, FlowEditorAddModalComponent, FlowEditorPermModalComponent } from './flow_editor_workflows/component';

@NgModule({
  declarations:
  [
    FlowEditorComponent,
    FlowEditorAddModalComponent,
    FlowEditorPermModalComponent,
    FlowWorkstepsComponent,
    FlowWorkstepsRmModalComponent,
    FlowWorkstepsAddModalComponent,
    FlowWidgetFunctionComponent,
    FlowWidgetUsrFormComponent,
    FlowWidgetSyncronComponent,
    FlowWidgetAsyncronComponent,
    FlowWidgetWaitforlistComponent,
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
    FlowWorkstepsRmModalComponent,
    FlowWorkstepsAddModalComponent,
    FlowWidgetSyncronComponent,
    FlowWidgetAsyncronComponent,
    FlowWidgetWaitforlistComponent,
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
