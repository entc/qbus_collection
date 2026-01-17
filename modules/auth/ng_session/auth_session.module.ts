import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { HttpClientModule } from '@angular/common/http';
import { AuthSession, AuthSessionRoleDirective, AuthSessionWorkspaceDirective, AuthWorkspaceGuard } from '@qbus/auth_session';
import { ConnModule } from '@conn/conn_module';
//=============================================================================

@NgModule({
    declarations: [
        AuthSessionRoleDirective,
        AuthSessionWorkspaceDirective
    ],
    providers: [
        AuthSession,
        AuthWorkspaceGuard
    ],
    imports: [
        CommonModule,
        ConnModule,
        HttpClientModule
    ],
    exports: [
        AuthSessionRoleDirective,
        AuthSessionWorkspaceDirective
    ]
})
export class AuthSessionModule
{
}
