import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { AuthServiceModule } from '@qbus/auth_service.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';

// components
import { FlowFunctionService, FlowWorkstepsComponent, FlowWidgetComponent, FlowWorkstepsRmModalComponent, FlowWorkstepsAddModalComponent, FlowWidgetSyncronComponent, FlowWidgetAsyncronComponent, FlowWidgetWaitforlistComponent, FlowWidgetSplitComponent, FlowWidgetSwitchComponent, FlowWidgetIfComponent, FlowWidgetCopyComponent, FlowWidgetCreateNodeComponent, FlowWidgetMoveComponent } from './flow_editor_worksteps/component';

@NgModule({
  declarations:
  [
    FlowWorkstepsComponent,
    FlowWorkstepsRmModalComponent,
    FlowWorkstepsAddModalComponent,
    FlowWidgetComponent,
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
    AuthServiceModule
  ],
  providers: [
    FlowFunctionService
  ],
  exports: [
    FlowWorkstepsComponent,
    FlowWidgetComponent
  ],
  entryComponents: [
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
