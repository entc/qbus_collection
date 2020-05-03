import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { PageToolbarPipe, QbngSortPipe, QbngFilterPipe, PageToolbarComponent } from './page_toolbar/component';
import { NgxPaginationModule, PaginatePipe } from 'ngx-pagination';

@NgModule({
  declarations: [PageToolbarPipe, QbngSortPipe, QbngFilterPipe, PageToolbarComponent],
  imports: [CommonModule, FormsModule, NgxPaginationModule],
  exports: [PageToolbarPipe, QbngSortPipe, QbngFilterPipe, PageToolbarComponent, PaginatePipe]
})
export class PageToolbarModule
{
}
