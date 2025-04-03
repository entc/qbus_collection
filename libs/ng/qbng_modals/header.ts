export class QbngErrorHolder
{
  code: number;
  text: string;
  data: object;
}

export class QbngOptionHolder
{
  /*
  header_text: string;
  body_text: string;
  button_text: string;
*/
  constructor (public header_text: string, public body_text: string, public button_text: string)
  {
  }
}
