import { ElementRef, TemplateRef, EmbeddedViewRef, Renderer2, HostListener } from '@angular/core';

//-----------------------------------------------------------------------------

const GRAPH_RESIZE_AREA       = 30;
const GRAPH_RESIZE_AREA__HALF = GRAPH_RESIZE_AREA / 2;

//-----------------------------------------------------------------------------

export class Graph {

  private on_delete_cb;
  private on_mv_cb;
  private on_edit_cb;

  private btn_mv_active: GraphBox = null;
  private btn_mv_w: number;
  private btn_mv_h: number;
  private action_ref: number;

  private mv_x: number;
  private mv_y: number;

  private grid: number;

  private option_mv_x: boolean;
  private option_mv_y: boolean;

  private boxes: GraphBox[] = [];

  //-----------------------------------------------------------------------------

  constructor (private el_dom: ElementRef, private el_box: TemplateRef<any>, private rd: Renderer2)
  {
    this.on_delete_cb = undefined;
    this.on_mv_cb = undefined;
    this.on_edit_cb = undefined;

    this.action_ref = 0;

    this.grid = 1;

    this.option_mv_x = false;
    this.option_mv_y = false;
  }

  //-----------------------------------------------------------------------------

  public init (width: number = 800, height: number = 600, move_x: boolean = true, move_y: boolean = true): void
  {
    const dom_el = this.el_dom.nativeElement;

  //  dom_el.style.width = String(width) + "px";
  //  dom_el.style.height = String(height) + "px";

    this.option_mv_x = move_x;
    this.option_mv_y = move_y;
  }

  //-----------------------------------------------------------------------------

  public clear (): void
  {
    const dom_el = this.el_dom.nativeElement;

    while (dom_el.firstChild)
    {
      this.rd.removeChild (dom_el, dom_el.lastChild);
    }
  }

  //-----------------------------------------------------------------------------

  public refresh ()
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    this.boxes.forEach ((box: GraphBox) => this.box__adjust (box, elem_rect));
  }

  //-----------------------------------------------------------------------------

  private box__adjust (box: GraphBox, elem_rect)
  {
    // change position
    this.rd.setStyle (box.dom_box, 'left', (elem_rect.width * box.x / 100) + 'px');
    this.rd.setStyle (box.dom_box, 'top', (elem_rect.height * box.y / 100) + 'px');

    // change dimension
    this.rd.setStyle (box.dom_box, 'width', (elem_rect.width * box.w / 100) + 'px');
    this.rd.setStyle (box.dom_box, 'height', (elem_rect.height * box.h / 100) + 'px');

    if (box.div_nw)
    {
      this.box__resize_buttons__adjust (box, new GraphRect (box.x, box.y, box.w, box.h, elem_rect));
    }
  }

  //-----------------------------------------------------------------------------

  private box_set_coordinates (box: GraphBox, x: number, y: number, elem_rect)
  {
    box.x = x / elem_rect.width * 100;
    box.y = y / elem_rect.height * 100;

    if (this.on_mv_cb)
    {
      this.on_mv_cb (box);
    }
  }

  //-----------------------------------------------------------------------------

  private box_set_size (box: GraphBox, x: number, y: number, w: number, h: number, elem_rect)
  {
    box.x = x / elem_rect.width * 100;
    box.y = y / elem_rect.height * 100;
    box.w = w / elem_rect.width * 100;
    box.h = h / elem_rect.height * 100;

    if (this.on_mv_cb)
    {
      this.on_mv_cb (box);
    }
  }

  //-----------------------------------------------------------------------------

  private box__enable (event, dom_el, box: GraphBox)
  {
    if (event.which === 1)
    {
      // disable all other boxes
      this.boxes.forEach ((box_loop: GraphBox) => this.box__dissable (box_loop));

      // set the enabled class
      this.rd.addClass (box.dom_box, 'box-el-enabled');

      this.box__move__on_activate (event, box);
    }
  }

  //-----------------------------------------------------------------------------

  private set_box_attributes (dom_box, names: object, colors: object)
  {
    let dom_head_elements = dom_box.querySelectorAll('.box-el-head');

    if (dom_head_elements.length > 0)
    {
      const dom_head = dom_head_elements[0];

      this.rd.setProperty (dom_head, 'innerHTML', names['head']);

      if (colors)
      {
        const color_head = colors['head'];
        if (color_head)
        {
          this.rd.setStyle (dom_head, 'color', color_head);
        }
      }
    }

    let dom_body_elements = dom_box.querySelectorAll('.box-el-body');

    if (dom_body_elements.length > 0)
    {
      const dom_body = dom_body_elements[0];

      this.rd.setProperty (dom_body, 'innerHTML', names['body']);

      if (colors)
      {
        const color_body = colors['body'];
        if (color_body)
        {
          this.rd.setStyle (dom_body, 'color', color_body);
        }
      }
    }

    let dom_foot_elements = dom_box.querySelectorAll('.box-el-foot');

    if (dom_foot_elements.length > 0)
    {
      const dom_foot = dom_foot_elements[0];

      this.rd.setProperty (dom_foot, 'innerHTML', names['foot']);

      if (colors)
      {
        const color_foot = colors['foot'];
        if (color_foot)
        {
          this.rd.setStyle (dom_foot, 'color', color_foot);
        }
      }
    }
  }

  //-----------------------------------------------------------------------------

  private add_box__append_interieur (dom_box)
  {
    let dom_head = this.rd.createElement('div');
    this.rd.addClass(dom_head, 'box-el-head');
    this.rd.appendChild (dom_box, dom_head);

    let dom_body = this.rd.createElement('div');
    this.rd.addClass(dom_body, 'box-el-body');
    this.rd.appendChild (dom_box, dom_body);

    let dom_foot = this.rd.createElement('div');
    this.rd.addClass(dom_foot, 'box-el-foot');
    this.rd.appendChild (dom_box, dom_foot);
  }

  //-----------------------------------------------------------------------------

  private box__dissable__remove (el: any, div_el: any): any
  {
    if (div_el)
    {
      this.rd.removeChild (el, div_el);
    }

    return null;
  }

  //-----------------------------------------------------------------------------

  private box__dissable (box: GraphBox): void
  {
    const dom_el = this.el_dom.nativeElement;

    this.rd.removeClass (box.dom_box, 'box-el-enabled');

    box.div_ne = this.box__dissable__remove (dom_el, box.div_ne);
    box.div_nw = this.box__dissable__remove (dom_el, box.div_nw);
    box.div_n  = this.box__dissable__remove (dom_el, box.div_n);
    box.div_w  = this.box__dissable__remove (dom_el, box.div_w);
    box.div_e  = this.box__dissable__remove (dom_el, box.div_e);
    box.div_s  = this.box__dissable__remove (dom_el, box.div_s);
    box.div_se = this.box__dissable__remove (dom_el, box.div_se);
    box.div_sw = this.box__dissable__remove (dom_el, box.div_sw);
  }

  //-----------------------------------------------------------------------------

  private box__resize_buttons__adjust (box: GraphBox, rect: GraphRect): void
  {
    this.rd.setStyle (box.div_nw, 'left', String(rect.x - GRAPH_RESIZE_AREA__HALF) + 'px');
    this.rd.setStyle (box.div_nw, 'top' , String(rect.y - GRAPH_RESIZE_AREA__HALF) + 'px');

    this.rd.setStyle (box.div_ne, 'left', String(rect.x + rect.w - GRAPH_RESIZE_AREA__HALF) + 'px');
    this.rd.setStyle (box.div_ne, 'top' , String(rect.y - GRAPH_RESIZE_AREA__HALF) + 'px');

    this.rd.setStyle (box.div_n, 'left', String(rect.x + rect.w / 2 - GRAPH_RESIZE_AREA__HALF) + 'px');
    this.rd.setStyle (box.div_n, 'top' , String(rect.y - GRAPH_RESIZE_AREA__HALF) + 'px');

    this.rd.setStyle (box.div_e, 'left', rect.x + rect.w - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle (box.div_e, 'top' , rect.y + rect.h / 2 - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle (box.div_w, 'left', rect.x - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle (box.div_w, 'top' , rect.y + rect.h / 2 - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle (box.div_s, 'left', rect.x + rect.w / 2 - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle (box.div_s, 'top' , rect.y + rect.h - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle (box.div_sw, 'left', rect.x - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle (box.div_sw, 'top' , rect.y + rect.h - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle (box.div_se, 'left', rect.x + rect.w - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle (box.div_se, 'top' , rect.y + rect.h - GRAPH_RESIZE_AREA__HALF + 'px');
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_mousedown (event, box: GraphBox, div_el: any, mx: number, my: number, sx: number, sy: number)
  {
    if (event.which === 1)
    {
      this.box__resize__on_activate (event, box, mx, my, sx, sy);
    }
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_activate (event, box: GraphBox, mx: number, my: number, sx: number, sy: number)
  {
    const dom_el = this.el_dom.nativeElement;

    // prevent any other events outside the box
    event.preventDefault();

    // calculate the delta to the current mouse position
    box.mv_x = event.clientX;
    box.mv_y = event.clientY;

    box.ev_mm = this.rd.listen (dom_el, 'mousemove', (event) => this.box__resize__on_move (event, box, mx, my, sx, sy));
    box.ev_tm = this.rd.listen (dom_el, 'touchmove', (event) => this.box__resize__on_move (event, box, mx, my, sx, sy));

    box.ev_mu = this.rd.listen (dom_el, 'mouseup', (event) => this.box__resize__on_mouseup (event, box, mx, my, sx, sy));
    box.ev_te = this.rd.listen (dom_el, 'touchend', (event) => this.box__resize__on_mouseup (event, box, mx, my, sx, sy));
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_move (event, box: GraphBox, mx: number, my: number, sx: number, sy: number)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    var rect: GraphRect = new GraphRect (box.x, box.y, box.w, box.h, elem_rect);

    rect.resize (event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect, mx, my, sx, sy);

    this.rd.setStyle (box.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle (box.dom_box, 'top', '' + rect.y + 'px');
    this.rd.setStyle (box.dom_box, 'width', '' + rect.w + 'px');
    this.rd.setStyle (box.dom_box, 'height', '' + rect.h + 'px');

    this.box__resize_buttons__adjust (box, rect);
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_mouseup (event, box: GraphBox, mx: number, my: number, sx: number, sy: number)
  {
    if (event.which === 1)
    {
      const dom_el = this.el_dom.nativeElement;

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect ();

      box.ev_mm();
      box.ev_tm();
      box.ev_mu();
      box.ev_te();

      var rect: GraphRect = new GraphRect (box.x, box.y, box.w, box.h, elem_rect);

      rect.resize (event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect, mx, my, sx, sy);

      this.box_set_size (box, rect.x, rect.y, rect.w, rect.h, elem_rect);
    }
  }

  //-----------------------------------------------------------------------------

  private box__activate_resize_buttons__append (box: GraphBox, dom_el, cursor_class: string, mx: number, my: number, sx: number, sy: number): any
  {
    let div_outer = this.rd.createElement('div');
    let div_inner = this.rd.createElement('div');

    this.rd.addClass (div_outer, 'box-el-resize');
    this.rd.setStyle (div_outer, 'cursor', cursor_class);

    this.rd.appendChild (dom_el, div_outer);

    this.rd.listen (div_outer, 'mousedown', (event) => this.box__resize__on_mousedown (event, box, dom_el, mx, my, sx, sy));
    this.rd.listen (div_outer, 'touchstart', (event) => this.box__resize__on_mousedown (event, box, dom_el, mx, my, sx, sy));

    this.rd.appendChild (div_outer, div_inner);
    this.rd.addClass (div_inner, 'box-el-resize-inner');

    return div_outer;
  }

  //-----------------------------------------------------------------------------

  private box__resize__activate (box: GraphBox): void
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    box.div_nw = this.box__activate_resize_buttons__append (box, dom_el, 'nw-resize', 1, 1, -1, -1);
    box.div_ne = this.box__activate_resize_buttons__append (box, dom_el, 'ne-resize', 0, 1, 1, -1);
    box.div_n  = this.box__activate_resize_buttons__append (box, dom_el, 'n-resize', 0, 1, 0, -1);
    box.div_e  = this.box__activate_resize_buttons__append (box, dom_el, 'e-resize', 0, 0, 1, 0);
    box.div_w  = this.box__activate_resize_buttons__append (box, dom_el, 'w-resize', 1, 0, -1, 0);
    box.div_s  = this.box__activate_resize_buttons__append (box, dom_el, 's-resize', 0, 0, 0, 1);
    box.div_sw = this.box__activate_resize_buttons__append (box, dom_el, 'sw-resize', 1, 0, -1, 1);
    box.div_se = this.box__activate_resize_buttons__append (box, dom_el, 'se-resize', 0, 0, 1, 1);

    var rect: GraphRect = new GraphRect (box.x, box.y, box.w, box.h, elem_rect);

    this.box__resize_buttons__adjust (box, rect);
  }

  //-----------------------------------------------------------------------------

  private box__move__on_activate (event, box: GraphBox)
  {
    const dom_el = this.el_dom.nativeElement;

    // prevent any other events outside the box
    event.preventDefault();

    // calculate the delta to the current mouse position
    box.mv_x = event.clientX;
    box.mv_y = event.clientY;

    box.ev_mm = this.rd.listen (dom_el, 'mousemove', (event) => this.box__move__on_move (event, box));
    box.ev_tm = this.rd.listen (dom_el, 'touchmove', (event) => this.box__move__on_move (event, box));

    box.ev_mu = this.rd.listen (dom_el, 'mouseup', (event) => this.box__move__on_mouseup (event, box));
    box.ev_te = this.rd.listen (dom_el, 'touchend', (event) => this.box__move__on_mouseup (event, box));
  }

  //-----------------------------------------------------------------------------

  private box__move__on_move (event, box: GraphBox)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    var rect: GraphRect = new GraphRect (box.x, box.y, box.w, box.h, elem_rect);

    // do a simple move transformation
    rect.move (event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect);

    this.rd.setStyle (box.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle (box.dom_box, 'top', '' + rect.y + 'px');
  }

  //-----------------------------------------------------------------------------

  private box__move__on_mouseup (event, box: GraphBox)
  {
    if (event.which === 1)
    {
      const dom_el = this.el_dom.nativeElement;

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect ();

      box.ev_mm();
      box.ev_tm();
      box.ev_mu();
      box.ev_te();

      var rect: GraphRect = new GraphRect (box.x, box.y, box.w, box.h, elem_rect);

      rect.move (event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect);

      this.box_set_coordinates (box, rect.x, rect.y, elem_rect);
    }
  }

  //-----------------------------------------------------------------------------

  public add_box (id: number, x: number, y: number, w: number, h: number, names: object): void
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    x = ((x == null) ? 0 : x);
    y = ((y == null) ? 0 : y);

    var box: GraphBox = new GraphBox;
    box.id = id;
    box.x = x;
    box.y = y;
    box.w = w;
    box.h = h;
    box.dom_box = this.rd.createElement('div');
    box.div_n = null;

    this.boxes.push (box);

    this.box__adjust (box, elem_rect);

    this.rd.setProperty (box.dom_box, 'id', 'G_' + id);
    this.rd.listen (box.dom_box, 'click', (event) => this.box__resize__activate (box));

    this.rd.listen (box.dom_box, 'mousedown', (event) => this.box__enable(event, dom_el, box));
    this.rd.listen (box.dom_box, 'touchstart', (event) => this.box__enable(event, dom_el, box));

    // add class
    this.rd.addClass(box.dom_box, 'box-el');

    // add all internal elements
    this.add_box__append_interieur (box.dom_box);

    //this.set_box_attributes (box.dom_box, names, colors);
    this.rd.appendChild (dom_el, box.dom_box);

    // create a new element from the given template
    /*
    let ref_view = this.el_box.createEmbeddedView (null);

    // retrieve the dom native element
    var dom_ref = ref_view.rootNodes[0];

    this.rd.appendChild (dom_box, dom_ref);
    */
  }

  //-----------------------------------------------------------------------------

  public set_box (id: number, names: object, colors: object): void
  {
    this.set_box_attributes (document.getElementById('G_' + id), names, colors);
  }

  //-----------------------------------------------------------------------------

  public on_edit (cb): void
  {
    this.on_edit_cb = cb;
  }

  //-----------------------------------------------------------------------------

  public on_delete (cb): void
  {
    this.on_delete_cb = cb;
  }

  //-----------------------------------------------------------------------------

  public on_mv (cb): void
  {
    this.on_mv_cb = cb;
  }

  //-----------------------------------------------------------------------------

  public set_background_image (data, cb)
  {
    this.clear ();

    const dom_el = this.el_dom.nativeElement;

    // create a blob
    var blob = new Blob([data]);

    // create an URL to display the blob
    const fileURL = URL.createObjectURL(blob);

    var image = new Image();

    image.onload = cb;
    image.src = fileURL;

    this.rd.addClass (image, 'paper-image');
    this.rd.setStyle (image, 'width', '100%');

    this.rd.listen (image, 'mousedown', (event) => {

      if (event.which === 1)
      {
        this.boxes.forEach ((box: GraphBox) => this.box__dissable (box));
      }

    });

    // add class
    this.rd.appendChild (dom_el, image);

/*
    let unlistener = this.rd.listen (image, 'onload', (event) => {

      unlistener ();

    });
    */
  }

  //-----------------------------------------------------------------------------

}

class GraphRect
{
  x: number;
  y: number;
  w: number;
  h: number;

  //---------------------------------------------------------------------------

  constructor (x: number, y: number, w: number, h: number, elem_rect)
  {
    this.x = elem_rect.width * x / 100;
    this.y = elem_rect.height * y / 100;
    this.w = elem_rect.width * w / 100;
    this.h = elem_rect.height * h / 100;
  }

  //---------------------------------------------------------------------------

  public move (x: number, y: number, elem_rect)
  {
    this.x = this.x + x;
    this.y = this.y + y;

    this.check_borders (elem_rect);
  }

  //---------------------------------------------------------------------------

  public resize (x: number, y: number, elem_rect, mx: number, my: number, sx: number, sy: number)
  {
    this.x = this.x + x * mx;
    this.y = this.y + y * my;

    this.check_borders (elem_rect);

    this.w = this.w + x * sx;
    this.h = this.h + y * sy;

    this.check_size ();
  }

  //---------------------------------------------------------------------------

  public check_borders (elem_rect)
  {
    if (this.x < 0)
    {
      this.x = 0;
    }

    if (this.y < 0)
    {
      this.y = 0;
    }

    if (this.x > elem_rect.width - this.w)
    {
      this.x = elem_rect.width - this.w;
    }

    if (this.y > elem_rect.height - this.h)
    {
      this.y = elem_rect.height - this.h;
    }
  }

  //---------------------------------------------------------------------------

  public check_size ()
  {
    if (this.w < 20)
    {
      this.w = 20;
    }

    if (this.h < 20)
    {
      this.h = 20;
    }
  }

  //---------------------------------------------------------------------------
}

export class GraphBox
{
  x: number;
  y: number;
  w: number;
  h: number;

  id: number;
  dom_box;

  // resize divs
  div_nw: any;
  div_ne: any;
  div_n: any;
  div_e: any;
  div_w: any;
  div_s: any;
  div_sw: any;
  div_se: any;

  // for resizing
  mv_x: number;
  mv_y: number;

  ev_mm;
  ev_tm;
  ev_mu;
  ev_te;
}
