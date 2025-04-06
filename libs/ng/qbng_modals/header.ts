export class QbngErrorHolder
{
  constructor (public code: number, public text: string, public data: object = null)
  {
  }
}

export class QbngOptionHolder
{
  constructor (public header_text: string, public body_text: string, public button_text: string)
  {
  }
}
