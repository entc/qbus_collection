import { HttpClient } from '@angular/common/http';
import { Injectable, NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';

import { TrloService, TrloServiceComponent, TrloPipeLocale } from '@qbus/trlo_service/service';

import { registerLocaleData } from '@angular/common';
import localeFr from '@angular/common/locales/fr';
import localeDe from '@angular/common/locales/de';

import {
  TRANSLOCO_LOADER,
  Translation,
  TranslocoLoader,
  TRANSLOCO_CONFIG,
  translocoConfig,
  TranslocoModule
} from '@ngneat/transloco';


//-----------------------------------------------------------------------------

registerLocaleData(localeFr, 'fr-FR');
registerLocaleData(localeDe, 'de-DE');

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
  declarations: [ TrloServiceComponent, TrloPipeLocale ],
  imports: [ CommonModule ],
  exports: [ TranslocoModule, TrloServiceComponent, TrloPipeLocale ],
  providers: [ TrloService,
    {
      provide: TRANSLOCO_CONFIG,
      useValue: translocoConfig({
        availableLangs: ['de', 'en', 'fr'],
        defaultLang: 'de',
        // Remove this option if your application doesn't support changing language in runtime.
        reRenderOnLangChange: true
      })
    },
    { provide: TRANSLOCO_LOADER, useClass: TranslocoHttpLoader }
  ]
})
export class TrloModule {}

//-----------------------------------------------------------------------------
