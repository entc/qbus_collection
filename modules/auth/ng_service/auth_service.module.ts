import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';
import { HttpClientModule } from '@angular/common/http';

// auth services
import { AuthService, AuthCredential, AuthLoginModalComponent, AuthInfoModalComponent, AuthWorkspacesModalComponent, AuthServiceComponent, AuthRoleDirective } from '@qbus/auth.service';

@NgModule({
  declarations: [
    AuthLoginModalComponent,
    AuthInfoModalComponent,
    AuthWorkspacesModalComponent,
    AuthServiceComponent,
    AuthRoleDirective
  ],
  providers: [AuthService],
  imports: [CommonModule, FormsModule, TrloModule, HttpClientModule],
  entryComponents: [
    AuthLoginModalComponent,
    AuthInfoModalComponent,
    AuthWorkspacesModalComponent
  ],
  exports: [AuthServiceComponent, AuthRoleDirective]
})
export class AuthServiceModule
{
}
