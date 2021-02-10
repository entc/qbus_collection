import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { QbngParamsComponent } from './qbng_params/component';
import { QbngSwitchComponent, QbngUploadComponent, QbngUploadModalComponent } from './qbng_widgets/component';
import { SizeService } from './size_service/service'
import { SizeDetectorComponent } from './size_service/component'

@NgModule({
  declarations: [
    QbngParamsComponent,
    QbngSwitchComponent,
    QbngUploadComponent,
    QbngUploadModalComponent,
    SizeDetectorComponent
  ],
  imports: [
    CommonModule,
    FormsModule,
    NgbModule,
    TrloModule
  ],
  providers: [
    SizeService
  ],
  exports: [
    QbngParamsComponent,
    QbngSwitchComponent,
    QbngUploadComponent,
    SizeDetectorComponent
  ],
  entryComponents: [
    QbngUploadModalComponent
  ]
})
export class QbngModule
{
}
