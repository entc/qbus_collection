import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { AuthLoginsComponent } from './auth_logins/component';

const routes: Routes = [
  { path: 'auth_logins', component: AuthLoginsComponent }
];

@NgModule({
  declarations: [AuthLoginsComponent],
  imports: [CommonModule, FormsModule, RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class AuthAdminModule
{
}
