import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { QbngParamsComponent } from './qbng_params/component';
import { QbngSwitchComponent, QbngUploadComponent, QbngUploadModalComponent } from './qbng_widgets/component';

@NgModule({
  declarations: [
    QbngParamsComponent,
    QbngSwitchComponent,
    QbngUploadComponent,
    QbngUploadModalComponent
  ],
  imports: [
    CommonModule,
    FormsModule,
    NgbModule,
    TrloModule
  ],
  exports: [
    QbngParamsComponent,
    QbngSwitchComponent,
    QbngUploadComponent
  ],
  entryComponents: [
    QbngUploadModalComponent
  ]
})
export class QbngModule
{
}
