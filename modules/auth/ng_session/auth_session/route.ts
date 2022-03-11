import { Injectable } from '@angular/core';
import { Observable, BehaviorSubject, of } from 'rxjs';
import { map, mergeMap } from 'rxjs/operators'
import { AuthSession } from '@qbus/auth_session';
import { CanActivate, Routes, ActivatedRouteSnapshot, RouterStateSnapshot, UrlTree } from '@angular/router';

//=============================================================================

@Injectable()
export class AuthSessionGuard implements CanActivate {

  private permissions: string[] = null;

  //---------------------------------------------------------------------------

  constructor (private auth_session: AuthSession)
  {
  }

  //---------------------------------------------------------------------------

  canActivate (route: ActivatedRouteSnapshot, state: RouterStateSnapshot): Observable<boolean|UrlTree>|Promise<boolean|UrlTree>|boolean|UrlTree
  {
    // grab the permission array
    this.permissions = route.data['permissions'] as string[];

    return this.auth_session.roles.pipe(mergeMap ((data) => {

      let new_status: boolean = this.auth_session.contains_role__or (data, this.permissions);

      return of (new_status);

    })) as Observable<boolean>;
  }
}

//=============================================================================
