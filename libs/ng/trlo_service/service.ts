import { Component, Injectable, Injector, Pipe, PipeTransform, Input } from '@angular/core';
import { formatDate } from '@angular/common';
import { TranslocoService } from '@ngneat/transloco';
import { Observable, BehaviorSubject } from 'rxjs';
//import { NgbDatepickerI18n, NgbCalendar, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';

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

  public datetime_to_h (datetime_in_ms: number)
  {
    if (datetime_in_ms < 1000)
    {
      return Math.floor (datetime_in_ms) + ' ' + this.translate('DATE.MS');
    }
    else if (datetime_in_ms < 60000)
    {
      return Math.floor (datetime_in_ms / 1000) + ' ' + this.translate('DATE.SECONDS');
    }
    else if (datetime_in_ms < 3600000)
    {
      var min = Math.floor (datetime_in_ms / 60000);
      var sec = Math.floor ((datetime_in_ms - min * 60000) / 1000);
      return min + ':' + String(sec).padStart(2 ,"0") + ' ' + this.translate('DATE.MINUTES');
    }
    else if (datetime_in_ms < 86400000)
    {
      var hrs = Math.floor (datetime_in_ms / 3600000);
      var min = Math.floor ((datetime_in_ms - hrs * 3600000) / 60000);

      return hrs + ':' + String(min).padStart(2 ,"0") + ' ' + this.translate('DATE.HOURS');
    }
    else
    {
      return Math.floor (datetime_in_ms / 86400000) + ' ' + this.translate('DATE.DAYS');
    }
  }

  //---------------------------------------------------------------------------

  public diff_date__future (date_in_iso: string): string
  {
    var time = new Date(date_in_iso).getTime() - new Date().getTime();

    if (time < 0)
    {
      return this.translate('DATE.NAT');
    }
    else
    {
      return this.datetime_to_h (time);
    }
  }

  //---------------------------------------------------------------------------

  public diff_date__past (date_in_iso: string): string
  {
    var time = new Date().getTime() - new Date(date_in_iso).getTime();

    if (time < 0)
    {
      return this.translate('DATE.NAT');
    }
    else
    {
      return this.datetime_to_h (time);
    }
  }

  //---------------------------------------------------------------------------

  public time_date (datetime: number)
  {
    return this.datetime_to_h (datetime);
  }

  //---------------------------------------------------------------------------

  public translate (val: string): string
  {
    return this.translocoService.translate(val);
  }
}

//=============================================================================

@Component({
  selector: 'trlo-selector',
  templateUrl: './component.html',
  styleUrls: ['./component.scss']
}) export class TrloServiceComponent {

  public locale: BehaviorSubject<string>;

  // this member is needed for the select html element
  public _locales;

  //---------------------------------------------------------------------------

  @Input() set locales (locales)
  {
    this._locales = locales;
  }

  //---------------------------------------------------------------------------

  constructor (private trlo_service: TrloService)
  {
    this._locales = this.trlo_service.locales;
  }

  //---------------------------------------------------------------------------

  public set_locale (e: Event)
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

  transform (value: string, in_the_future: boolean = false): string
  {
    if (value)
    {
      return in_the_future ? this.trlo_service.diff_date__future (value) : this.trlo_service.diff_date__past (value);
    }
    else
    {
      return '';
    }
  }
}

//=============================================================================

@Pipe({name: 'trlo_time_ms'})
export class TrloPipeTime implements PipeTransform {

  constructor (private trlo_service: TrloService)
  {
  }

  //---------------------------------------------------------------------------

  transform (value_in_ms: string): string
  {
    if (value_in_ms)
    {
      return this.trlo_service.time_date (Number(value_in_ms));
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

//=============================================================================

/*
@Injectable()
export class TranslocoDatepicker extends NgbDatepickerI18n
{
  constructor(private trlo_service: TrloService) { super(); }

  // old API
  getWeekdayShortName (weekday: number): string
  {
    return this.getWeekdayLabel (weekday);
  }

  // new API
  getWeekdayLabel(weekday: number): string
  {
    return this.trlo_service.translate('DATE.WEEKDAY_' + weekday);
  }

  // new API
  getWeekLabel (): string
  {
    return this.trlo_service.translate('DATE.WEEK');
  }

  getMonthShortName(month: number): string
  {
    return this.trlo_service.translate('DATE.MONTH_SHORT_' + month);
  }

  getMonthFullName(month: number): string
  {
    return this.trlo_service.translate('DATE.MONTH_LONG_' + month);
  }

  getDayAriaLabel(date: NgbDateStruct): string
  {
    return `${date.day}-${date.month}-${date.year}`;
  }
}
*/
