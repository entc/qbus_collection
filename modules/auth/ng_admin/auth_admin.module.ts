import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { AuthLoginsComponent } from './auth_logins/component';
import { NgxPaginationModule } from 'ngx-pagination';
import { TrloModule } from '@qbus/trlo.module';
import { QbngModule } from '@qbus/qbng.module';

const routes: Routes = [
  { path: 'auth_logins', component: AuthLoginsComponent }
];

@NgModule({
  declarations: [AuthLoginsComponent],
  imports: [CommonModule, FormsModule, NgxPaginationModule, TrloModule, QbngModule, RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class AuthAdminModule
{
}
