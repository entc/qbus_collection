import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';
import { HttpClientModule } from '@angular/common/http';
import { AuthSession, AuthSessionLoginModalComponent, AuthWorkspacesModalComponent, AuthSessionComponentDirective, AuthSessionLoginComponent } from '@qbus/auth_session';
import { AuthSessionMenuComponent, AuthSessionInfoModalComponent, AuthSessionRoleDirective } from './component';
import { AuthSessionGuard } from '@qbus/auth_session/route';

@NgModule({
  declarations: [
    AuthWorkspacesModalComponent,
    AuthSessionLoginModalComponent,
    AuthSessionMenuComponent,
    AuthSessionComponentDirective,
    AuthSessionLoginComponent,
    AuthSessionInfoModalComponent,
    AuthSessionRoleDirective
  ],
  providers: [
    AuthSession,
    AuthSessionGuard
  ],
  imports: [
    CommonModule,
    FormsModule,
    TrloModule,
    HttpClientModule
  ],
  entryComponents: [
    AuthWorkspacesModalComponent,
    AuthSessionLoginModalComponent,
    AuthSessionLoginComponent,
    AuthSessionInfoModalComponent
  ],
  exports: [
    AuthSessionMenuComponent,
    AuthSessionRoleDirective
  ]
})
export class AuthSessionModule
{
}
