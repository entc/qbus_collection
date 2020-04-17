import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { QbngParamsComponent, QbngSortPipe } from './qbng_params/component';
import { NgJsonEditorModule } from 'ang-jsoneditor';

@NgModule({
  declarations: [QbngParamsComponent, QbngSortPipe],
  imports: [CommonModule, FormsModule, NgbModule, NgJsonEditorModule],
  exports: [QbngParamsComponent, QbngSortPipe]
})
export class QbngModule
{
}
