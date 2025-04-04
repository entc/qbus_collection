import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { HttpClientModule } from '@angular/common/http';
import { ConnService } from './conn_service';

//---------------------------------------------------------------------------

@NgModule({
    providers: [
        ConnService
    ],
    imports: [
        CommonModule,
        HttpClientModule
    ]
})
export class ConnModule
{
}

//---------------------------------------------------------------------------
