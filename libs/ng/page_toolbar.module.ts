import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { PageToolbarPipe, QbngSortPipe, QbngFilterPipe, PageToolbarComponent, RxModelDirective, QbngFSPipeFile } from './page_toolbar/component';
import { NgxPaginationModule, PaginatePipe } from 'ngx-pagination';
import { PageService } from './page_service/service';

@NgModule({
  declarations: [PageToolbarPipe, QbngSortPipe, QbngFilterPipe, PageToolbarComponent, RxModelDirective, QbngFSPipeFile],
  imports: [CommonModule, FormsModule, NgxPaginationModule],
  exports: [PageToolbarPipe, QbngSortPipe, QbngFilterPipe, PageToolbarComponent, PaginatePipe, RxModelDirective, QbngFSPipeFile],
  providers: [ PageService ]
})
export class PageToolbarModule
{
}
