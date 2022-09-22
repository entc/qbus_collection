import { Component, Injectable, Injector, Pipe, PipeTransform } from '@angular/core';
import { Observable, BehaviorSubject } from 'rxjs';

//-----------------------------------------------------------------------------

@Injectable()
export class MenuRouterService
{
  public is_toggled: BehaviorSubject<boolean> = new BehaviorSubject(true);

  //---------------------------------------------------------------------------

  constructor ()
  {
  }

  //---------------------------------------------------------------------------

  public toggle (): void
  {
    const is_toggled: boolean = this.is_toggled.value;

    this.is_toggled.next (!is_toggled);
  }
}

//-----------------------------------------------------------------------------
