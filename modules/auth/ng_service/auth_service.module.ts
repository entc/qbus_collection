import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

// auth services
import { AuthService } from '@qbus/auth.service';
import { AuthLoginModalComponent, AuthLogoutModalComponent, AuthWorkspacesModalComponent, AuthServiceComponent } from '@qbus/auth.service';
import { AuthLoginsComponent } from '@qbus/auth_logins/component';

@NgModule({
  declarations: [
    AuthLoginModalComponent,
    AuthLogoutModalComponent,
    AuthWorkspacesModalComponent,
    AuthServiceComponent,
    AuthLoginsComponent
  ],
  providers: [AuthService],
  imports: [CommonModule, FormsModule],
  entryComponents: [
    AuthLoginModalComponent,
    AuthLogoutModalComponent,
    AuthWorkspacesModalComponent
  ],
  exports: [AuthServiceComponent]
})
export class AuthServiceModule
{
}
