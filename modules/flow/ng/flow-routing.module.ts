import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Routes, RouterModule } from '@angular/router';
import { FlowEditorComponent } from './flow-editor/flow-editor.component';
import { FlowListComponent } from './flow-list/flow-list.component';

const routes: Routes = [
  { path: 'flow_editor', component: FlowEditorComponent },
  { path: 'flow_editor/:wfid', component: FlowListComponent }
];

@NgModule({
  imports: [CommonModule, FormsModule, RouterModule.forChild(routes)],
  exports: [RouterModule]
})
export class FlowRoutingModule
{
}
