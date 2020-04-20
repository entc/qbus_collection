import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { QbngParamsComponent } from './qbng_params/component';
import { NgJsonEditorModule } from 'ang-jsoneditor';

@NgModule({
  declarations: [QbngParamsComponent],
  imports: [CommonModule, FormsModule, NgbModule, NgJsonEditorModule],
  exports: [QbngParamsComponent]
})
export class QbngModule
{
}
