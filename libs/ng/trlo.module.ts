import { HttpClient } from '@angular/common/http';
import { Injectable, NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { TRANSLOCO_LOADER, Translation, TranslocoLoader, TRANSLOCO_CONFIG, translocoConfig, TranslocoModule } from '@ngneat/transloco';
import { TrloService, TrloServiceComponent, TrloPipeLocale, TrloPipeTimediff, TrloPipeFilesize, TrloPipeTime, TranslocoDatepicker } from '@qbus/trlo_service/service';
import { NgbDatepickerI18n } from '@ng-bootstrap/ng-bootstrap';

import { registerLocaleData } from '@angular/common';
import localeFr from '@angular/common/locales/fr';
import localeDe from '@angular/common/locales/de';
import localePa from '@angular/common/locales/pa';
import localeRo from '@angular/common/locales/ro';

//-----------------------------------------------------------------------------

registerLocaleData(localeFr, 'fr-FR');
registerLocaleData(localeDe, 'de-DE');
registerLocaleData(localePa, 'hi-IN');
registerLocaleData(localePa, 'ta-IN');
registerLocaleData(localePa, 'ro-RO');

//-----------------------------------------------------------------------------

@Injectable({ providedIn: 'root' })
export class TranslocoHttpLoader implements TranslocoLoader {
  constructor(private http: HttpClient) {}

  getTranslation(lang: string) {
    return this.http.get<Translation>(`i18n/${lang}.json`);
  }
}

//-----------------------------------------------------------------------------

@NgModule({
  declarations:
  [
    TrloServiceComponent,
    TrloPipeLocale,
    TrloPipeTimediff,
    TrloPipeFilesize,
    TrloPipeTime
  ],
  imports:
  [
    CommonModule
  ],
  exports:
  [
    TrloServiceComponent,
    TrloPipeLocale,
    TrloPipeTimediff,
    TrloPipeFilesize,
    TrloPipeTime,
    TranslocoModule
  ],
  providers:
  [
    TrloService,
    {
      provide: TRANSLOCO_CONFIG,
      useValue: translocoConfig({
        availableLangs: ['de', 'en', 'fr', 'hi', 'ta', 'ro'],
        defaultLang: 'de',
        // Remove this option if your application doesn't support changing language in runtime.
        reRenderOnLangChange: true
      })
    },
    { provide: TRANSLOCO_LOADER, useClass: TranslocoHttpLoader },
    { provide: NgbDatepickerI18n, useClass: TranslocoDatepicker }
  ]
})
export class TrloModule {}

//-----------------------------------------------------------------------------
