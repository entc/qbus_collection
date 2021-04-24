import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { QbngModule } from '@qbus/qbng.module';
import { AuthSessionModule } from '@qbus/auth_session.module';

import { JobsListComponent, JobsAddModalComponent } from './jobs_list/component';

const routes: Routes = [
  { path: 'jobs_list', component: JobsListComponent }
];

@NgModule({
  declarations: [JobsListComponent, JobsAddModalComponent],
  imports: [CommonModule, FormsModule, NgbModule, QbngModule, AuthSessionModule, RouterModule.forChild(routes)],
  exports: [RouterModule],
  entryComponents: [JobsAddModalComponent]
})
export class JobsModule
{
}
