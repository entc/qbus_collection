import { Directive, OnInit, Injectable } from '@angular/core';
import { ActivatedRoute, Router, EventType, ActivatedRouteSnapshot, RouterStateSnapshot } from '@angular/router';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';
import { TrloService } from '@qbus/trlo_service/service';
import { ConnService } from '@conn/conn_service';

//-----------------------------------------------------------------------------

@Directive({selector: '[authSite]'}) export class AuthSiteDirective implements OnInit {

  private route_subscription;
  private session_subscription;

  //---------------------------------------------------------------------------

  constructor (private conn_service: ConnService, private trlo_service: TrloService, private auth_session: AuthSession, public router: Router, private route: ActivatedRoute)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.route_subscription = this.router.events.subscribe(evt => {

      // only change routes if the connection service is available
      // -> connection service guards might change routes as well
      if ((evt['type'] == EventType.NavigationStart) && this.conn_service.is_available ())
      {
        // use regex to check if the url begins with /login
        const curl = /^\/login(\?.*)?$/.test (evt['url']);

        // extract the language query parameter
        const lang: string = this.route.snapshot.queryParams["lang"];

        // debug output
        console.log('router: lang = ' + lang + ', url = ' + curl);

        if (false == curl)
        {
          this.session_subscription = this.auth_session.session.subscribe ((sitem: AuthSessionItem) => {

            // if null there was no valid login
            if (null == sitem)
            {
              console.log('forward to login');

              this.router.navigate(['login'], {queryParams : {lang: lang}});
            }
            else
            {
              if (lang)
              {
                this.trlo_service.updateLocale (lang);
              }
            }

          });
        }
        else
        {
          // debug output
          console.log('router: lang = ' + lang + ', already at login');

          this.session_subscription = this.auth_session.session.subscribe ((sitem: AuthSessionItem) => {

            if (null == sitem)
            {
              if (lang)
              {
                this.trlo_service.updateLocale (lang);
              }
              else
              {
                this.trlo_service.update_from_browser ();
              }
            }
            else
            {
              this.router.navigate([''], {queryParams : {lang: lang}});
            }

          });
        }
      }
    });
  }

  //---------------------------------------------------------------------------

  ngOnDestroy()
  {
    this.route_subscription.unsubscribe();
    this.session_subscription.unsubscribe();
  }

  //---------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------

@Injectable({providedIn: 'root'}) export class AuthPermissionsService {

  constructor(private router: Router, private auth_session: AuthSession)
  {

  }

  canActivate (): boolean
  {
      return true;
  }
}

//-----------------------------------------------------------------------------
