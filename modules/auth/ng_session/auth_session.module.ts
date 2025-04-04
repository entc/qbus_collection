import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { HttpClientModule } from '@angular/common/http';
import { AuthSession, AuthSessionRoleDirective } from '@qbus/auth_session';
import { ConnModule } from '@conn/conn_module';

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
        ConnModule,
        HttpClientModule
    ],
    exports: [
        AuthSessionRoleDirective
    ]
})
export class AuthSessionModule
{
}
