import { Component, ViewChild, ElementRef, AfterViewInit, HostListener } from '@angular/core';
import { Observable } from 'rxjs';
import { SizeService, SCREEN_SIZE } from './service';

//-----------------------------------------------------------------------------

@Component({
  selector: 'size-detector',
  templateUrl: './component.html'
})
export class SizeDetectorComponent implements AfterViewInit {
  prefix = 'is-';
  sizes = [
    {
      id: SCREEN_SIZE.XS, name: 'xs', css: `d-block d-sm-none`
    },
    {
      id: SCREEN_SIZE.SM, name: 'sm', css: `d-none d-sm-block d-md-none`
    },
    {
      id: SCREEN_SIZE.MD, name: 'md', css: `d-none d-md-block d-lg-none`
    },
    {
      id: SCREEN_SIZE.LG, name: 'lg', css: `d-none d-lg-block d-xl-none`
    },
    {
      id: SCREEN_SIZE.XL, name: 'xl', css: `d-none d-xl-block`
    },
  ];

  //-----------------------------------------------------------------------------

  @HostListener("window:resize", []) private onResize()
  {
    this.on_resize();
  }

  //-----------------------------------------------------------------------------

  constructor(private elementRef: ElementRef, private size_service: SizeService)
  {
  }

  //-----------------------------------------------------------------------------

  ngAfterViewInit()
  {
    this.on_resize();
  }

  //-----------------------------------------------------------------------------

  private on_resize()
  {
    const currentSize = this.sizes.find(x => {
      // get the HTML element
      const el = this.elementRef.nativeElement.querySelector(`.${this.prefix}${x.id}`);

      // check its display property value
      const isVisible = window.getComputedStyle(el).display != 'none';

      return isVisible;
    });

    this.size_service.set (currentSize.id);
  }
}

//-----------------------------------------------------------------------------
