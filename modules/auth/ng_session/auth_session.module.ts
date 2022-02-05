import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { HttpClientModule } from '@angular/common/http';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { AuthSession, AuthSessionLoginModalComponent, AuthWorkspacesModalComponent, Auth2FactorModalComponent, AuthSessionComponentDirective, AuthSessionLoginComponent, AuthFirstuseModalComponent } from '@qbus/auth_session';
import { AuthSessionMenuComponent, AuthSessionInfoModalComponent, AuthSessionRoleDirective, AuthSessionContentComponent, AuthSessionPassResetComponent, AuthSessionLangComponent, AuthSessionPasscheckComponent } from './component';
import { AuthCommonModule } from '@qbus/auth_common.module';

//import { AuthSessionGuard } from '@qbus/auth_session/route';

//=============================================================================

@NgModule({
  declarations: [
    AuthWorkspacesModalComponent,
    Auth2FactorModalComponent,
    AuthFirstuseModalComponent,
    AuthSessionLoginModalComponent,
    AuthSessionMenuComponent,
    AuthSessionComponentDirective,
    AuthSessionLoginComponent,
    AuthSessionInfoModalComponent,
    AuthSessionRoleDirective,
    AuthSessionContentComponent,
    AuthSessionPassResetComponent,
    AuthSessionLangComponent,
    AuthSessionPasscheckComponent
  ],
  providers: [
    AuthSession
//    AuthSessionGuard
  ],
  imports: [
    CommonModule,
    FormsModule,
    TrloModule,
    QbngModule,
    NgbModule,
    HttpClientModule,
    AuthCommonModule
  ],
  entryComponents: [
    AuthWorkspacesModalComponent,
    Auth2FactorModalComponent,
    AuthFirstuseModalComponent,
    AuthSessionLoginModalComponent,
    AuthSessionLoginComponent,
    AuthSessionInfoModalComponent
  ],
  exports: [
    AuthSessionMenuComponent,
    AuthSessionRoleDirective,
    AuthSessionLoginComponent,
    AuthSessionContentComponent,
    AuthSessionPassResetComponent,
    AuthSessionLangComponent,
    AuthSessionPasscheckComponent
  ]
})
export class AuthSessionModule
{
}
