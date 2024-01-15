import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { HttpClientModule } from '@angular/common/http';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { AuthSession, AuthWorkspacesModalComponent, Auth2FactorModalComponent, AuthSessionComponentDirective, AuthFirstuseModalComponent, AuthLoginModalComponent } from '@qbus/auth_session';
import { AuthSessionRoleDirective, AuthSessionContentComponent, AuthSessionPassResetComponent, AuthSessionLangComponent } from './component';

// components
import { AuthLoginComponent } from './auth_login/component';
import { AuthPasscheckComponent } from './auth_passcheck/component';
import { AuthSessionGuard } from '@qbus/auth_session/route';

//=============================================================================

@NgModule({
    declarations: [
        AuthWorkspacesModalComponent,
        Auth2FactorModalComponent,
        AuthFirstuseModalComponent,
        AuthSessionComponentDirective,
        AuthSessionRoleDirective,
        AuthSessionContentComponent,
        AuthSessionPassResetComponent,
        AuthSessionLangComponent,
        AuthLoginComponent,
        AuthLoginModalComponent,
        AuthPasscheckComponent
    ],
    providers: [
        AuthSession,
        AuthSessionGuard
    ],
    imports: [
        CommonModule,
        FormsModule,
        TrloModule,
        QbngModule,
        NgbModule,
        HttpClientModule
    ],
    exports: [
        AuthSessionRoleDirective,
        AuthSessionContentComponent,
        AuthSessionPassResetComponent,
        AuthSessionLangComponent,
        AuthLoginComponent,
        AuthPasscheckComponent
    ]
})
export class AuthSessionModule
{
}
