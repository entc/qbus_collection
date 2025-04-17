import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { NgxPaginationModule } from 'ngx-pagination';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';
import { AuthSessionModule } from '@qbus/auth_session.module';

//-----------------------------------------------------------------------------

// components
import { AuthRolesComponent } from './auth_roles/component';
import { AuthUsersComponent, AuthUsersSettingsModalComponent, AuthUsersRolesModalComponent, AuthUsersSessionsModalComponent, AuthUsersAddModalComponent, AuthUsersPasswdModalComponent } from './auth_users/component';
import { AuthSessionInfoComponent, AuthSessionInfoModalComponent, AuthSessionInfoNameModalComponent, AuthSessionInfoPasswordModalComponent } from './auth_info/component';
import { AuthMsgsComponent } from './auth_msgs/component';
import { AuthLogsComponent } from './auth_logs/component';
import { AuthPermComponent } from './auth_perm/component';
import { AuthLoginComponent, Auth2FactorModalComponent, AuthLoginModalComponent } from './auth_login/component';
import { AuthPasscheckComponent } from './auth_passcheck/component';
import { AuthSessionPassResetComponent } from './auth_passreset/component';
import { AuthLastComponent } from './auth_last/component';

//-----------------------------------------------------------------------------

@NgModule({
    declarations: [
        AuthRolesComponent,
        AuthUsersComponent,
        AuthUsersSettingsModalComponent,
        AuthUsersRolesModalComponent,
        AuthUsersSessionsModalComponent,
        AuthUsersAddModalComponent,
        AuthUsersPasswdModalComponent,
        AuthSessionInfoComponent,
        AuthSessionInfoModalComponent,
        AuthSessionInfoNameModalComponent,
        AuthSessionInfoPasswordModalComponent,
        AuthMsgsComponent,
        AuthLogsComponent,
        AuthPermComponent,
        AuthLoginComponent,
        Auth2FactorModalComponent,
        AuthLoginModalComponent,
        AuthPasscheckComponent,
        AuthSessionPassResetComponent,
        AuthLastComponent
    ],
    imports: [
        CommonModule,
        FormsModule,
        NgbModule,
        NgxPaginationModule,
        TrloModule,
        QbngModule,
        PageToolbarModule,
        AuthSessionModule
    ],
    exports: [
        AuthUsersComponent,
        AuthRolesComponent,
        AuthLogsComponent,
        AuthPermComponent,
        AuthLoginComponent,
        AuthPasscheckComponent,
        AuthSessionPassResetComponent,
        AuthLastComponent
    ]
})
export class AuthCommonModule
{
}
