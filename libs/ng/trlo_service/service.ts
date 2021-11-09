import { Component, Injectable, Injector, Pipe, PipeTransform } from '@angular/core';
import { formatDate } from '@angular/common';
import { TranslocoService } from '@ngneat/transloco';
import { Observable, BehaviorSubject } from 'rxjs';

//-----------------------------------------------------------------------------

@Injectable()
export class TrloService
{
  // TODO: this must be available from everywhere
  // -> is this possible?
  // -> having a custom pipe?
  public locale: string = 'en-US';

  // all available languages apearing in the select
  public locales = [
    { label: '🇩🇪(de)', value: 'de-DE' },
    { label: '🇺🇸(us)', value: 'en-US' }
  ];

  public lang: BehaviorSubject<string>;

  //---------------------------------------------------------------------------

  constructor (private translocoService: TranslocoService)
  {
  }

  //---------------------------------------------------------------------------

  updateLocale (value: string)
  {
    // parse the value
    const newLocale = value.match(/^[a-zA-Z]{2}/)[0] || 'en';

    console.log('set active language: ' + newLocale + ' with locale: ' + value);

    // the local local variable
    this.locale = value;

    // set the language in the i18n service
    this.translocoService.setActiveLang (newLocale);
  }

  //---------------------------------------------------------------------------

  public set (locales): void
  {
    this.locales = locales;
  }

  //---------------------------------------------------------------------------

  public format_date (date_in_iso: string, format_type: string): string
  {
    return formatDate (date_in_iso, format_type, this.locale);
  }
}

//=============================================================================

@Component({
  selector: 'trlo-service-component',
  templateUrl: './component.html',
  styleUrls: ['./component.scss']
}) export class TrloServiceComponent {

  public locale: BehaviorSubject<string>;

  // this member is needed for the select html element
  public locales;

  //---------------------------------------------------------------------------

  constructor (private trlo_service: TrloService)
  {
    this.locales = this.trlo_service.locales;
  }

  //---------------------------------------------------------------------------

  updateLocale (value: string)
  {
    // the global locale in the service
    this.trlo_service.updateLocale (value)
  }
}

//=============================================================================

@Pipe({name: 'trlo_locale'})
export class TrloPipeLocale implements PipeTransform {

  constructor (private trlo_service: TrloService)
  {
  }

  //---------------------------------------------------------------------------

  transform (value: string): string
  {
    if (value)
    {
      return this.trlo_service.format_date (value, 'short');
    }
    else
    {
      return '';
    }
  }
}
