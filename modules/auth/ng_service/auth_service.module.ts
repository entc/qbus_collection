import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

// auth services
import { AuthService, AuthLoginModalComponent, AuthLogoutModalComponent, AuthWorkspacesModalComponent } from '@qbus/auth.service';

@NgModule({
  declarations: [
    AuthLoginModalComponent,
    AuthLogoutModalComponent,
    AuthWorkspacesModalComponent,
  ],
  providers: [AuthService],
  imports: [CommonModule, FormsModule],
  entryComponents: [
    AuthLoginModalComponent,
    AuthLogoutModalComponent,
    AuthWorkspacesModalComponent
  ],
})
export class AuthServiceModule
{
}
