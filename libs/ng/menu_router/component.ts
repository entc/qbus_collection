import { Component, OnInit, Input, Directive, Injector } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable, Subject, BehaviorSubject } from 'rxjs';
import { SizeService, SCREEN_SIZE } from '@qbus/size_service/service';
import { Router, ActivatedRoute } from '@angular/router';

//-----------------------------------------------------------------------------

@Component({
  selector: 'qbng-menu',
  templateUrl: './component.html'
})
export class MenuRouterComponent implements OnInit {

  public mobile_size: boolean = false;

  @Input('menu') menu_structure: MenuStructureSection[];

  //-----------------------------------------------------------------------------

  constructor (private modal_service: NgbModal, private size_service: SizeService, public route: ActivatedRoute)
  {
    this.size_service.get().subscribe (x => this.mobile_size = this.size_service.is_mobile (x));
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //-----------------------------------------------------------------------------

  open_menu()
  {
    this.modal_service.open(MenuRouterModalComponent, {ariaLabelledBy: 'modal-basic-title', injector: Injector.create([{provide: ActivatedRoute, useValue: this.route}, {provide: MenuRouterComponent, useValue: this}])}).result.then(() => {

    }, () => {

    });
  }
}

//=============================================================================

@Component({
  selector: 'qbng-success-modal-component',
  templateUrl: './modal_menu.html'
}) export class MenuRouterModalComponent implements OnInit {

  public menu_structure: MenuStructureSection[];

  //---------------------------------------------------------------------------

  constructor (public modal: NgbActiveModal, private router: Router, comp: MenuRouterComponent)
  {
    this.menu_structure = comp.menu_structure;
  }

  //---------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  follow_route(link: string)
  {
    var to = this.router.url.lastIndexOf ('/');
    var url = this.router.url.substring (0, to);

    this.router.navigate([url + '/' + link]);
    this.modal.close();
  }

}

//-----------------------------------------------------------------------------

export class MenuStructureSection
{
  constructor (public name: string | null, public auth: string[] | null, public items: MenuStructureItem[])
  {
  }
}

//-----------------------------------------------------------------------------

export class MenuStructureItem
{
  constructor (public name: string, public link: string, public icon: string, public auth: string[] | null)
  {
  }
}
