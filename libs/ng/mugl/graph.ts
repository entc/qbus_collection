import {ElementRef, TemplateRef, EmbeddedViewRef, Renderer2, HostListener, EventEmitter} from '@angular/core';

//-----------------------------------------------------------------------------

const GRAPH_RESIZE_AREA = 30;
const GRAPH_RESIZE_AREA__HALF = GRAPH_RESIZE_AREA / 2;

const GRAPH_OPTION_AREA = 24;
const GRAPH_OPTION_AREA__HALF = GRAPH_OPTION_AREA / 2;

//-----------------------------------------------------------------------------

export class Graph {

  private on_delete_cb;
  private on_delete_sw;
  private on_delete_con;
  private on_mv_cb;
  private on_edit_cb;
  private on_dbclick_cb;
  private on_box_enabled_cb;
  private on_edit_sw;

  private btn_mv_active: GraphBox = null;
  private btn_mv_w: number;
  private btn_mv_h: number;
  private action_ref: number;

  private mv_x: number;
  private mv_y: number;

  private grid_x: number;
  private grid_y: number;

  private option_mv_x: boolean;
  private option_mv_y: boolean;

  private boxes: GraphBox[] = [];
  private switches: GraphSwitch[] =[];
  private connections: GraphConnection[] = [];
  private layer: any;

  //-----------------------------------------------------------------------------

  constructor(private el_dom: ElementRef, private el_box: TemplateRef<any>, private el_switch:TemplateRef<any>, private rd: Renderer2)
  {
    this.on_delete_cb = undefined;
    this.on_delete_sw = undefined;
    this.on_delete_con = undefined;
    this.on_mv_cb = undefined;
    this.on_edit_cb = undefined;
    this.on_edit_sw = undefined;

    this.action_ref = 0;

    this.grid_x = 1;
    this.grid_y = 1;

    this.option_mv_x = false;
    this.option_mv_y = false;

    this.layer = null;
  }

  //-----------------------------------------------------------------------------

  public layer__disable_boxes(event): void
  {
    if (event.which === 1)
    {
      this.boxes.forEach((box: GraphBox) => this.box__dissable(box));
    }
  }
  //-----------------------------------------------------------------------------

  public layer__disable_switches(event): void
  {
    if (event.which === 1)
    {
      this.switches.forEach((sw: GraphSwitch) => this.switch__dissable(sw));
    }
  }

  //-----------------------------------------------------------------------------

  public init(move_x: boolean = true, move_y: boolean = true): void
  {
    const dom_el = this.el_dom.nativeElement;

    this.option_mv_x = move_x;
    this.option_mv_y = move_y;

    this.layer = this.rd.createElement('div');

    // set the enabled class
    this.rd.addClass(this.layer, 'paper-el');

    // register events for this layer
    this.rd.listen(this.layer, 'mousedown', (event) =>
    {
      this.layer__disable_boxes(event);
      this.layer__disable_switches(event);
    });
    this.rd.listen(this.layer, 'touchstart', (event) =>
    {
      this.layer__disable_boxes(event)
      this.layer__disable_switches(event);
    });

    this.rd.listen(this.layer, 'dblclick', (event) =>
    {

      if (this.on_dbclick_cb)
      {
        // get the top and left coordinates of the box element
        var elem_rect = dom_el.getBoundingClientRect();

        var x: number = (event.clientX - elem_rect.left) / elem_rect.width * 100;
        var y: number = (event.clientY - elem_rect.top) / elem_rect.height * 100;

        this.on_dbclick_cb(x, y);
      }

    });

    // add class
    this.rd.appendChild(dom_el, this.layer);
  }

  //-----------------------------------------------------------------------------

  public set_dim(width: number, height: number)
  {
    const dom_el = this.el_dom.nativeElement;

    dom_el.style.width = String(width) + "px";
    dom_el.style.height = String(height) + "px";
  }

  //-----------------------------------------------------------------------------

  public set_grid(x: number, y: number)
  {
    this.grid_x = x;
    this.grid_y = y;
  }

  //-----------------------------------------------------------------------------

  public clear(): void
  {
    const dom_el = this.el_dom.nativeElement;

    while (dom_el.firstChild)
    {
      this.rd.removeChild(dom_el, dom_el.lastChild);
    }
  }

  //-----------------------------------------------------------------------------

  public refresh()
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    this.boxes.forEach((box: GraphBox) => this.box__adjust(box, elem_rect));
    this.switches.forEach((sw: GraphSwitch) => this.switch__adjust(sw, elem_rect));
  }

  //-----------------------------------------------------------------------------

  private box__adjust(box: GraphBox, elem_rect)
  {
    // change position
    this.rd.setStyle(box.dom_box, 'left', (elem_rect.width * box.x / 100) + 'px');
    this.rd.setStyle(box.dom_box, 'top', (elem_rect.height * box.y / 100) + 'px');

    // change dimension
    this.rd.setStyle(box.dom_box, 'width', (elem_rect.width * box.w / 100) + 'px');
    this.rd.setStyle(box.dom_box, 'height', (elem_rect.height * box.h / 100) + 'px');

    if (box.div_nw)
    {
      this.box__resize_buttons__adjust(box, new GraphRect(box.x, box.y, box.w, box.h, elem_rect));
      this.box__option_buttons__adjust(box, new GraphRect(box.x, box.y, box.w, box.h, elem_rect));
    }
  }

  //-----------------------------------------------------------------------------

  private switch__adjust(sw: GraphSwitch, elem_rect)
  {
    // change position
    this.rd.setStyle(sw.dom_box, 'left', (elem_rect.width * sw.x / 100) + 'px');
    this.rd.setStyle(sw.dom_box, 'top', (elem_rect.height * sw.y / 100) + 'px');

    // change dimension
    this.rd.setStyle(sw.dom_box, 'width', (elem_rect.width * sw.w / 100) + 'px');
    this.rd.setStyle(sw.dom_box, 'height', (elem_rect.height * sw.h / 100) + 'px');

    if (sw.div_nw)
    {
      this.switch__option_buttons__adjust(sw, new GraphRect(sw.x, sw.y, sw.w, sw.h, elem_rect));
    }
  }

  //-----------------------------------------------------------------------------

  private box_set_coordinates(box: GraphBox, x: number, y: number, elem_rect)
  {
    box.x = x / elem_rect.width * 100;
    box.y = y / elem_rect.height * 100;

    this.box_send_coordinates(box);
  }

  //-----------------------------------------------------------------------------

  private box_send_coordinates(box: GraphBox)
  {
    if (this.on_mv_cb)
    {
      this.on_mv_cb(box);
    }
  }
  //-----------------------------------------------------------------------------

  private switch_set_coordinates(sw: GraphSwitch, x: number, y: number, elem_rect)
  {
    sw.x = x / elem_rect.width * 100;
    sw.y = y / elem_rect.height * 100;

    this.switch_send_coordinates(sw);
  }

  //-----------------------------------------------------------------------------

  private switch_send_coordinates(sw: GraphSwitch)
  {
    if (this.on_mv_cb)
    {
      this.on_mv_cb(sw);
    }
  }

  //-----------------------------------------------------------------------------

  private box_set_size(box: GraphBox, x: number, y: number, w: number, h: number, elem_rect)
  {
    box.x = x / elem_rect.width * 100;
    box.y = y / elem_rect.height * 100;
    box.w = w / elem_rect.width * 100;
    box.h = h / elem_rect.height * 100;

    if (this.on_mv_cb)
    {
      this.on_mv_cb(box);
    }
  }

  //-----------------------------------------------------------------------------

  private box__enable(box: GraphBox)
  {
    // disable all other boxes
    this.boxes.forEach((box_loop: GraphBox) => this.box__dissable(box_loop));

    // set the enabled class
    this.rd.addClass(box.dom_box, 'box-el-enabled');

    if (this.on_box_enabled_cb)
    {
      this.on_box_enabled_cb(box);
    }
  }
  //-----------------------------------------------------------------------------

  private switch__enable(sw: GraphSwitch)
  {
    // disable all other switches
    this.switches.forEach((sw_loop: GraphSwitch) => this.switch__dissable(sw_loop));

    // set the enabled class
    this.rd.addClass(sw.dom_box, 'box-el-enabled');

    if (this.on_box_enabled_cb)
    {
      this.on_box_enabled_cb(sw);
    }
  }

  //-----------------------------------------------------------------------------

  private box__dissable__remove(el: any, div_el: any): any
  {
    if (div_el)
    {
      this.rd.removeChild(el, div_el);
    }

    return null;
  }

  //-----------------------------------------------------------------------------

  private box__dissable(box: GraphBox): void
  {
    const dom_el = this.el_dom.nativeElement;

    this.rd.removeClass(box.dom_box, 'box-el-enabled');

    box.div_ne = this.box__dissable__remove(dom_el, box.div_ne);
    box.div_nw = this.box__dissable__remove(dom_el, box.div_nw);
    box.div_n = this.box__dissable__remove(dom_el, box.div_n);
    box.div_w = this.box__dissable__remove(dom_el, box.div_w);
    box.div_e = this.box__dissable__remove(dom_el, box.div_e);
    box.div_s = this.box__dissable__remove(dom_el, box.div_s);
    box.div_se = this.box__dissable__remove(dom_el, box.div_se);
    box.div_sw = this.box__dissable__remove(dom_el, box.div_sw);

    if (this.on_box_enabled_cb)
    {
      this.on_box_enabled_cb(null);
    }
  }

  //-----------------------------------------------------------------------------

  private switch__dissable(sw: GraphSwitch): void
  {
    const dom_el = this.el_dom.nativeElement;

    this.rd.removeClass(sw.dom_box, 'box-el-enabled');

    sw.div_ne = this.box__dissable__remove(dom_el, sw.div_ne);
    sw.div_nw = this.box__dissable__remove(dom_el, sw.div_nw);

    if (this.on_box_enabled_cb)
    {
      this.on_box_enabled_cb(null);
    }
  }

  //-----------------------------------------------------------------------------

  private box__resize_buttons__adjust(box: GraphBox, rect: GraphRect): void
  {
    //this.rd.setStyle (box.div_nw, 'left', String(rect.x - GRAPH_RESIZE_AREA__HALF) + 'px');
    //this.rd.setStyle (box.div_nw, 'top' , String(rect.y - GRAPH_RESIZE_AREA__HALF) + 'px');

    //this.rd.setStyle (box.div_ne, 'left', String(rect.x + rect.w - GRAPH_RESIZE_AREA__HALF) + 'px');
    //this.rd.setStyle (box.div_ne, 'top' , String(rect.y - GRAPH_RESIZE_AREA__HALF) + 'px');

    this.rd.setStyle(box.div_n, 'left', String(rect.x + rect.w / 2 - GRAPH_RESIZE_AREA__HALF) + 'px');
    this.rd.setStyle(box.div_n, 'top', String(rect.y - GRAPH_RESIZE_AREA__HALF) + 'px');

    this.rd.setStyle(box.div_e, 'left', rect.x + rect.w - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle(box.div_e, 'top', rect.y + rect.h / 2 - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle(box.div_w, 'left', rect.x - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle(box.div_w, 'top', rect.y + rect.h / 2 - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle(box.div_s, 'left', rect.x + rect.w / 2 - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle(box.div_s, 'top', rect.y + rect.h - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle(box.div_sw, 'left', rect.x - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle(box.div_sw, 'top', rect.y + rect.h - GRAPH_RESIZE_AREA__HALF + 'px');

    this.rd.setStyle(box.div_se, 'left', rect.x + rect.w - GRAPH_RESIZE_AREA__HALF + 'px');
    this.rd.setStyle(box.div_se, 'top', rect.y + rect.h - GRAPH_RESIZE_AREA__HALF + 'px');
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_mousedown(event, box: GraphBox, div_el: any, mx: number, my: number, sx: number, sy: number)
  {
    if (event.which === 1)
    {
      const dom_el = this.el_dom.nativeElement;

      // prevent any other events outside the box
      event.preventDefault();

      box.ev_mm = this.rd.listen(dom_el, 'mousemove', (event) => this.box__resize__on_mousemove(event, box, mx, my, sx, sy));
      box.ev_mu = this.rd.listen(document, 'mouseup', (event) => this.box__resize__on_mouseup(event, box, mx, my, sx, sy));

      // this sets the mv_x and mv_y values
      box.init_move__mouse_event(event);
    }
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_mousemove(event, box: GraphBox, mx: number, my: number, sx: number, sy: number)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

    rect.resize(event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect, this.grid_x, this.grid_y, mx, my, sx, sy);

    this.rd.setStyle(box.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle(box.dom_box, 'top', '' + rect.y + 'px');
    this.rd.setStyle(box.dom_box, 'width', '' + rect.w + 'px');
    this.rd.setStyle(box.dom_box, 'height', '' + rect.h + 'px');

    this.box__resize_buttons__adjust(box, rect);
    this.box__option_buttons__adjust(box, rect);
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_mouseup(event, box: GraphBox, mx: number, my: number, sx: number, sy: number)
  {
    if (event.which === 1)
    {
      const dom_el = this.el_dom.nativeElement;

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect();

      box.ev_mm();
      box.ev_mu();

      var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

      rect.resize(event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect, this.grid_x, this.grid_y, mx, my, sx, sy);

      this.box_set_size(box, rect.x, rect.y, rect.w, rect.h, elem_rect);
    }
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_touchdown(event, box: GraphBox, div_el: any, mx: number, my: number, sx: number, sy: number)
  {
    const dom_el = this.el_dom.nativeElement;

    // prevent the scrolling
    box.ev_to = this.rd.listen(document.body, 'touchmove', (event) => event.preventDefault());

    // prevent any other events outside the box
    event.preventDefault();

    box.ev_tm = this.rd.listen(dom_el, 'touchmove', (event) => this.box__resize__on_touchmove(event, box, mx, my, sx, sy));
    box.ev_te = this.rd.listen(document, 'touchend', (event) => this.box__resize__on_touchup(event, box, mx, my, sx, sy));

    box.init_move__touch_event(event);
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_touchmove(event, box: GraphBox, mx: number, my: number, sx: number, sy: number)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    var touch = event.touches[0] || event.changedTouches[0];

    var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

    rect.resize(touch.pageX - box.mv_x, touch.pageY - box.mv_y, elem_rect, this.grid_x, this.grid_y, mx, my, sx, sy);

    this.rd.setStyle(box.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle(box.dom_box, 'top', '' + rect.y + 'px');
    this.rd.setStyle(box.dom_box, 'width', '' + rect.w + 'px');
    this.rd.setStyle(box.dom_box, 'height', '' + rect.h + 'px');

    this.box__resize_buttons__adjust(box, rect);
    this.box__option_buttons__adjust(box, rect);
  }

  //-----------------------------------------------------------------------------

  private box__resize__on_touchup(event, box: GraphBox, mx: number, my: number, sx: number, sy: number)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    box.ev_tm();
    box.ev_te();

    var touch = event.touches[0] || event.changedTouches[0];

    var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

    rect.resize(touch.pageX - box.mv_x, touch.pageY - box.mv_y, elem_rect, this.grid_x, this.grid_y, mx, my, sx, sy);

    this.box_set_size(box, rect.x, rect.y, rect.w, rect.h, elem_rect);
  }

  //-----------------------------------------------------------------------------

  private box__resize__append_button(box: GraphBox, dom_el, cursor_class: string, mx: number, my: number, sx: number, sy: number): any
  {
    let div_outer = this.rd.createElement('div');
    let div_inner = this.rd.createElement('div');

    this.rd.addClass(div_outer, 'box-el-resize');
    this.rd.setStyle(div_outer, 'cursor', cursor_class);

    this.rd.appendChild(dom_el, div_outer);

    this.rd.listen(div_outer, 'mousedown', (event) => this.box__resize__on_mousedown(event, box, dom_el, mx, my, sx, sy));
    this.rd.listen(div_outer, 'touchstart', (event) => this.box__resize__on_touchdown(event, box, dom_el, mx, my, sx, sy));

    this.rd.appendChild(div_outer, div_inner);
    this.rd.addClass(div_inner, 'box-el-resize-inner');

    return div_outer;
  }

  //-----------------------------------------------------------------------------

  private box__option_buttons__adjust(box: GraphBox, rect: GraphRect): void
  {
    this.rd.setStyle(box.div_nw, 'left', String(rect.x - GRAPH_OPTION_AREA__HALF) + 'px');
    this.rd.setStyle(box.div_nw, 'top', String(rect.y - GRAPH_OPTION_AREA - 2) + 'px');

    this.rd.setStyle(box.div_ne, 'left', String(rect.x + rect.w - GRAPH_OPTION_AREA__HALF) + 'px');
    this.rd.setStyle(box.div_ne, 'top', String(rect.y - GRAPH_OPTION_AREA - 2) + 'px');

    //this.rd.setStyle (box.div_se, 'left', String(rect.x + rect.w - GRAPH_OPTION_AREA__HALF) + 'px');
    //this.rd.setStyle (box.div_se, 'top' , String(rect.y + rect.h + 2) + 'px');
  }
  //-----------------------------------------------------------------------------

  private switch__option_buttons__adjust(sw: GraphSwitch, rect: GraphRect): void
  {
    this.rd.setStyle(sw.div_nw, 'left', String(rect.x - GRAPH_OPTION_AREA__HALF) + 'px');
    this.rd.setStyle(sw.div_nw, 'top', String(rect.y - GRAPH_OPTION_AREA - 2) + 'px');

    this.rd.setStyle(sw.div_ne, 'left', String(rect.x + rect.w - GRAPH_OPTION_AREA__HALF) + 'px');
    this.rd.setStyle(sw.div_ne, 'top', String(rect.y - GRAPH_OPTION_AREA - 2) + 'px');
  }

  //-----------------------------------------------------------------------------

  private box__option__append_button(box: GraphBox, dom_el, cursor_class: string, icon: string, cb): any
  {
    let div_outer = this.rd.createElement('div');

    this.rd.addClass(div_outer, 'box-el-option');
    this.rd.setStyle(div_outer, 'cursor', cursor_class);

    this.rd.setProperty(div_outer, 'innerHTML', '<i class="fa fa-' + icon + '"></i>');

    this.rd.appendChild(dom_el, div_outer);

    this.rd.listen(div_outer, 'click', (event) => cb ? cb(box) : null);

    return div_outer;
  }

  //-----------------------------------------------------------------------------

  private switch__option__append_button(sw: GraphSwitch, dom_el, cursor_class: string, icon: string, cb): any
  {
    let div_outer = this.rd.createElement('div');

    this.rd.addClass(div_outer, 'box-el-option');
    this.rd.setStyle(div_outer, 'cursor', cursor_class);

    this.rd.setProperty(div_outer, 'innerHTML', '<i class="fa fa-' + icon + '"></i>');

    this.rd.appendChild(dom_el, div_outer);

    this.rd.listen(div_outer, 'click', (event) => cb ? cb(sw) : null);

    return div_outer;
  }

  //-----------------------------------------------------------------------------

  private box__enable__append_buttons(box: GraphBox): void
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    //box.div_nw = this.box__resize__append_button (box, dom_el, 'nw-resize', 1, 1, -1, -1);
    //box.div_ne = this.box__resize__append_button (box, dom_el, 'ne-resize', 0, 1, 1, -1);
    box.div_n = this.box__resize__append_button(box, dom_el, 'n-resize', 0, 1, 0, -1);
    box.div_e = this.box__resize__append_button(box, dom_el, 'e-resize', 0, 0, 1, 0);
    box.div_w = this.box__resize__append_button(box, dom_el, 'w-resize', 1, 0, -1, 0);
    box.div_s = this.box__resize__append_button(box, dom_el, 's-resize', 0, 0, 0, 1);
    box.div_sw = this.box__resize__append_button(box, dom_el, 'sw-resize', 1, 0, -1, 1);
    box.div_se = this.box__resize__append_button(box, dom_el, 'se-resize', 0, 0, 1, 1);

    //box.div_ne = this.box__option__append_button (box, dom_el, 'hand', 'times', this.on_delete_cb);
    //box.div_se = this.box__option__append_button (box, dom_el, 'hand', 'pen', this.on_edit_cb);

    box.div_nw = this.box__option__append_button(box, dom_el, 'hand', 'times', this.on_delete_cb);
    box.div_ne = this.box__option__append_button(box, dom_el, 'hand', 'pen', this.on_edit_cb);

    var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

    this.box__resize_buttons__adjust(box, rect);
    this.box__option_buttons__adjust(box, rect);
  }

  //-----------------------------------------------------------------------------

  private switch__enable__append_buttons(sw: GraphSwitch): void
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    sw.div_nw = this.switch__option__append_button(sw, dom_el, 'hand', 'times', this.on_delete_sw);
    sw.div_ne = this.switch__option__append_button(sw, dom_el, 'hand', 'pen', this.on_edit_sw);

    var rect: GraphRect = new GraphRect(sw.x, sw.y, sw.w, sw.h, elem_rect);

    this.switch__option_buttons__adjust(sw, rect);
  }

  //-----------------------------------------------------------------------------

  private box__move__on_mousedown(event, dom_el, box: GraphBox)
  {
    if (event.which === 1)  // only react on left mouse button
    {
      // enable the box
      this.box__enable(box);

      // prevent any other events outside the box
      event.preventDefault();

      // add temporary listeners
      box.ev_mm = this.rd.listen(dom_el, 'mousemove', (event) => this.box__move__on_mousemove(event, box));
      box.ev_mu = this.rd.listen(document, 'mouseup', (event) => this.box__move__on_mouseup(event, box));

      box.init_move__mouse_event(event);
    }
  }

  //-----------------------------------------------------------------------------

  private switch__move__on_mousedown(event, dom_el, sw: GraphSwitch)
  {
    if (event.which === 1)  // only react on left mouse button
    {
      // enable the box
      this.switch__enable(sw);

      // prevent any other events outside the box
      event.preventDefault();

      // add temporary listeners
      sw.ev_mm = this.rd.listen(dom_el, 'mousemove', (event) => this.switch__move__on_mousemove(event, sw));
      sw.ev_mu = this.rd.listen(document, 'mouseup', (event) => this.switch__move__on_mouseup(event, sw));

      sw.init_move__mouse_event(event);
    }
  }

  //-----------------------------------------------------------------------------

  private box__move__on_mousemove(event, box: GraphBox)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

//console.log("mouse y = " + event.clientY + " top = " + elem_rect.top + " height = " + elem_rect.height + " window h = " + window.innerHeight);

    // check if it is outside of the boundaries
    if ((elem_rect.top + elem_rect.height) < event.clientY)
    {
      alert('gheee');
    }

    var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

    // do a simple move transformation
    rect.move(event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect, this.grid_x, this.grid_y);

    // align on snap points
    rect.snap(box, this.boxes, elem_rect);

    this.rd.setStyle(box.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle(box.dom_box, 'top', '' + rect.y + 'px');

    box.view.detectChanges();
    this.update_connection_position(box)
  }
  //-----------------------------------------------------------------------------

  private switch__move__on_mousemove(event, sw: GraphSwitch)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    // check if it is outside of the boundaries
    if ((elem_rect.top + elem_rect.height) < event.clientY)
    {
      alert('out of bounds');
    }

    var rect: GraphRect = new GraphRect(sw.x, sw.y, sw.w, sw.h, elem_rect);

    // do a simple move transformation
    rect.move(event.clientX - sw.mv_x, event.clientY - sw.mv_y, elem_rect, this.grid_x, this.grid_y);

    // align on snap points
    rect.snap(sw, this.boxes, elem_rect);

    this.rd.setStyle(sw.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle(sw.dom_box, 'top', '' + rect.y + 'px');

    sw.view.detectChanges();
    this.update_connection_position(sw)
  }

  //-----------------------------------------------------------------------------

  private box__move__on_mouseup(event, box: GraphBox)
  {
    if (event.which === 1)  // only react on left mouse button
    {
      box.ev_mm();
      box.ev_mu();

      const dom_el = this.el_dom.nativeElement;

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect();

      var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

      // do a simple move transformation
      rect.move(event.clientX - box.mv_x, event.clientY - box.mv_y, elem_rect, this.grid_x, this.grid_y);

      // align on snap points
      rect.snap(box, this.boxes, elem_rect);

      this.rd.setStyle(box.dom_box, 'left', '' + rect.x + 'px');
      this.rd.setStyle(box.dom_box, 'top', '' + rect.y + 'px');

      this.box_set_coordinates(box, rect.x, rect.y, elem_rect);

      this.box__enable__append_buttons(box);
      this.update_connection_position(box)
    }
  }

  private switch__move__on_mouseup(event, sw: GraphSwitch)
  {
    if (event.which === 1)  // only react on left mouse button
    {
      sw.ev_mm();
      sw.ev_mu();

      const dom_el = this.el_dom.nativeElement;

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect();

      var rect: GraphRect = new GraphRect(sw.x, sw.y, sw.w, sw.h, elem_rect);

      // do a simple move transformation
      rect.move(event.clientX - sw.mv_x, event.clientY - sw.mv_y, elem_rect, this.grid_x, this.grid_y);

      // align on snap points
      rect.snap(sw, this.boxes, elem_rect);

      this.rd.setStyle(sw.dom_box, 'left', '' + rect.x + 'px');
      this.rd.setStyle(sw.dom_box, 'top', '' + rect.y + 'px');

      this.switch_set_coordinates(sw, rect.x, rect.y, elem_rect);

      this.switch__enable__append_buttons(sw);
      this.update_connection_position(sw)
    }
  }

  //-----------------------------------------------------------------------------

  private box__move__on_touchstart(event, dom_el, box: GraphBox)
  {
    // enable the box
    this.box__enable(box);

    // prevent the scrolling
    box.ev_to = this.rd.listen(document.body, 'touchmove', (event) => event.preventDefault());

    // prevent any other events outside the box
    event.preventDefault();

    box.ev_tm = this.rd.listen(dom_el, 'touchmove', (event) => this.box__move__on_touchmove(event, box));
    box.ev_te = this.rd.listen(document, 'touchend', (event) => this.box__move__on_touchup(event, box));

    box.init_move__touch_event(event);
  }

  //-----------------------------------------------------------------------------

  private switch__move__on_touchstart(event, dom_el, sw: GraphSwitch)
  {
    // enable the box
    this.switch__enable(sw);

    // prevent the scrolling
    sw.ev_to = this.rd.listen(document.body, 'touchmove', (event) => event.preventDefault());

    // prevent any other events outside the box
    event.preventDefault();

    sw.ev_tm = this.rd.listen(dom_el, 'touchmove', (event) => this.switch__move__on_touchmove(event, sw));
    sw.ev_te = this.rd.listen(document, 'touchend', (event) => this.switch__move__on_touchup(event, sw));

    sw.init_move__touch_event(event);
  }

  //-----------------------------------------------------------------------------

  private box__move__on_touchmove(event, box: GraphBox)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

    var touch = event.touches[0] || event.changedTouches[0];

    // do a simple move transformation
    rect.move(touch.pageX - box.mv_x, touch.pageY - box.mv_y, elem_rect, this.grid_x, this.grid_y);

    this.rd.setStyle(box.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle(box.dom_box, 'top', '' + rect.y + 'px');

    box.view.detectChanges();
    this.update_connection_position(box)
  }

  //-----------------------------------------------------------------------------

  private switch__move__on_touchmove(event, sw: GraphSwitch)
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    var rect: GraphRect = new GraphRect(sw.x, sw.y, sw.w, sw.h, elem_rect);

    var touch = event.touches[0] || event.changedTouches[0];

    // do a simple move transformation
    rect.move(touch.pageX - sw.mv_x, touch.pageY - sw.mv_y, elem_rect, this.grid_x, this.grid_y);

    this.rd.setStyle(sw.dom_box, 'left', '' + rect.x + 'px');
    this.rd.setStyle(sw.dom_box, 'top', '' + rect.y + 'px');

    sw.view.detectChanges();
    this.update_connection_position(sw)
  }

  //-----------------------------------------------------------------------------

  private box__move__on_touchup(event, box: GraphBox)
  {
    //if (event.which === 1)
    {
      const dom_el = this.el_dom.nativeElement;

//      console.log('touch up');

      box.ev_to();

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect();

      box.ev_tm();
      box.ev_te();

      var touch = event.touches[0] || event.changedTouches[0];

      var rect: GraphRect = new GraphRect(box.x, box.y, box.w, box.h, elem_rect);

      rect.move(touch.pageX - box.mv_x, touch.pageY - box.mv_y, elem_rect, this.grid_x, this.grid_y);

      this.rd.setStyle(box.dom_box, 'left', '' + rect.x + 'px');
      this.rd.setStyle(box.dom_box, 'top', '' + rect.y + 'px');

      this.box_set_coordinates(box, rect.x, rect.y, elem_rect);

      this.box__enable__append_buttons(box);
      this.update_connection_position(box)
    }
  }
  //-----------------------------------------------------------------------------

  private switch__move__on_touchup(event, sw: GraphSwitch)
  {
      const dom_el = this.el_dom.nativeElement;
      sw.ev_to();

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect();

      sw.ev_tm();
      sw.ev_te();

      var touch = event.touches[0] || event.changedTouches[0];

      var rect: GraphRect = new GraphRect(sw.x, sw.y, sw.w, sw.h, elem_rect);

      rect.move(touch.pageX - sw.mv_x, touch.pageY - sw.mv_y, elem_rect, this.grid_x, this.grid_y);

      this.rd.setStyle(sw.dom_box, 'left', '' + rect.x + 'px');
      this.rd.setStyle(sw.dom_box, 'top', '' + rect.y + 'px');

      this.switch_set_coordinates(sw, rect.x, rect.y, elem_rect);

      this.switch__enable__append_buttons(sw);
      this.update_connection_position(sw)
  }

  //-----------------------------------------------------------------------------

  public add_box__reset_sync()
  {
    // set the sync var for all boxes to false
    // -> boxes which remain false might be removed
    this.boxes.forEach((box: GraphBox) => box.synced = false);
  }

  //-----------------------------------------------------------------------------

  public add_box__remove_sync()
  {
    const dom_el = this.el_dom.nativeElement;

    var i = this.boxes.length;
    while (i--)
    {
      var box: GraphBox = this.boxes[i];

      if (box.synced == false)
      {
        this.rm_box(box, i);

        /*
                this.box__dissable(box);
                this.rd.removeChild(dom_el, box.dom_box);

                this.boxes.splice(i, 1);
                */
      }
    }
  }

  //-----------------------------------------------------------------------------

  public rm_box(box: GraphBox, index: number = this.boxes.findIndex((b: GraphBox) => b == box))
  {
    if (index >= 0)
    {
      const dom_el = this.el_dom.nativeElement;

      this.box__dissable(box);
      this.rd.removeChild(dom_el, box.dom_box);

      this.boxes.splice(index, 1);
    }
  }

  //-----------------------------------------------------------------------------

  public get_box(sync): GraphBox
  {
    // this is not very efficient
    // -> TODO: replace this with some kind of other algorithm
    var index: number = this.boxes.findIndex((box: GraphBox) => sync(box));

    if (index == -1)
    {
      return null;
    } else
    {
      return this.boxes[index];
    }
  }
  //-----------------------------------------------------------------------------

  public get_switch(sync): GraphSwitch
  {
    // this is not very efficient
    // -> TODO: replace this with some kind of other algorithm
    var index: number = this.switches.findIndex((sw: GraphSwitch) => sync(sw));

    if (index == -1)
    {
      return null;
    } else
    {
      return this.switches[index];
    }
  }

  //-----------------------------------------------------------------------------

  public add_box(id: number, x: number, y: number, w: number, h: number, user_data: any, sync, color): GraphBox
  {
    if (sync)
    {
      var box: GraphBox = this.get_box(sync);

      if (box)
      {
        box.synced = true;

        // update the box values
        this.rd.setStyle(box.dom_box, 'background-color', color);

        return box;
      }
    }

    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    x = ((x == null) ? 0 : x);
    y = ((y == null) ? 0 : y);

    var box: GraphBox = new GraphBox(id, x, y, w, h, user_data);

    box.dom_box = this.rd.createElement('div');
    box.div_n = null;

    this.boxes.push(box);

    this.box__adjust(box, elem_rect);

    this.rd.setProperty(box.dom_box, 'id', 'G_' + id);
    this.rd.setStyle(box.dom_box, 'background-color', color);

    this.rd.listen(box.dom_box, 'mousedown', (event) => this.box__move__on_mousedown(event, dom_el, box));
    this.rd.listen(box.dom_box, 'touchstart', (event) => this.box__move__on_touchstart(event, dom_el, box));

    // add class
    this.rd.addClass(box.dom_box, 'box-el');
    this.rd.addClass(box.dom_box, 'box-opacity');

    //this.set_box_attributes (box.dom_box, names, colors);
    this.rd.appendChild(dom_el, box.dom_box);

    // create a new element from the given template
    // -> pass the box as context
    box.view = this.el_box.createEmbeddedView(box.ctx);

    // retrieve the dom native element
    var dom_ref = box.view.rootNodes[0];

    this.rd.appendChild(box.dom_box, dom_ref);

    return box;
  }

  //-----------------------------------------------------------------------------

  public add_switch(id: number, x: number, y: number, user_data: any, sync, color): GraphSwitch
  {
    if (sync)
    {
      var sw: GraphSwitch = this.get_switch(sync);

      if (sw)
      {
        sw.synced = true;

        // update the box values
        this.rd.setStyle(sw.dom_box, 'background-color', color);

        return sw;
      }
    }

    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect();

    x = ((x == null) ? 0 : x);
    y = ((y == null) ? 0 : y);

    const w = 10
    const h = 10

    var sw: GraphSwitch = new GraphSwitch(id, x, y, w, h, user_data);

    sw.dom_box = this.rd.createElement('div');
    sw.div_n = null;

    this.switches.push(sw);

    this.switch__adjust(sw, elem_rect);

    this.rd.setProperty(sw.dom_box, 'id', 'S_' + id);
    this.rd.setStyle(sw.dom_box, 'background-color', color);

    this.rd.listen(sw.dom_box, 'mousedown', (event) => this.switch__move__on_mousedown(event, dom_el, sw));
    this.rd.listen(sw.dom_box, 'touchstart', (event) => this.switch__move__on_touchstart(event, dom_el, sw));

    // add class
    this.rd.addClass(sw.dom_box, 'box-el');
    this.rd.addClass(sw.dom_box, 'box-opacity');

    this.rd.appendChild(dom_el, sw.dom_box);

    sw.view = this.el_box.createEmbeddedView(sw.ctx);

    // retrieve the dom native element
    var dom_ref = sw.view.rootNodes[0];

    this.rd.appendChild(sw.dom_box, dom_ref);

    return sw;
  }

  //-----------------------------------------------------------------------------

  public add_conn(id: number, elem1: GraphBox | GraphSwitch, elem2: GraphBox | GraphSwitch, isActive:boolean|undefined, user_data: any, sync)
  {
    console.log("called 1", id);
    console.log("called 2",  elem1);
    console.log("called 3",  elem2);
    const dom_el = this.el_dom.nativeElement;

    const elem_rect1= elem1.dom_box.getBoundingClientRect();
    let rect1_div
    if(elem1 instanceof GraphBox){
      rect1_div = document.getElementById("G_" + elem1.id);
    } else{
      rect1_div = document.getElementById("S_" + elem1.id);
    }
    const rect1_styles= window.getComputedStyle(rect1_div);

    const elem_rect2 = elem2.dom_box.getBoundingClientRect();
    let rect2_div
    if(elem2 instanceof GraphBox){
      rect2_div = document.getElementById("G_" + elem2.id);
    }else{
      rect2_div = document.getElementById("S_" + elem2.id);
    }
    const rect2_styles = window.getComputedStyle(rect2_div);

    const conn: GraphConnection = new GraphConnection(id, elem1, elem2, user_data)

    conn.dom_conn = this.rd.createElement('svg', 'svg');
    this.rd.setProperty(conn.dom_conn, 'id', 'C_' + id);

    this.connections.push(conn);

    let defs = this.rd.createElement('defs', 'svg');
    let line = this.rd.createElement('line', 'svg');
    let {
      conn_x1,
      conn_y1,
      conn_x2,
      conn_y2,
      svg_left,
      svg_top,
      svg_width,
      svg_height
    } = this.getConnectionPositionValues(rect1_styles, rect2_styles, elem_rect1, elem_rect2);

    let color
    if(isActive) {
      color="#198754";}
    else{
      color="#495057";
    }

    this.rd.setAttribute(line, 'x1', conn_x1.toString());
    this.rd.setAttribute(line, 'y1', conn_y1.toString());
    this.rd.setAttribute(line, 'x2', conn_x2.toString());
    this.rd.setAttribute(line, 'y2', conn_y2.toString());
    this.rd.setStyle(line, "stroke", color)
    this.rd.setStyle(line, "stroke-width", "4")
    this.rd.setStyle(line, "marker-end", "url(#arrowhead)")

    this.rd.appendChild(conn.dom_conn, defs)
    this.rd.appendChild(conn.dom_conn, line);
    this.rd.appendChild(dom_el, conn.dom_conn);

    // add class
    this.rd.addClass(conn.dom_conn, 'conn-el');

    this.rd.setAttribute(conn.dom_conn, 'width', svg_width + 'px');
    this.rd.setAttribute(conn.dom_conn, 'height', svg_height + 'px');
    this.rd.setStyle(conn.dom_conn, 'left', svg_left + 'px');
    this.rd.setStyle(conn.dom_conn, 'top', svg_top + 'px');
    this.rd.setStyle(conn.dom_conn, 'position', 'absolute');
  }

  private getConnectionPositionValues(rect1_styles: CSSStyleDeclaration, rect2_styles: CSSStyleDeclaration, elem_rect1, elem_rect2)
  {
    let conn_x1: number
    let conn_y1: number
    let conn_x2: number
    let conn_y2: number

    let svg_left: number
    let svg_right: number
    let svg_top: number
    let svg_bottom: number
    let svg_width: number
    let svg_height: number

    const rect_left1 = Number(rect1_styles.left.match(/^\d+/)[0]) ?? 0
    const rect_top1 = Number(rect1_styles.top.match(/^\d+/)[0]) ?? 0
    const rect_right1 = Number(rect1_styles.right.match(/^\d+/)[0]) ?? 0
    const rect_bottom1 = Number(rect1_styles.bottom.match(/^\d+/)[0]) ?? 0

    const rect_left2 = Number(rect2_styles.left.match(/^\d+/)[0]) ?? 0
    const rect_top2 = Number(rect2_styles.top.match(/^\d+/)[0]) ?? 0
    const rect_right2 = Number(rect2_styles.right.match(/^\d+/)[0]) ?? 0
    const rect_bottom2 = Number(rect2_styles.bottom.match(/^\d+/)[0]) ?? 0
    if ((rect_right1 - rect_left1) > (rect_right2 - rect_left2))
    {
      svg_left = Math.abs(rect_left1 + elem_rect1.width / 2)
      svg_right = Math.abs(rect_left2 + elem_rect2.width / 2)
      svg_width = Math.abs(svg_right - svg_left)
      conn_x1 = Math.abs(svg_width)
      conn_x2 = 3
    } else
    {
      svg_left = Math.abs(rect_left2 + elem_rect2.width / 2)
      svg_right = Math.abs(rect_left1 + elem_rect1.width / 2)
      svg_width = Math.abs(svg_right - svg_left)
      conn_x1 = 3
      conn_x2 = Math.abs(svg_width)
    }


    if ((rect_bottom1 - rect_top1) > (rect_bottom2 - rect_top2))
    {
      svg_top = Math.abs(rect_top1 + elem_rect1.height / 2)
      svg_bottom = Math.abs(rect_top2 + elem_rect2.height / 2)
      svg_height = Math.abs(svg_bottom - svg_top)
      conn_y1 = Math.abs(svg_height)
      conn_y2 = 3
    } else
    {
      svg_top = Math.abs(rect_top2 + elem_rect2.height / 2)
      svg_bottom = Math.abs(rect_top1 + elem_rect1.height / 2)
      svg_height = Math.abs(svg_bottom - svg_top)
      conn_y1 = 3
      conn_y2 = Math.abs(svg_height)
    }
    if (svg_height<20)
    {
      svg_height = 20
    }
    if (svg_width<20)
    {
      svg_width = 20
    }

    return {conn_x1, conn_y1, conn_x2, conn_y2, svg_left, svg_top, svg_width, svg_height};
  }

//-----------------------------------------------------------------------------

  public update_connection_position(box: GraphBox)
  {
    const connections = this.get_connections(box)
    console.log("graph connections", connections)

    for (let conn of connections)
    {
      console.log("id to search",conn.dom_conn.id)
      const conn_svg = document.getElementById(conn.dom_conn.id) as unknown as SVGSVGElement
      const conn_line = document.getElementById(conn.dom_conn.id).getElementsByTagName('line')[0] as unknown as SVGLineElement
      const elem_rect1 = conn.elem1.dom_box.getBoundingClientRect();
      let rect1_div
      if(conn.elem1 instanceof GraphBox){
        rect1_div = document.getElementById("G_" + conn.elem1.id);
      }else{
        rect1_div = document.getElementById("S_" + conn.elem1.id);
      }

      const rect1_styles = window.getComputedStyle(rect1_div);
      const elem_rect2 = conn.elem2.dom_box.getBoundingClientRect();
      let rect2_div
      if(conn.elem2 instanceof GraphBox){

      rect2_div = document.getElementById("G_" + conn.elem2.id);
      }else{
      rect2_div = document.getElementById("S_" + conn.elem2.id);
      }
      const rect2_styles = window.getComputedStyle(rect2_div);

      let {
        conn_x1,
        conn_y1,
        conn_x2,
        conn_y2,
        svg_left,
        svg_top,
        svg_width,
        svg_height
      } = this.getConnectionPositionValues(rect1_styles, rect2_styles, elem_rect1, elem_rect2);

      this.rd.setAttribute(conn_svg, 'width', svg_width + 'px');
      this.rd.setAttribute(conn_svg, 'height', svg_height + 'px');
      this.rd.setStyle(conn_svg, 'left', svg_left + 'px');
      this.rd.setStyle(conn_svg, 'top', svg_top + 'px');

      this.rd.setAttribute(conn_line, 'x1', conn_x1.toString());
      this.rd.setAttribute(conn_line, 'y1', conn_y1.toString());
      this.rd.setAttribute(conn_line, 'x2', conn_x2.toString());
      this.rd.setAttribute(conn_line, 'y2', conn_y2.toString());
    }

  }

  //-----------------------------------------------------------------------------

  public get_connections(elem: GraphBox | GraphSwitch): GraphConnection[]
  {
    const connections = this.connections.filter((conn: GraphConnection) => conn.elem1 === elem || conn.elem2 === elem)
    if (connections === undefined)
    {
      return null;
    } else
    {
      return connections;
    }
  }

  //-----------------------------------------------------------------------------

  public resetConnetions()
  {
    this.connections = [];
  }
  //-----------------------------------------------------------------------------

  public on_edit(cb): void
  {
    this.on_edit_cb = cb;
  }

  //-----------------------------------------------------------------------------

  public on_edit_switch(sw):void
  {
    this.on_edit_sw = sw
  }
  //-----------------------------------------------------------------------------

  public on_delete(cb): void
  {
    this.on_delete_cb = cb;
  }
  //-----------------------------------------------------------------------------

  public on_delete_switch(sw): void
  {
    this.on_delete_sw = sw;
  }
  //-----------------------------------------------------------------------------

  public on_delete_connection(con): void
  {
    this.on_delete_con = con;
  }

  //-----------------------------------------------------------------------------

  public on_mv(cb): void
  {
    this.on_mv_cb = cb;
  }

  //-----------------------------------------------------------------------------

  public on_dbclick(cb): void
  {
    this.on_dbclick_cb = cb;
  }

  //-----------------------------------------------------------------------------

  public on_box_enabled(cb): void
  {
    this.on_box_enabled_cb = cb;
  }

  //-----------------------------------------------------------------------------

  public set_background_image(data, cb)
  {
    const dom_el = this.el_dom.nativeElement;

    // create a blob
    var blob = new Blob([data]);

    // create an URL to display the blob
    const fileURL = URL.createObjectURL(blob);

    var image = new Image();

    image.onload = cb;
    image.src = fileURL;

    this.rd.addClass(image, 'paper-image');
    this.rd.setStyle(image, 'width', '100%');

    // add class
    this.rd.appendChild(this.layer, image);

    /*
        let unlistener = this.rd.listen (image, 'onload', (event) => {

          unlistener ();

        });
        */
  }

  //-----------------------------------------------------------------------------

}

class GraphRect {
  x: number;
  y: number;
  w: number;
  h: number;

  //---------------------------------------------------------------------------

  constructor(x: number, y: number, w: number, h: number, elem_rect)
  {
    this.x = elem_rect.width * x / 100;
    this.y = elem_rect.height * y / 100;
    this.w = elem_rect.width * w / 100;
    this.h = elem_rect.height * h / 100;
  }

  //---------------------------------------------------------------------------

  public move(x: number, y: number, elem_rect, grid_x: number, grid_y: number)
  {
    this.x = Math.floor((this.x + x) / grid_x) * grid_x;
    this.y = Math.floor((this.y + y) / grid_y) * grid_y;

    this.check_borders(elem_rect);
  }

  //---------------------------------------------------------------------------

  public snap(box: GraphBox, boxes: GraphBox[], elem_rect)
  {
    let snapy = 0;
    let snapx = 0;

    // recalculate the percentage values
    let x = this.x / elem_rect.width * 100;
    let y = this.y / elem_rect.height * 100;

    // check other boxes if we have any kind of alignment
    boxes.forEach((e: GraphBox) =>
    {

      if (e != box)
      {

        let midy = e.y + e.h / 2 - box.h / 2;
        let midx = e.x + e.w / 2 - box.w / 2;

        if (midy < y + 1 && midy > y - 1)
        {
          snapy = midy;
        }

        if (midx < x + 1 && midx > x - 1)
        {
          snapx = midx;
        }

        if (snapy == 0 && snapx == 0)
        {
          if (e.y < y + 1 && e.y > y - 1)
          {
            snapy = e.y;
          }

          if (e.x < x + 1 && e.x > x - 1)
          {
            snapx = e.x;
          }

          let maxy = e.y + e.h - box.h;
          let maxx = e.x + e.w - box.w;

          if (maxy < y + 1 && maxy > y - 1)
          {
            snapy = maxy;
          }

          if (maxx < x + 1 && maxx > x - 1)
          {
            snapx = maxx;
          }
        }
      }

    });

    if (snapy > 0)
    {
      this.y = elem_rect.height * snapy / 100;
    }

    if (snapx > 0)
    {
      this.x = elem_rect.width * snapx / 100;
    }
  }

  //---------------------------------------------------------------------------

  public resize(x: number, y: number, elem_rect, grid_x: number, grid_y: number, mx: number, my: number, sx: number, sy: number)
  {
    this.x = Math.floor((this.x + x * mx) / grid_x) * grid_x;
    this.y = Math.floor((this.y + y * my) / grid_y) * grid_y;

    this.check_borders(elem_rect);

    this.w = Math.floor((this.w + x * sx) / grid_x) * grid_x;
    this.h = Math.floor((this.h + y * sy) / grid_y) * grid_y;

    this.check_size();
  }

  //---------------------------------------------------------------------------

  public check_borders(elem_rect)
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

  public check_size()
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

export class GraphBox {
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

  ev_to;

  // internal use
  view: EmbeddedViewRef<GraphContext> = null;
  ctx: GraphContext = new GraphContext(null);

  synced: boolean;

  //---------------------------------------------------------------------------

  constructor(public id: number, public x: number, public y: number, public w: number, public h: number, public user_data: any)
  {
  }

  //---------------------------------------------------------------------------

  public set(data)
  {
    this.ctx.$implicit = data;

    if (this.view)
    {
      // call this (now sure why)
      // -> without the call the context will not be displayed
      this.view.detectChanges();
    }
  }

  //---------------------------------------------------------------------------

  public init_move__mouse_event(event)
  {
    this.mv_x = event.clientX;
    this.mv_y = event.clientY;
  }

  //---------------------------------------------------------------------------

  public init_move__touch_event(event)
  {
    var touch = event.touches[0] || event.changedTouches[0];

    // calculate the delta to the current mouse position
    this.mv_x = touch.pageX;
    this.mv_y = touch.pageY;
  }

  //---------------------------------------------------------------------------
}

//-----------------------------------------------------------------------------

class GraphContext {
  constructor(public $implicit: any)
  {
  }
}

//-----------------------------------------------------------------------------

export class GraphConnection {
  dom_conn

  constructor(public id: number, public elem1: GraphBox | GraphSwitch, public elem2: GraphBox| GraphSwitch, public user_data: any)
  {
  }
}

//-----------------------------------------------------------------------------

export class GraphSwitch {
  dom_box;

  div_nw: any;
  div_ne: any;
  div_n: any;
  div_e: any;
  div_w: any;
  div_s: any;
  div_sw: any;
  div_se: any;


  mv_x: number;
  mv_y: number;

  ev_mm;
  ev_tm;
  ev_mu;
  ev_te;

  ev_to;

  // internal use
  view: EmbeddedViewRef<GraphContext> = null;
  ctx: GraphContext = new GraphContext(null);

  synced: boolean;

  //---------------------------------------------------------------------------

  constructor(public id: number, public x: number, public y: number, public w: number, public h: number, public user_data: any)
  {
  }

  //---------------------------------------------------------------------------

  public set(data)
  {
    this.ctx.$implicit = data;

    if (this.view)
    {
      // call this (now sure why)
      // -> without the call the context will not be displayed
      this.view.detectChanges();
    }
  }

  //---------------------------------------------------------------------------

  public init_move__mouse_event(event)
  {
    this.mv_x = event.clientX;
    this.mv_y = event.clientY;
  }

  //---------------------------------------------------------------------------

  public init_move__touch_event(event)
  {
    var touch = event.touches[0] || event.changedTouches[0];

    // calculate the delta to the current mouse position
    this.mv_x = touch.pageX;
    this.mv_y = touch.pageY;
  }


  //---------------------------------------------------------------------------
}