import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { AuthSessionModule } from '@qbus/auth_session.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';

// components
import { FlowWorkflowSelectorComponent, FlowWorkflowSelectorModalComponent } from '@qbus/flow_common/component';


@NgModule({
    declarations: [
        FlowWorkflowSelectorComponent,
        FlowWorkflowSelectorModalComponent
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
    providers: [],
    exports: [
        FlowWorkflowSelectorComponent
    ]
})
export class FlowCommonModule
{
}
