import { Component, OnInit } from '@angular/core';
import { Observable } from 'rxjs';

// auth service
import { AuthService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow',
  templateUrl: './component.html',
  styleUrls: ['./component.scss']
})
export class AuthLoginsComponent implements OnInit {

  login_items: Observable<Array<LoginItem>>;

  // for sorting and pagination
  currentPage: number;
  filter = "name";
  reverse: boolean = false;

  //-----------------------------------------------------------------------------

  constructor (private AuthService: AuthService)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.login_items = this.AuthService.json_rpc ('AUTH', 'ui_login_get', {});
  }

  //-----------------------------------------------------------------------------

  sorting (value: boolean)
  {
    this.reverse = value;
  }

}

//-----------------------------------------------------------------------------

class LoginItem {

  public ltime: string;
  public workspace: string;
  public ip: string;
  public info: object;

}
