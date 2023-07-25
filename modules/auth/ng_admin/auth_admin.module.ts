import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { NgxPaginationModule } from 'ngx-pagination';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';
import { AuthSessionModule } from '@qbus/auth_session.module';
import { AuthCommonModule } from '@qbus/auth_common.module';

//-----------------------------------------------------------------------------

// components
import { AuthLoginsComponent } from '@qbus/auth_logins/component';

//-----------------------------------------------------------------------------

@NgModule({
    declarations: [
        AuthLoginsComponent
    ],
    imports: [
        CommonModule,
        FormsModule,
        NgxPaginationModule,
        TrloModule,
        QbngModule,
        PageToolbarModule,
        AuthSessionModule,
        AuthCommonModule
    ],
    exports: []
})
export class AuthAdminModule
{
}
