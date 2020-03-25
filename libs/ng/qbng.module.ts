import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { QbngParamsComponent, QbngSortPipe } from './qbng_params/component';

@NgModule({
  declarations: [QbngParamsComponent, QbngSortPipe],
  imports: [CommonModule, FormsModule, NgbModule],
  exports: [QbngParamsComponent, QbngSortPipe]
})
export class QbngModule
{
}
