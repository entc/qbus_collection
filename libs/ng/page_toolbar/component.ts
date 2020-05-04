import { Component, OnInit, SimpleChanges, Input, ViewChild } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';
import { NgForm } from '@angular/forms';
import { Pipe, PipeTransform } from '@angular/core';
import { PaginatePipe, PaginationService } from 'ngx-pagination';

//-----------------------------------------------------------------------------

@Component({
  selector: 'page-toolbar',
  templateUrl: './component.html'
})
export class PageToolbarComponent implements OnInit {

  @Input('sorting') sort_expr: string;
  public sort_vals: object;
  public sort_keys: string[];

  // for sorting and pagination
  currentPage: number;
  sort_name = undefined;
  sort_reverse: boolean = true;
  search: string = undefined;

  //-----------------------------------------------------------------------------

  constructor()
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.sort_vals = eval('(' + this.sort_expr + ')');
    this.sort_keys = Object.keys(this.sort_vals);

    this.sort_name = this.sort_vals[this.sort_keys[0]];
  }

  //-----------------------------------------------------------------------------

  reset_search ()
  {
    this.search = undefined;
  }

  //-----------------------------------------------------------------------------

}

//-----------------------------------------------------------------------------

@Pipe({
  name: "pageToolBar",
  pure: false
})
export class PageToolbarPipe implements PipeTransform {

  pagination: PaginatePipe;
  sort: QbngSortPipe;
  filter: QbngFilterPipe;

  constructor(private pagination_service: PaginationService)
  {
    this.pagination = new PaginatePipe(pagination_service);
    this.sort = new QbngSortPipe;
    this.filter = new QbngFilterPipe;
  }

  //---------------------------------------------------------------------------

  transform (items: any[], instance: PageToolbarComponent, filters: string[]): any[] {

    var items = this.filter.transform (items, instance.search, filters);

    items = this.sort.transform (items, instance.sort_name, instance.sort_reverse);

    return this.pagination.transform (items, { itemsPerPage: 30, currentPage: instance.currentPage });
  }

}

//-----------------------------------------------------------------------------

@Pipe({
  name: "qbngSort"
})
export class QbngSortPipe implements PipeTransform {
  transform(items: any[], field: string, reverse: boolean = false): any[] {
    if (!items) return [];

    if (field) items.sort((a, b) => (a[field] > b[field] ? 1 : -1));
    else items.sort((a, b) => (a > b ? 1 : -1));

    if (reverse) items.reverse();

    return items;
  }
}

//-----------------------------------------------------------------------------

@Pipe({
  name: 'qbngFilter'
})
export class QbngFilterPipe implements PipeTransform {
  transform(items: any[], searchText: string, filters: string[]): any[] {
    if (!items) return [];
    if (!searchText) return items;

    searchText = searchText.toLowerCase();

    let filteredItems = [];

    items.forEach(item => {
      filters.forEach(filter => {
        if (item[filter] && item[filter].toLowerCase().includes(searchText)) {
          filteredItems.push(item);
        }
      })
    })
    return filteredItems;
  }
}

//-----------------------------------------------------------------------------
