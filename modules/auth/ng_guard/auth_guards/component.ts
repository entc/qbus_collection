import { Directive, OnInit, Injectable } from '@angular/core';
import { ActivatedRoute, Router, EventType, ActivatedRouteSnapshot, RouterStateSnapshot } from '@angular/router';
import { AuthSession, AuthSessionItem, ConnStatus } from '@qbus/auth_session';
import { TrloService } from '@qbus/trlo_service/service';
import { ConnService } from '@conn/conn_service';

//-----------------------------------------------------------------------------

@Directive({selector: '[authSite]'}) export class AuthSiteDirective implements OnInit {

  private route_subscription;
  private session_subscription;
  private conn_subscriber = null;

  private conn_connected: boolean = false;

  private nurl: string;
  private lang: string;

  //---------------------------------------------------------------------------

  constructor (private conn_service: ConnService, private trlo_service: TrloService, private auth_session: AuthSession, public router: Router, private route: ActivatedRoute)
  {
    this.route_subscription = this.router.events.subscribe(evt => {

      // only change routes if the connection service is available
      // -> connection service guards might change routes as well
      if ((evt['type'] == EventType.NavigationStart))
      {
        console.log('route status changed = ' + evt['url']);

        // extract the language query parameter
        this.lang = this.route.snapshot.queryParams["lang"];

        // get the destination url
        this.nurl = evt['url'];

        if (this.conn_connected)
        {
          if (this.has_login_url ())
          {
            this.handle_login_route ();
          }
          else
          {
            this.handle_default_route ();
          }
        }
        else
        {
          if (this.has_conn_url ())
          {

          }
          else
          {
            this.handle_conn_route (this.nurl);
          }
        }
      }
    });
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
  }

  //---------------------------------------------------------------------------

  ngOnDestroy()
  {
    this.conn_subscriber.unsubscribe();
    this.route_subscription.unsubscribe();
    this.session_subscription.unsubscribe();
  }

  //---------------------------------------------------------------------------

  private has_login_url (): boolean
  {
    // use regex to check if the url begins with /login
    return /^\/login(\?.*)?$/.test (this.nurl);
  }

  //---------------------------------------------------------------------------

  private has_conn_url (): boolean
  {
    // use regex to check if the url begins with /login
    return /^\/conn-not(\?.*)?$/.test (this.nurl);
  }

  //---------------------------------------------------------------------------

  private handle_login_route ()
  {
    // debug output
    console.log('router: lang = ' + this.lang + ', already at login');

    this.session_subscription = this.auth_session.session.subscribe ((sitem: AuthSessionItem) => {

      if (null == sitem)
      {
        if (this.lang)
        {
          this.trlo_service.updateLocale (this.lang);
        }
        else
        {
          this.trlo_service.update_from_browser ();
        }
      }
      else   // fresh login
      {
        this.router.navigate([''], {});
      }

    });
  }

  //---------------------------------------------------------------------------

  private handle_default_route ()
  {
    // debug output
    console.log('router: lang = ' + this.lang);

    this.session_subscription = this.auth_session.session.subscribe ((sitem: AuthSessionItem) => {

      // if null there was no valid login
      if (null == sitem)
      {
        console.log('forward to login');

        this.router.navigate(['login'], {queryParams : {lang: this.lang}});
      }
      else  // is logged in
      {
        if (this.lang)
        {
          this.trlo_service.updateLocale (this.lang);
        }
      }

    });
  }

  //---------------------------------------------------------------------------

  private handle_conn_route (url: string)
  {
    this.router.navigate(['conn-not'], {queryParams : {lang: this.lang}});

    this.conn_subscriber = this.conn_service.status.subscribe ((status: ConnStatus) => {

      console.log('conn status changed = ' + status.connected);

      this.conn_connected = status.connected;

      if (false == this.conn_connected)
      {
        if (this.has_conn_url ())
        {

        }
        else
        {
          this.router.navigate(['conn-not'], {queryParams : {lang: this.lang}});
        }
      }
      else
      {
        this.router.navigate([url], {});
      }

    });
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
