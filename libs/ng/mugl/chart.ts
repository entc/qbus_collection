import { ElementRef, TemplateRef, EmbeddedViewRef, Renderer2, HostListener, EventEmitter } from '@angular/core';

//-----------------------------------------------------------------------------

export class Chart {

  //-----------------------------------------------------------------------------

  constructor (private el_dom: ElementRef, private rd: Renderer2, private data: number[])
  {
    const dom_el = this.el_dom.nativeElement;

    const xmlns = "http://www.w3.org/2000/svg";
    const svgElement = document.createElementNS (xmlns, "svg");

    this.rd.addClass (svgElement, 'chart-display');

    svgElement.setAttributeNS(null, "viewBox", `${0} ${0} ${31} ${40}`);

    const pathElement = document.createElementNS (xmlns, "path");

    var path = 'M';
    var len = data.length;

    for (let i = 0; i < len; i++)
    {
      const p: number = data[i];
//      path = path + `${len - i} ${10 - p}${i == (len - 1) ? "Z" : "L"}`;
      path = path + `${len - i} ${10 - p}${"L"}`;
    }

    pathElement.setAttributeNS(null, "d", path);
    pathElement.setAttributeNS(null, "stroke", "blue");
    pathElement.setAttributeNS(null, "fill", "none");
//    pathElement.setAttributeNS(null, "stroke-width", "1px");
//    pathElement.setAttributeNS(null, "vector-effect", "non-scaling-stroke");

    svgElement.appendChild (pathElement);

    this.rd.appendChild (dom_el, svgElement);
  }

}
