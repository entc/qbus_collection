import { Component, Injectable, Injector, Pipe, PipeTransform } from '@angular/core';
import { formatDate } from '@angular/common';
import { TranslocoService } from '@ngneat/transloco';

//-----------------------------------------------------------------------------

@Injectable()
export class TrloService
{
  // TODO: this must be available from everywhere
  // -> is this possible?
  // -> having a custom pipe?
  public locale: string = 'de-DE';

  constructor()
  {
  }

  updateLocale (locale: string)
  {
    this.locale = locale;
  }
}

//=============================================================================

@Component({
  selector: 'trlo-service-component',
  templateUrl: './component.html'
}) export class TrloServiceComponent {

  // all available languages apearing in the select
  public locales = [
    { label: '🇩🇪', value: 'de-DE' },
    { label: '🇺🇸', value: 'en-US' },
    { label: '🇫🇷', value: 'fr-FR' }
  ];

  // this member is needed for the select html element
  public locale: string;

  constructor (private trlo_service: TrloService, private translocoService: TranslocoService)
  {
    this.locale = trlo_service.locale;
  }

  updateLocale (value: string)
  {
    // parse the value
    const newLocale = value.match(/^[a-zA-Z]{2}/)[0] || 'de';

    // the local local variable
    this.locale = value;

    // set the language in the i18n service
    this.translocoService.setActiveLang(newLocale);

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

  transform (value: string): string {

    return formatDate (value, 'short', this.trlo_service.locale);
  }
}
