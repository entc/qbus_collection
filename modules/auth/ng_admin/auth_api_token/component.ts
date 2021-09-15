import { Component } from '@angular/core';
import { AuthSession, AuthSessionItem } from '@qbus/auth_session';

//-----------------------------------------------------------------------------

@Component({
  selector: 'auth-api-token',
  templateUrl: './component.html'
})
export class AuthApiTokenComponent {

  public token: string;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession)
  {
  }

  //-----------------------------------------------------------------------------

  get_session_token ()
  {
    this.auth_session.json_rpc ('AUTH', 'session_add', {type: 2}).subscribe ((data: AuthSessionItem) => {

      this.token = data.token;

    }, () => {

    });
  }

}

//-----------------------------------------------------------------------------
