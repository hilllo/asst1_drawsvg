#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {

float tripleMin(float a, float b, float c);
float tripleMax(float a, float b, float c);

// Implements SoftwareRenderer //

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = canvas_to_screen;

  // draw all elements
  /* 
   accepts an SVG file, and draws all elements in the SVG file via 
   a sequence of calls to draw_element().
  */
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y++;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y++;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y--;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y--;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

// buffer "render target": 
// the values in this buffer are values that will be displayed on screen.
// be called whenever the user resizes the application window.
void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 3: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;

}


void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 3: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;

}

/*
 inspects the type of the element, calls the appropriate draw function.
*/
void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 4 (part 1):
  // Modify this to implement the transformation stack

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position); //transform the input into its screen-space position
  rasterize_point( p.x, p.y, point.style.fillColor ); //actually drawing the point

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  //fill in the nearest pixel
  int sx = (int) floor(x);
  int sy = (int) floor(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  // fill sample - NOT doing alpha blending!
  render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (color.r * 255);
  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);

}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {
  // Task 1: 
  // Implement line rasterization

  // linear equation solution
  // bool steep = abs(y1-y0)>abs(x1-x0);
  // if(steep){ // reflected by y=x, swap x and y
  //   swap(x0,y0);
  //   swap(x1,y1);
  // }

  // double dx = x1-x0;
  // double dy = y1-y0;
  // double k = dy / dx;
  // double temp;

  // if(x1>x0){
  //     for(int x = x0,y = y0;x<x1;x++){ 
  //     temp = k*((double) x -x0)+y0;
  //     y = temp;
  //     if(steep){
  //       rasterize_point(y,x,color);
  //     }
  //     else{
  //       rasterize_point(x,y,color);
  //     }
  //   }
  // }
  // else{
  //     for(int x = x0,y = y0;x>x1;x--){ 
  //     temp = k*((double) x -x0)+y0;
  //     y = temp;
  //     if(steep){
  //       rasterize_point(y,x,color);
  //     }
  //     else{
  //       rasterize_point(x,y,color);
  //     }
  //   }
  // }
    
  // Bersenham algorithm solution
  bool steep = abs(y1-y0) > abs(x1-x0);
  
  if(steep){ // reflected by y=x, swap x and y
    // return;
    swap(x0,y0);
    swap(x1,y1);
  }
  if(x0>x1){ // if line start from top-right to bottom left, swap starting point and end point
    // return;
    swap(x0,x1);
    swap(y0,y1);
  }

  float dx = x1-x0;
  float dy = abs(y1-y0);
  float err = 0;
  float derr = dy/dx;
  int ystep;


  if(y1<y0){ // determines the increasing direction of y
    ystep = -1;
  }
  else {
    ystep = 1;
  }

  for(int x = x0,y = y0;x<x1;x++){
    if(steep){
      rasterize_point(y,x,color);
    }
    else{
      rasterize_point(x,y,color);
    }
    err += derr;
    if(err>=0.5){
      y+=ystep;
      err-=1.0;
    }

  }

}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 2: 
  // Implement triangle rasterization

  float minX;
  float minY;
  float maxX;
  float maxY;

  int x,y,i;

  rasterize_line(x0,y0,x1,y1,color);
  rasterize_line(x1,y1,x2,y2,color);
  rasterize_line(x2,y2,x0,y0,color);

  minX = tripleMin(x0,x1,x2);
  minY = tripleMin(y0,y1,y2);

  maxX = tripleMax(x0,x1,x2);
  maxY = tripleMax(y0,y1,y2);
  
  // int dx[5] = {1,2,3,4,5};

  float dx[3] = {x1-x0,x2-x1,x0-x2};
  float dy[3] = {y1-y0,y2-y1,y0-y2};
  float c[3] = {dx[0]*y0-dy[0]*x0,dx[1]*y1-dy[1]*x1,dx[2]*y2-dy[2]*x2};

  float e;

  for(x = minX;x<maxX;x++){
    for(y = minY;y<maxY;y++){
      for(i = 0;i<3;i++){
        e = dy[i] * (float)x -dx[i] * (float)y + c[i];
        if(e>0){
          break;
        }
      } 
      if(i>=3){
        rasterize_point(x,y,color);
      }
    }
  }

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task ?: 
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 3: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 3".
  return;

}

//functions
void swap(float *a, float *b){
  float temp;
  temp = *a;
  *a = *b;
  *b = temp;
}

float tripleMin(float a, float b, float c){
  if(b<a){
    a = b;
  }
  if(c<a){
    a = c;
  }
  return a;
}

float tripleMax(float a, float b, float c){
  if(b>a){
    a = b;
  }
  if(c>a){
    a = c;
  }
  return a;
}



} // namespace CMU462

