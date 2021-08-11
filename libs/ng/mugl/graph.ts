import { ElementRef, TemplateRef, EmbeddedViewRef, Renderer2 } from '@angular/core';

export class Graph {

  private on_delete_cb;
  private on_mv_cb;
  private on_edit_cb;
  private mouse_button_down: boolean;

  private btn_mv_active: any;
  private action_ref: number;

  private mv_x: number;
  private mv_y: number;

  private grid: number;

  private option_mv_x: boolean;
  private option_mv_y: boolean;

  //-----------------------------------------------------------------------------

  constructor (private el_dom: ElementRef, private el_box: TemplateRef<any>, private rd: Renderer2)
  {
    this.on_delete_cb = undefined;
    this.on_mv_cb = undefined;
    this.on_edit_cb = undefined;
    this.mouse_button_down = false;

    this.btn_mv_active = undefined;
    this.action_ref = 0;

    this.grid = 10;

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

  private calc_coordinates (ex: number, ey: number): {x: number, y: number}
  {
    const dom_el = this.el_dom.nativeElement;

    // get the top and left coordinates of the box element
    var elem_rect = dom_el.getBoundingClientRect ();

    var gx = Math.round((ex - this.mv_x) / this.grid);
    var gy = Math.round((ey - this.mv_y) / this.grid);

    return {x: (gx * this.grid), y: (gy * this.grid)};
  }

  //-----------------------------------------------------------------------------

  private init__events (dom_el): void
  {
    this.rd.listen (dom_el, 'mousemove', (event) => {

      if (this.mouse_button_down)
      {
        if (this.btn_mv_active)
        {
          const v = this.calc_coordinates (event.clientX, event.clientY);

          if (this.option_mv_x)
          {
            // change X position
            this.rd.setStyle (this.btn_mv_active, 'left', '' + v.x + 'px');
          }
          if (this.option_mv_y)
          {
            // change Y position
            this.rd.setStyle (this.btn_mv_active, 'top', '' + v.y + 'px');
          }
        }
      }

    });

    this.rd.listen (dom_el, 'mouseup', (event) => {

      if (event.which === 1 && this.mouse_button_down)
      {
        this.mouse_button_down = false;

        if (this.btn_mv_active)
        {
          if (this.on_mv_cb)
          {
            const v = this.calc_coordinates (event.clientX, event.clientY);

            this.on_mv_cb (this.action_ref, v.x, v.y);
          }

          this.btn_mv_active = false;
        }
      }

    });
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

  private add_box__append_button_mv (dom_el, dom_box, id: number, width: number, height: number, x: number, y: number)
  {
    let dom_btn = this.rd.createElement('button');

    this.rd.setProperty (dom_btn, 'innerHTML', '<i class="fa fa-arrows-alt"></i>');

    this.rd.listen (dom_btn, 'mousedown', (event) => {

      if (event.which === 1)
      {
        this.mouse_button_down = true;

        this.btn_mv_active = dom_box;
        this.action_ref = id;

        // get the top and left coordinates of the box element
        var el_rect = dom_el.getBoundingClientRect ();

        // get the top and left coordinates of the box element
        var box_rect = dom_box.getBoundingClientRect ();

        // calculate the delta to the current mouse position
        this.mv_x = event.clientX + el_rect.left - box_rect.left;
        this.mv_y = event.clientY + el_rect.top - box_rect.top;

        console.log('x = ' + event.clientX);
        console.log('y = ' + event.clientY);

        console.log('x = ' + this.mv_x + ', bx = ' + box_rect.left + ', ex = ' + el_rect.left);
        console.log('y = ' + this.mv_y + ', by = ' + box_rect.top + ', ey = ' + el_rect.top);
      }

    });

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

    let width = 100;
    let height = 120;

    let dom_box = this.rd.createElement('div');

    this.rd.setProperty (dom_box, 'id', 'G_' + id);

    x = ((x == null) ? 0 : x);
    y = ((y == null) ? 0 : y);

    // change position
    this.rd.setStyle (dom_box, 'left', x + 'px');
    this.rd.setStyle (dom_box, 'top', y + 'px');

    // change position
    this.rd.setStyle (dom_box, 'width', width + 'px');
    this.rd.setStyle (dom_box, 'height', height + 'px');

    // add class
    this.rd.addClass(dom_box, 'box-el');

    // add all internal elements
    this.add_box__append_interieur (dom_box);

    // add all small buttons visible by hover
    this.add_box__append_button_ed (dom_box, id, user_obj, width, height);
    this.add_box__append_button_rm (dom_box, id, width, height);
    this.add_box__append_button_mv (dom_el, dom_box, id, width, height, x, y);

    this.set_box_attributes (dom_box, names, colors);
    this.rd.appendChild (dom_el, dom_box);

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

  public set_background_image (data)
  {
    this.clear ();

    const dom_el = this.el_dom.nativeElement;

    // create a blob
    var blob = new Blob([data]);

    // create an URL to display the blob
    const fileURL = URL.createObjectURL(blob);

    var image = new Image();
    image.src = fileURL;

    // add class
    this.rd.appendChild (dom_el, image);
    this.rd.setStyle (image, 'width', '100%');
  }

  //-----------------------------------------------------------------------------

}
