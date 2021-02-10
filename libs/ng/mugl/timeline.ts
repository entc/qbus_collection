import { ElementRef, TemplateRef, EmbeddedViewRef, Renderer2 } from '@angular/core';
import { Graph } from './graph';

export class Timeline {

  private parts: TimelinePart[];

  //-----------------------------------------------------------------------------

  constructor (private el_dom: ElementRef, private el_box: TemplateRef<any>, private rd: Renderer2)
  {
    this.parts = [];
  }

  //-----------------------------------------------------------------------------

  public clear ()
  {

  }

  //-----------------------------------------------------------------------------

  private init ()
  {

  }

  //-----------------------------------------------------------------------------

  public addPart (description: string, parent: TimelinePart = null): TimelinePart
  {
    if (parent)
    {
      return parent.addPart (description);
    }
    else
    {
      var part: TimelinePart = new TimelinePart (description);

      this.parts.push (part);

      return part;
    }
  }

  //-----------------------------------------------------------------------------

  public refresh ()
  {
    for (var i in this.parts)
    {
      this.parts[i].refresh (this.el_dom, this.el_box, this.rd);
    }
  }

  //-----------------------------------------------------------------------------

}

export class TimelinePart {

  private parts: TimelinePart[];
  private graph: Graph;

  //-----------------------------------------------------------------------------

  constructor (private description: string)
  {
    this.parts = [];
    this.graph = null;
  }

  //-----------------------------------------------------------------------------

  public addPart (description: string): TimelinePart
  {
    var part: TimelinePart = new TimelinePart (description);

    this.parts.push (part);

    return part;
  }

  //-----------------------------------------------------------------------------

  public refresh (el_dom: ElementRef, el_box: TemplateRef<any>, rd: Renderer2)
  {
    const dom_el = el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    const w = elem_rect.width;
    const h = elem_rect.height;

    // calculate the width of all parts
    var parts_w = w / this.parts.length;

    if (!this.graph)
    {
      this.graph = new Graph (el_dom, el_box, rd);
      this.graph.init (600, 140, true, false);
    }

    this.graph.add_box (0, 0, 0, {head: this.description, body: 'test', foot: 'test'}, {head: '#ccc', body: '#eee', foot: '#ccc'}, null);
  }

}
