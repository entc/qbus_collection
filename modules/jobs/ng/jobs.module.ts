import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { JobsListComponent } from './jobs_list/component';

const routes: Routes = [
  { path: 'jobs_list', component: JobsListComponent }
];

@NgModule({
  imports: [CommonModule, FormsModule, RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class JobsModule
{
}
