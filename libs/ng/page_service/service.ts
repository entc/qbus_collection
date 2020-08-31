import { Component, OnInit, SimpleChanges, Injectable } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable } from 'rxjs';

//-----------------------------------------------------------------------------

@Injectable()
export class PageService
{

  //-----------------------------------------------------------------------------

  constructor()
  {
  }

  //-----------------------------------------------------------------------------

  get (name: string): string
  {
    return sessionStorage.getItem (name);
  }

  //-----------------------------------------------------------------------------

  set (name: string, value: string)
  {
    sessionStorage.setItem (name, value);
  }

}

//-----------------------------------------------------------------------------
