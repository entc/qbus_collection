import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgxPaginationModule } from 'ngx-pagination';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';
import { AuthSessionModule } from '@qbus/auth_session.module';

//-----------------------------------------------------------------------------

// components
import { AuthLoginsComponent } from './auth_logins/component';
import { AuthApiTokenComponent } from './auth_api_token/component';
import { AuthUsersComponent, AuthUsersSettingsModalComponent } from './auth_users/component';

//-----------------------------------------------------------------------------

@NgModule({
  declarations: [
    AuthLoginsComponent,
    AuthApiTokenComponent,
    AuthUsersComponent,
    AuthUsersSettingsModalComponent
  ],
  imports: [
    CommonModule,
    FormsModule,
    NgxPaginationModule,
    TrloModule,
    QbngModule,
    PageToolbarModule,
    AuthSessionModule
  ],
  exports: [
    AuthUsersComponent
  ],
  entryComponents: [
    AuthUsersSettingsModalComponent
  ]
})
export class AuthAdminModule
{
}
