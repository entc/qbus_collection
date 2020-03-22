import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { TrloModule } from '@qbus/trlo.module';

// auth services
import { AuthService } from '@qbus/auth.service';
import { AuthLoginModalComponent, AuthInfoModalComponent, AuthWorkspacesModalComponent, AuthServiceComponent } from '@qbus/auth.service';

@NgModule({
  declarations: [
    AuthLoginModalComponent,
    AuthInfoModalComponent,
    AuthWorkspacesModalComponent,
    AuthServiceComponent
  ],
  providers: [AuthService],
  imports: [CommonModule, FormsModule, TrloModule],
  entryComponents: [
    AuthLoginModalComponent,
    AuthInfoModalComponent,
    AuthWorkspacesModalComponent
  ],
  exports: [AuthServiceComponent]
})
export class AuthServiceModule
{
}
