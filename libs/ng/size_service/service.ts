import { Component, OnInit, SimpleChanges, Injectable } from '@angular/core';
import { NgbModal, NgbActiveModal, NgbTimeStruct, NgbDateStruct } from '@ng-bootstrap/ng-bootstrap';
import { Observable, BehaviorSubject } from 'rxjs';
import { distinctUntilChanged } from 'rxjs/operators';

//-----------------------------------------------------------------------------

export enum SCREEN_SIZE {
  XS,
  SM,
  MD,
  LG,
  XL
}

//-----------------------------------------------------------------------------

@Injectable()
export class SizeService
{
  private resize_subject: BehaviorSubject<SCREEN_SIZE>;

  //-----------------------------------------------------------------------------

  constructor()
  {
    this.resize_subject = new BehaviorSubject(SCREEN_SIZE.XL);
  }

  //-----------------------------------------------------------------------------

  public get (): Observable<SCREEN_SIZE>
  {
    return this.resize_subject.asObservable().pipe(distinctUntilChanged());
  }

  //-----------------------------------------------------------------------------

  public set (size: SCREEN_SIZE)
  {
    this.resize_subject.next(size);
  }

  //-----------------------------------------------------------------------------

  public is_mobile (size: SCREEN_SIZE): boolean
  {
    switch (size)
    {
      case SCREEN_SIZE.XS:
      case SCREEN_SIZE.SM:
      case SCREEN_SIZE.MD:
      {
        return true;
      }
      default: return false;
    }
  }
}

//-----------------------------------------------------------------------------
