import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { NgxPaginationModule } from 'ngx-pagination';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';
import { AuthSessionModule } from '@qbus/auth_session.module';
import { AuthCommonModule } from '@qbus/auth_common.module';

//-----------------------------------------------------------------------------

// components
import { AuthRolesComponent } from './auth_roles/component';
import { AuthSessionsComponent } from './auth_sessions/component';
import { AuthUsersComponent, AuthUsersSettingsModalComponent, AuthUsersAddModalComponent, AuthUsersPasswdModalComponent } from './auth_users/component';

//-----------------------------------------------------------------------------

@NgModule({
    declarations: [
        AuthRolesComponent,
        AuthUsersComponent,
        AuthUsersSettingsModalComponent,
        AuthUsersAddModalComponent,
        AuthUsersPasswdModalComponent,
        AuthSessionsComponent
    ],
    imports: [
        CommonModule,
        FormsModule,
        NgbModule,
        NgxPaginationModule,
        TrloModule,
        QbngModule,
        PageToolbarModule,
        AuthSessionModule,
        AuthCommonModule
    ],
    exports: [
        AuthUsersComponent,
    ]
})
export class AuthUsersModule
{
}
