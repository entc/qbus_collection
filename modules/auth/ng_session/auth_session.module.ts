import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';
import { HttpClientModule } from '@angular/common/http';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
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
        FormsModule,
        TrloModule,
        NgbModule,
        HttpClientModule
    ],
    exports: [
        AuthSessionRoleDirective
    ]
})
export class AuthSessionModule
{
}
