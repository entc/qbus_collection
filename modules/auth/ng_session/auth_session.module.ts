import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';
import { HttpClientModule } from '@angular/common/http';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { AuthSession, AuthSessionLoginModalComponent, AuthWorkspacesModalComponent, AuthSessionComponentDirective, AuthSessionLoginComponent, AuthFirstuseModalComponent } from '@qbus/auth_session';
import { AuthSessionMenuComponent, AuthSessionInfoModalComponent, AuthSessionRoleDirective, AuthSessionContentComponent, AuthSessionPassResetComponent, AuthSessionMsgsComponent } from './component';
import { AuthSessionGuard } from '@qbus/auth_session/route';

//=============================================================================

@NgModule({
  declarations: [
    AuthWorkspacesModalComponent,
    AuthFirstuseModalComponent,
    AuthSessionLoginModalComponent,
    AuthSessionMenuComponent,
    AuthSessionComponentDirective,
    AuthSessionLoginComponent,
    AuthSessionInfoModalComponent,
    AuthSessionRoleDirective,
    AuthSessionContentComponent,
    AuthSessionPassResetComponent,
    AuthSessionMsgsComponent
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
  entryComponents: [
    AuthWorkspacesModalComponent,
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
    AuthSessionMsgsComponent
  ]
})
export class AuthSessionModule
{
}
