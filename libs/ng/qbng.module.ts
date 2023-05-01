import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { QbngParamsComponent } from './qbng_params/component';
import { QbngSwitchComponent, QbngSwitchMultiComponent, QbngUploadComponent, QbngUploadModalComponent, QbngTimePeriodModalComponent, QbngLoadingPipe } from './qbng_widgets/component';
import { SizeService } from './size_service/service';
import { SizeDetectorComponent } from './size_service/component';
import { QbngSpinnerModalComponent, QbngSpinnerOkModalComponent, QbngSuccessModalComponent, QbngErrorModalComponent, QbngWarnOptionModalComponent } from './qbng_modals/component';
//import { QbngCastPipe, QbngUsingDirective } from './qbng_common/component';

@NgModule({
    declarations: [
        QbngParamsComponent,
        QbngSwitchComponent,
        QbngSwitchMultiComponent,
        QbngUploadComponent,
        QbngUploadModalComponent,
        QbngTimePeriodModalComponent,
        SizeDetectorComponent,
        QbngSpinnerModalComponent,
        QbngSpinnerOkModalComponent,
        QbngSuccessModalComponent,
        QbngErrorModalComponent,
        QbngWarnOptionModalComponent,
        QbngLoadingPipe
        /*
            QbngCastPipe,
            QbngUsingDirective
            */
    ],
    imports: [
        CommonModule,
        FormsModule,
        NgbModule,
        TrloModule,
    ],
    providers: [
        SizeService
    ],
    exports: [
        QbngParamsComponent,
        QbngSwitchComponent,
        QbngSwitchMultiComponent,
        QbngUploadComponent,
        SizeDetectorComponent,
        QbngLoadingPipe
        /*
        QbngCastPipe,
        QbngUsingDirective
        */
    ]
})
export class QbngModule
{
}
