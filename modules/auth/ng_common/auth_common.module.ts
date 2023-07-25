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
import { AuthRolesComponent } from './auth_roles/component';
import { AuthUsersComponent, AuthUsersSettingsModalComponent, AuthUsersRolesModalComponent, AuthUsersSessionsModalComponent, AuthUsersAddModalComponent } from './auth_users/component';
import { AuthSessionInfoComponent, AuthSessionInfoModalComponent } from './auth_info/component';
import { AuthMsgsComponent } from './auth_msgs/component';

//-----------------------------------------------------------------------------

@NgModule({
    declarations: [
        AuthRolesComponent,
        AuthUsersComponent,
        AuthUsersSettingsModalComponent,
        AuthUsersRolesModalComponent,
        AuthUsersSessionsModalComponent,
        AuthUsersAddModalComponent,
        AuthSessionInfoComponent,
        AuthSessionInfoModalComponent,
        AuthMsgsComponent
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
        AuthUsersComponent,
        AuthRolesComponent
    ]
})
export class AuthCommonModule
{
}
