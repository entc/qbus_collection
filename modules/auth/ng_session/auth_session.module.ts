import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { HttpClientModule } from '@angular/common/http';
import { AuthSession, AuthSessionRoleDirective } from '@qbus/auth_session';

//=============================================================================

@NgModule({
    declarations: [
        AuthSessionRoleDirective
    ],
    providers: [
        AuthSession
    ],
    imports: [
        CommonModule,
        HttpClientModule
    ],
    exports: [
        AuthSessionRoleDirective
    ]
})
export class AuthSessionModule
{
}
