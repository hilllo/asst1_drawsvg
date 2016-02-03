#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462 {



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

  transformation = canvas_to_screen;

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y++;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y++;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y--;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y--;

  rasterize_line(a.x, a.y-2, b.x, b.y-2, Color::Black);
  rasterize_line(a.x, a.y-2, c.x, c.y+2, Color::Black);
  rasterize_line(d.x, d.y+2, b.x, b.y-2, Color::Black);
  rasterize_line(d.x, d.y+2, c.x, c.y+2, Color::Black);

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
  this->clear_sample();


}


void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 3:
  // You may want to modify this for supersampling support

  // free(supersample_target);

  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
  this->clear_sample();
}


/*
 inspects the type of the element, calls the appropriate draw function.
*/
void SoftwareRendererImp::draw_element( SVGElement* element ) {

  transformation = transformation * element->transform;

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
      // break;
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

  transformation = transformation * element->transform.inv();

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
  fill_pixel(sx,sy,color);
  // render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (color.r * 255);
  // render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
  // render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
  // render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);
}

float SoftwareRendererImp::fpart(float x){
  if(x<0)
    return 1.0-(x-floor(x));
  else
    return x-floor(x);
}

float SoftwareRendererImp::rfpart(float x){
  return 1.0-fpart(x);
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {
  // Task 1:
  // Implement line rasterization
  x0 *= sample_rate;
  y0 *= sample_rate;
  x1 *= sample_rate;
  y1 *= sample_rate;

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

  //Xiaolin Wu's Solution
  float dx = x1-x0;
  float dy = y1-y0;
  float gradient = dy/dx;



  float xend = floor(x0)+0.5;
  float yend = y0 + gradient * (xend-x0);
  float xgap = rfpart(x0);
  int xpxl1 = floor(x0);
  int ypxl1 = floor(yend - 0.5);

  float intery = yend - ypxl1 -0.5;

  if(steep){
    color.a = rfpart(intery) * xgap;
    fill_sample(ypxl1,xpxl1,color);
    color.a = fpart(intery) * xgap;
    fill_sample(ypxl1+1,xpxl1,color);
  }
  else{
    color.a = rfpart(intery) * xgap;
    fill_sample(xpxl1,ypxl1,color);
    color.a = fpart(intery) * xgap;
    fill_sample(xpxl1,ypxl1+1,color);
  }



  xend = floor(x1)+0.5;
  yend = y1 + gradient * (xend-x1);
  xgap = fpart(x1);
  int xpxl2 = floor(x1);
  int ypxl2 = floor(yend - 0.5);

  intery = yend - ypxl2 -0.5;

  if(steep){
    color.a = rfpart(intery)*xgap;
    fill_sample(ypxl2,xpxl2,color);
    color.a = fpart(intery)*xgap;
    fill_sample(ypxl2+1,xpxl2,color);
  }
  else{
    color.a = rfpart(intery)*xgap;
    fill_sample(xpxl2,ypxl2,color);
    color.a = fpart(intery)*xgap;
    fill_sample(xpxl2,ypxl2+1,color);
  }

  float yline = y0 + (floor(x0)+1+0.5-x0)*gradient;
  for(int x = floor(x0)+1;x<=floor(x1)-1;x++){
    intery = yline - floor(yline - 0.5) - 0.5;

    if(steep){
      color.a = rfpart(intery);
      fill_sample(floor(yline - 0.5) , x , color);
      color.a = fpart(intery);
      fill_sample(floor(yline - 0.5)+1 , x , color);
    }
    else{
      color.a = rfpart(intery);
      fill_sample(x , floor(yline - 0.5) ,  color);
      color.a = fpart(intery);
      fill_sample(x , floor(yline - 0.5)+1 ,  color);
    }
    yline += gradient;
  }

  // Bersenham algorithm solution
  // float dx = x1-x0;
  // float dy = abs(y1-y0);
  // float err = 0;
  // float derr = dy/dx;
  // int ystep;

  // if(y1<y0){ // determines the increasing direction of y
  //   ystep = -1;
  // }
  // else {
  //   ystep = 1;
  // }

  // for(int x = x0,y = y0;x<x1;x++){
  //   if(steep){
  //     rasterize_point(y,x,color);
  //   }
  //   else{
  //     rasterize_point(x,y,color);
  //   }
  //   err += derr;
  //   if(err>=0.5){
  //     y+=ystep;
  //     err-=1.0;
  //   }

  // }

  // TBD: Xiaolin Wu's method


}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 2:
  // Implement triangle rasterization
  if((x0==x1&&x1==x2)||(y0==y1&&y1==y2)){
    std::cout<< "This is not a triangle!"<<endl;
    return;
  }

  x0 *= sample_rate;
  x1 *= sample_rate;
  x2 *= sample_rate;
  y0 *= sample_rate;
  y1 *= sample_rate;
  y2 *= sample_rate;

  /* Another Method
    if(y0>y1){
      swap(x0,x1);
      swap(y0,y1);
    }
    if(y0>y2){
      swap(x0,x2);
      swap(y0,y2);
    }
    if(y1>y2){
      swap(x1,x2);
      swap(y1,y2);
    }
      cout<<"("<<x0<<", "<<y0<<") ";
      cout<<"("<<x1<<", "<<y1<<") ";
      cout<<"("<<x2<<", "<<y2<<") "<<endl;

    if(y0==y1){
      top_triangle(x0,y0,x1,y1,x2,y2,color);
    }
    else if(y1 == y2){
      bottom_triangle(x0,y0,x1,y1,x2,y2,color);
    }
    else{
      float nx;
      if((int)y2-(int)y0==0){
        nx = x0;
      }
      else{
        nx = x0+(y1-y0)*(x2-x0)/(y2-y0);
      }
      cout<<"new x:"<<"("<<nx<<", "<<y1<<") "<<endl;
      top_triangle(nx,y1,x1,y1,x2,y2,color);
      bottom_triangle(x0,y0,nx,y1,x1,y1,color);
    }
  */

  // Rectangle Method
  // CCW triangle
  // Step 1: choose the toppest point as (x0,y0)
  if(y0>y1){
    swap(x0,x1);
    swap(y0,y1);
  }
  if(y0>y2){
    swap(x0,x2);
    swap(y0,y2);
  }

  // Step 2: draw a line between (x0,y0) and (x2,y2)
  float dxtemp = x0-x2;
  float dytemp = y0-y2;
    // judge whether (x1,x2) should switch with (x2,y2)
  if(dxtemp == 0){
    if(x1<x2){
      swap(x1,x2);
      swap(y1,y2);
    }
  }
  else{
    float dtemp = dytemp / dxtemp;
    if(dxtemp<0){
      if(y1>dtemp*(x1-x0)+y0){
        swap(x1,x2);
        swap(y1,y2);
      }
    }
    else if(dxtemp>0){
      if(y1<dtemp*(x1-x0)+y0){
        swap(x1,x2);
        swap(y1,y2);
      }
    }
  }


  // cout<<"Rectangle:"<<endl;
  // cout<<"("<<x0<<", "<<y0<<") ";
  // cout<<"("<<x1<<", "<<y1<<") ";
  // cout<<"("<<x2<<", "<<y2<<") "<<endl;

  float minX;
  float minY;
  float maxX;
  float maxY;

  int x,y,i;

  // rasterize_line(x0,y0,x1,y1,color);
  // rasterize_line(x1,y1,x2,y2,color);
  // rasterize_line(x2,y2,x0,y0,color);

  minX = tripleMin(x0,x1,x2);
  minY = y0;

  maxX = tripleMax(x0,x1,x2);
  maxY = tripleMax(y0,y1,y2);


  float dx[3] = {x1-x0,x2-x1,x0-x2};
  float dy[3] = {y1-y0,y2-y1,y0-y2};
  float c[3] = {dx[0]*y0-dy[0]*x0,dx[1]*y1-dy[1]*x1,dx[2]*y2-dy[2]*x2};


  float e;

  for(x = floor(minX)-1;x<=floor(maxX);x++){
    for(y = floor(minY)-1;y<=floor(maxY);y++){
      for(i = 0;i<3;i++){
        e = dy[i] * (float)x -dx[i] * (float)y + c[i];
        // cout<<"e = "<<e<<endl;
        if(e>0){
          break;
        }
      }
      if(i>=3){

        fill_sample(x,y,color);
        // supersample_target[4 * (x + y * target_w)    ] = (uint8_t) (color.r * 255);
        // supersample_target[4 * (x + y * target_w) + 1] = (uint8_t) (color.g * 255);
        // supersample_target[4 * (x + y * target_w) + 2] = (uint8_t) (color.b * 255);
        // supersample_target[4 * (x + y * target_w) + 3] = (uint8_t) (color.a * 255);

        // render_target[4 * (x + y * target_w)    ] = (uint8_t) (color.r * 255);
        // render_target[4 * (x + y * target_w) + 1] = (uint8_t) (color.g * 255);
        // render_target[4 * (x + y * target_w) + 2] = (uint8_t) (color.b * 255);
        // render_target[4 * (x + y * target_w) + 3] = (uint8_t) (color.a * 255);
      }
    }


  }

  // TBD:faster and anti-alias

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 5:
  // Implement image rasterization

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 3:
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 3".
    float weight = sample_rate * sample_rate;
    float temp[4] = {0,0,0,0};
    int k;

    for(int x = 0;x<target_w;x++){
      for(int y=0;y<target_h;y++){
        for(k=0;k<4;k++){
          temp[k] = 0;
        }

          for(int i=0;i<sample_rate;i++){
            for(int j=0;j<sample_rate;j++){
              for(k=0;k<4;k++){
                temp[k] += sample_buffer[4*(x*sample_rate+i + (y*sample_rate+j) * target_w*sample_rate) + k];
              }
            }
          }

          for(k=0;k<4;k++){
            temp[k] /= weight;
            render_target[4 * (x + y * target_w)+k] = temp[k];
          }
      }
    }





  memset(&sample_buffer[0], 255, 4 * sample_rate * sample_rate * target_h * target_w);
  return;

}

void SoftwareRendererImp::fill_pixel(int sx, int sy, const Color& c){

  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;
  // alpha blending
  Color origin,target;

  origin.r = (float)render_target[4 * (sx + sy * target_w)]/255;
  origin.g = (float)render_target[4 * (sx + sy * target_w)+1]/255;
  origin.b = (float)render_target[4 * (sx + sy * target_w)+2]/255;
  origin.a = (float)render_target[4 * (sx + sy * target_w)+3]/255;

  target.r = (1-c.a) * origin.r+c.a * c.r;
  target.g = (1-c.a) * origin.g+c.a * c.g;
  target.b = (1-c.a) * origin.b+c.a * c.b;
  target.a = 1-(1-origin.a)*(1-c.a);


  render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (target.r * 255);
  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (target.g * 255);
  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (target.b * 255);
  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (target.a * 255);
}

void SoftwareRendererImp::fill_sample(int sx, int sy, const Color& c){

  if ( sx < 0 || sx >= target_w*sample_rate ) return;
  if ( sy < 0 || sy >= target_h*sample_rate ) return;
  // alpha blending
  Color origin,target;

  origin.r = (float)sample_buffer[4 * (sx + sy * target_w*sample_rate)]/255;
  origin.g = (float)sample_buffer[4 * (sx + sy * target_w*sample_rate)+1]/255;
  origin.b = (float)sample_buffer[4 * (sx + sy * target_w*sample_rate)+2]/255;
  origin.a = (float)sample_buffer[4 * (sx + sy * target_w*sample_rate)+3]/255;

  target.r = (1-c.a) * origin.r+c.a * c.r;
  target.g = (1-c.a) * origin.g+c.a * c.g;
  target.b = (1-c.a) * origin.b+c.a * c.b;
  target.a = 1-(1-origin.a)*(1-c.a);


  sample_buffer[4 * (sx + sy * target_w*sample_rate)    ] = (uint8_t) (target.r * 255);
  sample_buffer[4 * (sx + sy * target_w*sample_rate) + 1] = (uint8_t) (target.g * 255);
  sample_buffer[4 * (sx + sy * target_w*sample_rate) + 2] = (uint8_t) (target.b * 255);
  sample_buffer[4 * (sx + sy * target_w*sample_rate) + 3] = (uint8_t) (target.a * 255);
}

//functions
void swap(float & a, float & b){
  float temp;
  temp = a;
  a = b;
  b = temp;
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
