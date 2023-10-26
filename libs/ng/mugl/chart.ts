import { ElementRef, TemplateRef, EmbeddedViewRef, Renderer2, HostListener, EventEmitter } from '@angular/core';

//-----------------------------------------------------------------------------

export class Chart {

  //-----------------------------------------------------------------------------

  constructor (private el_dom: ElementRef, private rd: Renderer2, private data: number[], miny: number = 0)
  {
    const dom_el = this.el_dom.nativeElement;

    const xmlns = "http://www.w3.org/2000/svg";
    const svgElement = document.createElementNS (xmlns, "svg");

    this.rd.addClass (svgElement, 'chart-display');

    let datastats = this.datastats (data);

    var len = data.length;

    //const textElement = document.createElementNS (xmlns, "text");
    var range: number = datastats.max - miny;

    svgElement.setAttributeNS(null, "viewBox", `${0} ${0} ${31} ${range}`);
    svgElement.setAttributeNS(null, "preserveAspectRatio", "none");

    for (let i = 0; i < len; i++)
    {
      const p: number = data[i];
      const pathElement = document.createElementNS (xmlns, "path");

      var path = `${"M"}${i} ${range}${"L"}${i} ${range - (p - miny)}${"L"}${i + 0.8} ${range - (p - miny)}${"L"}${i + 0.8} ${range}`;

      pathElement.setAttributeNS(null, "d", path);
      pathElement.setAttributeNS(null, "stroke", "blue");

      if (p > datastats.avg)
      {
        pathElement.setAttributeNS(null, "fill", "green");
      }
      else
      {
        pathElement.setAttributeNS(null, "fill", "blue");
      }

      pathElement.setAttributeNS(null, "stroke-width", "1px");
      pathElement.setAttributeNS(null, "vector-effect", "non-scaling-stroke");

      svgElement.appendChild (pathElement);
    }

    /*
    const textElement = document.createElementNS (xmlns, "text");

    textElement.setAttributeNS(null, "x", '10');
    textElement.setAttributeNS(null, "y", '10');
    pathElement.setAttributeNS(null, "vector-effect", "non-scaling-stroke");
    textElement.innerHTML = '0';

    svgElement.appendChild (textElement);
    */

    this.rd.appendChild (dom_el, svgElement);
  }

  //-----------------------------------------------------------------------------

  private datastats (data: number[]): {max: number, avg: number}
  {
    var len = data.length;

    let max: number = 0;
    let sum: number = 0;
    let cnt: number = 0;

    // determine the maximum
    for (let i = 0; i < len; i++)
    {
      const p: number = data[i];

      if (p > max)
      {
        max = p;
      }

      if (p)
      {
        cnt++;
      }

      sum = sum + p;
    }

    return {max: max, avg: sum / (cnt > 0 ? cnt : 1)};
  }

}
