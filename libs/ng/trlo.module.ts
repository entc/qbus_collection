import { HttpClient, provideHttpClient } from '@angular/common/http';
import { inject, Injectable, NgModule, isDevMode } from '@angular/core';
import { CommonModule } from '@angular/common';
import { TRANSLOCO_LOADER, Translation, TranslocoLoader, TRANSLOCO_CONFIG, translocoConfig, TranslocoModule, provideTransloco } from '@ngneat/transloco';
import { TrloService, TrloServiceComponent, TrloPipeLocale, TrloPipeTimediff, TrloPipeFilesize, TrloPipeTime, /*TranslocoDatepicker*/ } from '@qbus/trlo_service/service';

import { registerLocaleData } from '@angular/common';
import localeFr from '@angular/common/locales/fr';
import localeDe from '@angular/common/locales/de';
import localePa from '@angular/common/locales/pa';
import localeRo from '@angular/common/locales/ro';
import localeAr from '@angular/common/locales/ar';
import localeFa from '@angular/common/locales/fa';
import localeHu from '@angular/common/locales/hu';
import localeSl from '@angular/common/locales/sl';
import localePl from '@angular/common/locales/pl';

//-----------------------------------------------------------------------------

registerLocaleData(localeFr, 'fr-FR');
registerLocaleData(localeDe, 'de-DE');
registerLocaleData(localePa, 'hi-IN');
registerLocaleData(localePa, 'ta-IN');
registerLocaleData(localeRo, 'ro-RO');
registerLocaleData(localeAr, 'ar-AR');
registerLocaleData(localeFa, 'fa-FA');
registerLocaleData(localeHu, 'hu-HU');
registerLocaleData(localeSl, 'sl-SL');
registerLocaleData(localePl, 'pl-PL');

//-----------------------------------------------------------------------------

@Injectable({ providedIn: 'root' })
export class TranslocoHttpLoader implements TranslocoLoader {

  private http = inject(HttpClient);

  getTranslation(lang: string)
  {
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
    TrloService
  ]
})
export class TrloModule {}

//-----------------------------------------------------------------------------

@NgModule({
    exports: [ TranslocoModule ],
    providers: [
        provideTransloco({
            config: {
                availableLangs: ['de', 'en', 'fr', 'hi', 'ta', 'ro', 'ar', 'fa', 'hu', 'sl', 'pl'],
                defaultLang: 'en',
                // Remove this option if your application doesn't support changing language in runtime.
                reRenderOnLangChange: true,
                prodMode: !isDevMode(),
            },
            loader: TranslocoHttpLoader
        }),
    ],
})
export class TranslocoRootModule {}

//-----------------------------------------------------------------------------
