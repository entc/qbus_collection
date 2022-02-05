import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgxPaginationModule } from 'ngx-pagination';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';

//-----------------------------------------------------------------------------

// components
import { AuthRolesComponent } from './auth_roles/component';
import { AuthMsgsComponent } from './auth_msgs/component';

//-----------------------------------------------------------------------------

@NgModule({
  declarations: [
    AuthRolesComponent,
    AuthMsgsComponent
  ],
  imports: [
    CommonModule,
    FormsModule,
    NgxPaginationModule,
    TrloModule,
    QbngModule,
    PageToolbarModule
  ],
  exports: [
    AuthRolesComponent,
    AuthMsgsComponent
  ],
  entryComponents: [
  ]
})
export class AuthCommonModule
{
}
