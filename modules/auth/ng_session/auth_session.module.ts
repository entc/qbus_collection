import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';
import { HttpClientModule } from '@angular/common/http';
import { AuthSession, AuthSessionLoginModalComponent, AuthWorkspacesModalComponent, AuthSessionComponentDirective, AuthSessionLoginComponent } from '@qbus/auth_session';
import { AuthSessionMenuComponent, AuthSessionInfoModalComponent } from './component';

@NgModule({
  declarations: [
    AuthWorkspacesModalComponent,
    AuthSessionLoginModalComponent,
    AuthSessionMenuComponent,
    AuthSessionComponentDirective,
    AuthSessionLoginComponent,
    AuthSessionInfoModalComponent
  ],
  providers: [
    AuthSession
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
    AuthSessionMenuComponent
  ]
})
export class AuthSessionModule
{
}
