import { ElementRef, TemplateRef, EmbeddedViewRef, Renderer2, HostListener } from '@angular/core';

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

    this.init__events (dom_el);
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

    this.boxes.forEach ((box: GraphBox) => this.adjust_box (box, elem_rect));
  }

  //-----------------------------------------------------------------------------

  private adjust_box (box: GraphBox, elem_rect)
  {
    // change position
    this.rd.setStyle (box.dom_box, 'left', (elem_rect.width * box.x / 100) + 'px');
    this.rd.setStyle (box.dom_box, 'top', (elem_rect.height * box.y / 100) + 'px');

    // change dimension
    this.rd.setStyle (box.dom_box, 'width', (elem_rect.width * box.w / 100) + 'px');
    this.rd.setStyle (box.dom_box, 'height', (elem_rect.height * box.h / 100) + 'px');
  }

  //-----------------------------------------------------------------------------

  private box_set_coordinates (box: GraphBox, x: number, y: number, elem_rect)
  {
    box.x = x / elem_rect.width * 100;
    box.y = y / elem_rect.height * 100;
  }

  //-----------------------------------------------------------------------------

  private calc_coordinates (ex: number, ey: number): {x: number, y: number}
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    var gx = Math.round((ex - this.mv_x) / this.grid);
    var gy = Math.round((ey - this.mv_y) / this.grid);

    var x = (gx * this.grid);
    var y = (gy * this.grid);

    // check boundaries
    if (x < 0)
    {
      x = 0;
    }

    if (y < 0)
    {
      y = 0;
    }

    if (x > elem_rect.width - this.btn_mv_w)
    {
      x = elem_rect.width - this.btn_mv_w;
    }

    if (y > elem_rect.height - this.btn_mv_h)
    {
      y = elem_rect.height - this.btn_mv_h;
    }

    return {x: x, y: y};
  }

  //-----------------------------------------------------------------------------

  private handle_move_start (event, dom_el, box: GraphBox)
  {
    if (event.which === 1)
    {
      event.preventDefault();

      //this.action_ref = id;
      this.btn_mv_active = box;

      // get the top and left coordinates of the box element
      var el_rect = dom_el.getBoundingClientRect ();

      // get the top and left coordinates of the box element
      var box_rect = box.dom_box.getBoundingClientRect ();

      // calculate the delta to the current mouse position
      this.mv_x = event.clientX + el_rect.left - box_rect.left;
      this.mv_y = event.clientY + el_rect.top - box_rect.top;

      this.btn_mv_w = box_rect.width;
      this.btn_mv_h = box_rect.height;

      console.log('x = ' + event.clientX);
      console.log('y = ' + event.clientY);

      console.log('x = ' + this.mv_x + ', bx = ' + box_rect.left + ', ex = ' + el_rect.left);
      console.log('y = ' + this.mv_y + ', by = ' + box_rect.top + ', ey = ' + el_rect.top);
    }
  }

  //-----------------------------------------------------------------------------

  private handle_move (event)
  {
    if (this.btn_mv_active)
    {
      const v = this.calc_coordinates (event.clientX, event.clientY);

      if (this.option_mv_x)
      {
        // change X position
        this.rd.setStyle (this.btn_mv_active.dom_box, 'left', '' + v.x + 'px');
      }

      if (this.option_mv_y)
      {
        // change Y position
        this.rd.setStyle (this.btn_mv_active.dom_box, 'top', '' + v.y + 'px');
      }
    }
  }

  //-----------------------------------------------------------------------------

  private handle_move_end (event)
  {
    if (event.which === 1 && this.btn_mv_active)
    {
      const dom_el = this.el_dom.nativeElement;

      // get the top and left coordinates of the box element
      var elem_rect = dom_el.getBoundingClientRect ();

      const v = this.calc_coordinates (event.clientX, event.clientY);

      this.box_set_coordinates (this.btn_mv_active, v.x, v.y, elem_rect);

      if (this.on_mv_cb)
      {
        this.on_mv_cb (this.btn_mv_active);
      }

      this.btn_mv_active = null;
    }
  }

  //-----------------------------------------------------------------------------

  private init__events (dom_el): void
  {
    this.rd.listen (dom_el, 'mousemove', (event) => this.handle_move(event));
    this.rd.listen (dom_el, 'touchmove', (event) => this.handle_move(event));

    this.rd.listen (dom_el, 'mouseup', (event) => this.handle_move_end(event));
    this.rd.listen (dom_el, 'touchend', (event) => this.handle_move_end(event));
  }

  //-----------------------------------------------------------------------------

  private add_box__append_button_ed (dom_box, id: number, user_obj: any, width: number, height: number)
  {
    let dom_btn = this.rd.createElement('button');

    this.rd.setProperty (dom_btn, 'innerHTML', '<i class="fa fa-pen"></i>');

    this.rd.listen (dom_btn, 'click', () => {

      if (this.on_edit_cb)
      {
        this.on_edit_cb (id, user_obj);
      }

    });

    // add class
    this.rd.addClass (dom_btn, 'box-button-ed');

    // set position
    this.rd.setStyle (dom_btn, 'top', '4' + 'px');
    this.rd.setStyle (dom_btn, 'left', '4' + 'px');

    this.rd.appendChild (dom_box, dom_btn);
  }

  //-----------------------------------------------------------------------------

  private add_box__append_button_rm (dom_box, id: number, width: number, height: number)
  {
    let dom_btn = this.rd.createElement('button');

    this.rd.setProperty (dom_btn, 'innerHTML', '<i class="fa fa-times"></i>');

    this.rd.listen (dom_btn, 'click', () => {

      if (this.on_delete_cb)
      {
        this.on_delete_cb (id);
      }

    });

    // add class
    this.rd.addClass (dom_btn, 'box-button-rm');

    // set position
    this.rd.setStyle (dom_btn, 'top', '4' + 'px');
    this.rd.setStyle (dom_btn, 'left', (width - 24) + 'px');

    this.rd.appendChild (dom_box, dom_btn);
  }

  //-----------------------------------------------------------------------------

  private add_mv (dom_el, box: GraphBox)
  {
    this.rd.listen (box.dom_box, 'mousedown', (event) => this.handle_move_start(event, dom_el, box));
    this.rd.listen (box.dom_box, 'touchstart', (event) => this.handle_move_start(event, dom_el, box));
  }

  //-----------------------------------------------------------------------------

  private add_box__append_button_mv (dom_el, dom_box, id: number, width: number, height: number, x: number, y: number)
  {
    let dom_btn = this.rd.createElement('button');

    this.rd.setProperty (dom_btn, 'innerHTML', '<i class="fa fa-arrows-alt"></i>');

    this.add_mv (dom_btn, dom_box);

    // add class
    this.rd.addClass (dom_btn, 'box-button-mv');

    // set position
    this.rd.setStyle (dom_btn, 'top', (height - 24) + 'px');
    this.rd.setStyle (dom_btn, 'left', (width - 24) + 'px');

    this.rd.appendChild (dom_box, dom_btn);
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

  public add_box (id: number, x: number, y: number, names: object, colors: object, user_obj: any): void
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
    box.w = 10;
    box.h = 20;
    box.dom_box = this.rd.createElement('div');

    this.boxes.push (box);

    this.rd.setProperty (box.dom_box, 'id', 'G_' + id);

    this.adjust_box (box, elem_rect);

    // add class
    this.rd.addClass(box.dom_box, 'box-el');

    // add all internal elements
    this.add_box__append_interieur (box.dom_box);

    // add all small buttons visible by hover
    //this.add_box__append_button_ed (dom_box, id, user_obj, width, height);
    //this.add_box__append_button_rm (dom_box, id, width, height);
    //this.add_box__append_button_mv (dom_el, dom_box, id, width, height, x, y);

    this.add_mv (dom_el, box);

    this.set_box_attributes (box.dom_box, names, colors);
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

    // add class
    this.rd.appendChild (dom_el, image);
    this.rd.setStyle (image, 'width', '100%');

/*
    let unlistener = this.rd.listen (image, 'onload', (event) => {

      unlistener ();

    });
    */
  }

  //-----------------------------------------------------------------------------

}

export class GraphBox
{
  x: number;
  y: number;
  w: number;
  h: number;

  id: number;
  dom_box;
}
