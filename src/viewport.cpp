#include "viewport.h"

#include <stdio.h>
#include <iostream>

#include "CMU462.h"

namespace CMU462 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 4 (part 2):
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.

  Matrix3x3 m = get_canvas_to_norm();

  // m.zero();
  m(0,0) = (double)(1/(2*span));
  m(0,2) = (double)((span-x)/(2*span));
  m(1,1) = m(0,0);
  m(1,2) = (double)((span-y)/(2*span));
  m(2,2) = 1;

  // for(int i=0;i<3;i++){
  //   std::cout<<m(i,0)<<", "<<m(i,1)<<", "<<m(i,2)<<std::endl;
  // }
  // Matrix3x3 t;
  // Matrix3x3 s;
  //
  // t.zero();
  // t(0,0) = 1;
  // t(0,2) = (double)(span-x);
  // t(1,1) = 1;
  // t(1,2) = (double)(span-y);
  // t(2,2) = 1;
  //
  // s.zero();
  // s(0,0) = (double)(1/(2*span));
  // s(1,1) = (double)(1/(2*span));
  // s(2,2) = 1;


  set_canvas_to_norm(m);

  this->x = x;
  this->y = y;
  this->span = span;

}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) {

  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CMU462
