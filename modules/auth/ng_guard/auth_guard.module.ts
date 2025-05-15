import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { TrloModule } from '@qbus/trlo.module';
import { AuthSessionModule } from '@qbus/auth_session.module';

//-----------------------------------------------------------------------------

import { AuthSiteDirective, AuthPermissionsService } from './auth_guards/component';

//-----------------------------------------------------------------------------

@NgModule({
    declarations: [
      AuthSiteDirective
    ],
    imports: [
        CommonModule,
        AuthSessionModule,
        TrloModule
    ],
    exports: [
      AuthSiteDirective
    ],
    providers: [
      AuthPermissionsService
    ]
})
export class AuthGuardModule
{
}

//-----------------------------------------------------------------------------
