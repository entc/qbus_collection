import { Component, OnInit, Injectable } from '@angular/core';
import { Observable } from 'rxjs';
import { HttpClient } from '@angular/common/http';
// auth service
import { AuthSession } from '@qbus/auth_session';

//-----------------------------------------------------------------------------

@Component({
  selector: 'app-flow',
  templateUrl: './component.html'
})
export class AuthLoginsComponent implements OnInit {

  login_items: LoginItem[];

  // for sorting and pagination
  currentPage: number;
  filter = "name";
  reverse: boolean = false;

  //-----------------------------------------------------------------------------

  constructor (private auth_session: AuthSession, private map_service: MapsService)
  {
  }

  //-----------------------------------------------------------------------------

  ngOnInit()
  {
    this.auth_session.json_rpc ('AUTH', 'ui_login_get', {lp: "D5"}).subscribe ((data: LoginItem[]) => {

      for (var i in data)
      {
        const item: LoginItem = data[i];

        const ip_parts = item.ip.split('.');

        if (ip_parts.length == 4 && item.ip != '0.0.0.0')
        {
          this.map_service.getIPInfo (item.ip).subscribe (data => {

            item.city = data.city;
            item.country_name = data.country_name;

          });
        }
      }

      this.login_items = data;

    });
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
  public city: string;
  public country_name: string;
}

//-----------------------------------------------------------------------------

interface IPInfo {
	latitude: string;
	longitude: string;
	country_name: string;
	city: string;
}

//-----------------------------------------------------------------------------

@Injectable({
  providedIn: 'root'
})
export class MapsService {

  constructor (private http: HttpClient)
  {
  }

	getIPInfo (ip: string)
  {
		return this.http.get<IPInfo>('https://ipapi.co/' + ip + '/json/');
	}
}

//-----------------------------------------------------------------------------
