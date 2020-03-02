import { Component, OnInit } from '@angular/core';
import { Observable } from 'rxjs';

// auth service
import { ConfigService, PageReloadService } from '@qbus/auth.service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow',
  templateUrl: './component.html',
  styleUrls: ['./component.scss'],
  providers: [ConfigService, PageReloadService]
})
export class AuthLoginsComponent implements OnInit {

  login_items: Observable<Array<LoginItem>>;

  //-----------------------------------------------------------------------------

  constructor (private configService: ConfigService)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.login_items = this.configService.json_get ('AUTH', 'ui_login_get', {});
  }

}

//-----------------------------------------------------------------------------

class LoginItem {

  public ltime: string;
  public workspace: string;
  public ip: string;
  public info: object;

}
