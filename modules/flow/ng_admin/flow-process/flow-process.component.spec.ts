import { async, ComponentFixture, TestBed } from '@angular/core/testing';

import { FlowProcessComponent } from './flow-process.component';

describe('FlowProcessComponent', () => {
  let component: FlowProcessComponent;
  let fixture: ComponentFixture<FlowProcessComponent>;

  beforeEach(async(() => {
    TestBed.configureTestingModule({
      declarations: [ FlowProcessComponent ]
    })
    .compileComponents();
  }));

  beforeEach(() => {
    fixture = TestBed.createComponent(FlowProcessComponent);
    component = fixture.componentInstance;
    fixture.detectChanges();
  });

  it('should create', () => {
    expect(component).toBeTruthy();
  });
});
