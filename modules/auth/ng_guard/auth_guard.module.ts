import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { TrloModule } from '@qbus/trlo.module';
import { AuthSessionModule } from '@qbus/auth_session.module';

//-----------------------------------------------------------------------------

import { AuthSiteDirective } from './directives/component';

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
    ]
})
export class AuthGuardModule
{
}

//-----------------------------------------------------------------------------
