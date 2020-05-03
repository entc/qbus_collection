import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { FlowEditorComponent, FlowEditorRmModalComponent, FlowEditorAddModalComponent } from './flow_editor/component';
import { FlowWorkstepsComponent } from './flow_worksteps/component';
import { FlowProcessComponent } from './flow_process/component';
import { AuthServiceModule } from '@qbus/auth_service.module';
import { PageToolbarModule } from '@qbus/page_toolbar.module';

const routes: Routes = [
  { path: 'flow_editor', component: FlowEditorComponent },
  { path: 'flow_editor/:wfid', component: FlowWorkstepsComponent },
  { path: 'flow_process', component: FlowProcessComponent }
];

@NgModule({
  declarations: [ FlowWorkstepsComponent, FlowEditorComponent, FlowProcessComponent, FlowEditorRmModalComponent, FlowEditorAddModalComponent],
  imports: [CommonModule, FormsModule, PageToolbarModule, AuthServiceModule, RouterModule.forChild(routes)],
  exports: [RouterModule],
  entryComponents: [ FlowEditorRmModalComponent, FlowEditorAddModalComponent ]
})
export class FlowAdminModule
{
}
