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

  //---------------------------------------------------------------------------

  public diff_date (date_in_iso: string): string
  {
    var time = new Date().getTime() - new Date(date_in_iso).getTime();

    if (time < 1000)
    {
      return time + ' ms';
    }
    else if (time < 60000)
    {
      return Math.floor (time / 1000) + ' sec';
    }
    else if (time < 3600000)
    {
      var min = Math.floor (time / 60000);
      var sec = Math.floor ((time - min * 60000) / 1000);
      return min + ':' + String(sec).padStart(2 ,"0") + ' min';
    }
    else if (time < 86400000)
    {
      var hrs = Math.floor (time / 3600000);
      var min = Math.floor ((time - hrs * 3600000) / 60000);

      return hrs + ':' + String(min).padStart(2 ,"0") + ' hrs';
    }
    else
    {
      return Math.floor (time / 86400000) + ' days';
    }
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

  updateLocale (e: Event)
  {
    const target = e.target as HTMLSelectElement;

    if (target && target.value)
    {
      // the global locale in the service
      this.trlo_service.updateLocale (target.value)
    }
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

//=============================================================================

@Pipe({name: 'trlo_timediff'})
export class TrloPipeTimediff implements PipeTransform {

  constructor (private trlo_service: TrloService)
  {
  }

  //---------------------------------------------------------------------------

  transform (value: string): string
  {
    if (value)
    {
      return this.trlo_service.diff_date (value);
    }
    else
    {
      return '';
    }
  }
}

//=============================================================================

@Pipe({name: 'trlo_filesize'})
export class TrloPipeFilesize implements PipeTransform {

  constructor (private trlo_service: TrloService)
  {
  }

  //---------------------------------------------------------------------------

  private format (bytes: number): string
  {
    const UNITS = ['Bytes', 'kB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB'];
    const factor = 1024;
    let index = 0;

    while (bytes >= factor)
    {
      bytes /= factor;
      index++;
    }

    return parseFloat (bytes.toFixed(2)) + ' ' + UNITS[index];
  }

  //---------------------------------------------------------------------------

  transform (value: number): string
  {
    if (value)
    {
      return this.format (Number(value));
    }
    else
    {
      return '';
    }
  }
}
