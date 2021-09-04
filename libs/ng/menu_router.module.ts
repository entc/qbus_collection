import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { NgbModule } from '@ng-bootstrap/ng-bootstrap';
import { TrloModule } from '@qbus/trlo.module';
import { SizeService } from './size_service/service';
import { RouterModule } from '@angular/router';
import { AuthSessionModule } from '@qbus/auth_session.module';
import { MenuRouterComponent, MenuRouterModalComponent } from './menu_router/component';

@NgModule({
  declarations: [
    MenuRouterComponent,
    MenuRouterModalComponent
  ],
  imports: [
    CommonModule,
    NgbModule,
    TrloModule,
    RouterModule,
    AuthSessionModule
  ],
  exports: [
    MenuRouterComponent
  ],
  entryComponents: [
    MenuRouterModalComponent
  ]
})
export class MenuRouterModule
{
}
