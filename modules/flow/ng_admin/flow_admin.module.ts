import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { FlowEditorComponent } from './flow-editor/flow-editor.component';
import { FlowListComponent } from './flow-list/flow-list.component';
import { FlowProcessComponent } from './flow-process/flow-process.component';
import { AuthServiceModule } from '@qbus/auth_service.module';

const routes: Routes = [
  { path: 'flow_editor', component: FlowEditorComponent },
  { path: 'flow_editor/:wfid', component: FlowListComponent },
  { path: 'flow_process', component: FlowProcessComponent }
];

@NgModule({
  declarations: [ FlowListComponent, FlowEditorComponent, FlowProcessComponent],
  imports: [CommonModule, FormsModule, AuthServiceModule, RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class FlowAdminModule
{
}
