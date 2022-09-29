import { Component, OnInit, SimpleChanges, Input, ViewChild, Directive, Self } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { NgForm, NgControl, NgModel } from '@angular/forms';
import { PaginatePipe, PaginationService } from 'ngx-pagination';
import { PageService } from '@qbus/page_service/service';
import { Pipe, PipeTransform } from '@angular/core';
import { Observable, Subject, BehaviorSubject } from 'rxjs';
import { map, filter } from 'rxjs/operators'

//-----------------------------------------------------------------------------

@Component({
  selector: 'page-toolbar',
  templateUrl: './component.html'
})
export class PageToolbarComponent implements OnInit {

  @Input('sorting') sort_expr: string;
  @Input('recall') recall: string;
  public sort_vals: object;
  public sort_keys: string[];

  // for sorting and pagination
  currentPage: number;
  sort_name = undefined;
  sort_reverse: boolean = true;

  //public search: BehaviorSubject<string>;
  public search: string;

  //-----------------------------------------------------------------------------

  constructor (private page_service: PageService)
  {
    //this.search = new BehaviorSubject<string>(undefined);
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.sort_vals = eval('(' + this.sort_expr + ')');
    this.sort_keys = Object.keys(this.sort_vals);

    this.sort_name = this.sort_vals[this.sort_keys[0]];

    if (this.recall)
    {
      this.search = this.page_service.get (this.recall);

      /*
      this.search.subscribe ((search) => {

        this.page_service.set (this.recall, search);

      });
      */
    }
  }

  //-----------------------------------------------------------------------------

  on_search_reset ()
  {
    //this.search.next (undefined);
    this.search = undefined;
    this.page_service.set (this.recall, '');
  }

  //-----------------------------------------------------------------------------

  on_search_changed ()
  {
    this.page_service.set (this.recall, this.search);
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

  transform (items: any[], instance: PageToolbarComponent, filters: string[]) {

    /*
    return instance.search.pipe (map ((search) => {

      var h = this.filter.transform (items, search, filters);

      h = this.sort.transform (h, instance.sort_name, instance.sort_reverse);

      return this.pagination.transform (h, { itemsPerPage: 30, currentPage: instance.currentPage });

//        this.pagination.transform (this.sort.transform (this.filter.transform (items, data, filters), instance.sort_name, instance.sort_reverse), { itemsPerPage: 30, currentPage: instance.currentPage })

    }));
    */

    var items = this.filter.transform (items, instance.search, filters);

    items = this.sort.transform (items, instance.sort_name, instance.sort_reverse);

    return this.pagination.transform (items, { itemsPerPage: 30, currentPage: instance.currentPage });
  }

}

//-----------------------------------------------------------------------------

@Pipe({
  name: "qbngSort"
})
export class QbngSortPipe implements PipeTransform
{
  private on_compare (a: string, b: string)
  {
    if (a)
    {
      if (b)
      {
        return a > b ? 1 : -1;
      }
      else
      {
        return 1;
      }
    }
    else
    {
      return -1;
    }
  }

  transform(items: any[], field: string, reverse: boolean = false): any[]
  {
    if (!items) return [];

    if (field)
    {
      if (reverse)
      {
        items.sort((a, b) => this.on_compare (b[field], a[field]));
      }
      else
      {
        items.sort((a, b) => this.on_compare (a[field], b[field]));
      }
    }
    else
    {
      if (reverse)
      {
        items.sort((a, b) => this.on_compare (b, a));
      }
      else
      {
        items.sort((a, b) => this.on_compare (a, b));
      }
    }

    return items;
  }
}

//-----------------------------------------------------------------------------

@Pipe({
  name: 'qbngFilter'
})
export class QbngFilterPipe implements PipeTransform
{

  transform(items: any[], searchText: string, filters: string[]): any[]
  {
    if (!items) return [];
    if (!searchText) return items;

    searchText = searchText.toLowerCase();

    let filteredItems = [];

    items.forEach(item => {
      filters.forEach(filter => {

        const item_var = item[filter];

        if (item_var)
        {
          if (typeof item_var == 'number')
          {
            if (item_var === Number(searchText))
            {
              filteredItems.push(item);
            }
          }
          else
          {
            if (item_var.toLowerCase().includes(searchText))
            {
              filteredItems.push(item);
            }
          }
        }
      })
    })
    return filteredItems;
  }
}

//=============================================================================

@Pipe({name: 'qbngFile'})
export class QbngFSPipeFile implements PipeTransform {

  transform (filepath: string): string
  {
    if (filepath)
    {
      return filepath.split('\\').pop().split('/').pop();
    }
    else
    {
      return '';
    }
  }
}

//-----------------------------------------------------------------------------

@Directive({
  selector: '[asyncModel]'
})
export class RxModelDirective {

  /**
   * Reference to the behaviorSubject, because during the first change we will transform
   * the behaviorSubject to its value in the ngModel
   */
  behaviorSubjectReference: BehaviorSubject<any>;

  constructor( @Self() private ngControl: NgControl) { }


  ngOnInit() {

    if ( !(this.ngControl instanceof NgModel ) ) {
        // If the ngControl is not an instanceof ngModel, return early
        return;
    }

    const ngControlValue = this.ngControl.control.value;

    this.ngControl.valueChanges.pipe(
      filter( value => {
        return value instanceof BehaviorSubject || Boolean(this.behaviorSubjectReference);
      })
    ).subscribe( (value: BehaviorSubject<any>|any) => {

      if ( value instanceof BehaviorSubject ) {
        // Saving the behaviorSubject for later use
        //
        this.behaviorSubjectReference = value;
        this.ngControl.control.setValue(this.behaviorSubjectReference.value);
      } else {
        // If we are in the else clause, the first change has already gone by and we have a
        // behaviorSubjectReference we can call .next on
        //
        this.behaviorSubjectReference.next(value);
      }

    });
  }
}

//-----------------------------------------------------------------------------
