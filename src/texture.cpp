#include "texture.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE(sky):
  // The starter code allocates the mip levels and generates a level
  // map simply fills each level with a color that differs from its
  // neighbours'. The reference solution uses trilinear filtering
  // and it will only work when you have mipmaps.

  // Task 7: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level";
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors
  // Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  // for(size_t i = 1; i < tex.mipmap.size(); ++i) {
  //
  //   Color c = colors[i % 3];
  //   MipLevel& mip = tex.mipmap[i];
  //
  //   for(size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
  //     float_to_uint8( &mip.texels[i], &c.r );
  //   }
  // }

  // fill all levels
  // method as supersampling
  // std::cout<<"levels = "<<tex.mipmap.size()<<endl;

  unsigned char temp[4];
  int k;
  int last_width,this_width;
  for(size_t l = 1; l < tex.mipmap.size(); ++l) { //level
    last_width = tex.mipmap[l-1].width;
    this_width = tex.mipmap[l].width;

    // std::cout<<"l = "<<l;
    // std::cout<<", last width = "<<last_width;
    // std::cout<<", this width = "<<this_width<<endl;

    for(int x = 0;x<tex.mipmap[l].width;x++){ //pixel
      for(int y = 0;y<tex.mipmap[l].height;y++){

        for(k=0;k<4;k++){
          temp[k] = 0;
        }

        // for(k=0;k<4;k++){
        //   // temp[k] += tex.mipmap[l-1].texels[4*((x*2) + (y*2)*last_width)+k];
        //   temp[k] += tex.mipmap[l-1].texels[4*((x*2) + (y*2+1)*last_width)+k];
        // }
        for(int i=0;i<2;i++){
          for(int j=0;j<2;j++){
            for(k=0;k<4;k++){
              temp[k] += tex.mipmap[l-1].texels[4*((x*2+i) + (y*2+j)*last_width)+k];
            }
          }
        }

        for(k = 0;k<4;k++){
          temp[k] /= 4;
          tex.mipmap[l].texels[4*(x + y*this_width)+k] = temp[k];
        }
      }
    }
  }

}

Color Sampler2DImp::sample_nearest(Texture& tex,
                                   float u, float v,
                                   int level) {


  // Task ?: Implement nearest neighbour interpolation
  // return magenta for invalid level
  if(level>tex.mipmap.size()){
    return Color(1,0,1,1);
  }

  int width = tex.mipmap[level].width;
  int height = tex.mipmap[level].height;
  u *=  width - 0.5;
  v *= height - 0.5;
  int x = floor(u+0.5);
  int y = floor(v+0.5);

  Color result;
  result.r = (float)(tex.mipmap[level].texels[4*(x + y*width)  ])/255;
  result.g = (float)(tex.mipmap[level].texels[4*(x + y*width)+1])/255;
  result.b = (float)(tex.mipmap[level].texels[4*(x + y*width)+2])/255;
  result.a = (float)(tex.mipmap[level].texels[4*(x + y*width)+3])/255;

  return result;


}

Color Sampler2DImp::sample_bilinear(Texture& tex,
                                    float u, float v,
                                    int level) {

  // Task ?: Implement bilinear filtering
  // return magenta for invalid level
  if(level>tex.mipmap.size()){
    return Color(1,0,1,1);
  }

  int width = tex.mipmap[level].width;
  int height = tex.mipmap[level].height;
  u *=  width - 0.5;
  v *= height - 0.5;
  int x = floor(u);
  int y = floor(v);
  float u_ratio = u - x;
  float v_ratio = v - y;
  float u_opposite = 1 - u_ratio;
  float v_opposite = 1 - v_ratio;

  Color result;
  float temp1,temp2;
  float rgba[4];
  for(int i=0;i<4;i++){
    temp1 = (float)(tex.mipmap[level].texels[4*(x+y*width) + i]) * u_opposite +
    (float)(tex.mipmap[level].texels[4*(x+1+y*width) + i]) * u_ratio;
    temp2 = (float)(tex.mipmap[level].texels[4*(x+(y+1)*width) + i]) * u_opposite +
    (float)(tex.mipmap[level].texels[4*(x+1+(y+1)*width) + i]) * u_ratio;
    rgba[i] = temp1*v_opposite + temp2*v_ratio;
  }
  result.r = rgba[0]/255;
  result.g = rgba[1]/255;
  result.b = rgba[2]/255;
  result.a = rgba[3]/255;



  return result;


}

Color Sampler2DImp::sample_trilinear(Texture& tex,
                                     float u, float v,
                                     float u_scale, float v_scale) {

  // Task 8: Implement trilinear filtering

  // return magenta for invalid level
  // return Color(1,0,1,1);
  Color result;
  if(u_scale>tex.mipmap[0].width||v_scale>tex.mipmap[0].height){
    result = sample_bilinear(tex,u,v,0);
    return result;
    // return Color(1,0,1,1);
  }

  int upper_level,lower_level;
  float max_scale;

  if(u_scale>v_scale){
    max_scale = u_scale;
  }
  else{
    max_scale = v_scale;
  }

  float src_level = log2(max_scale);
  src_level = (float)(tex.mipmap.size())-src_level-1.0;

  upper_level = ceil(src_level);
  lower_level = upper_level-1; //floor(src_level);

  // std::cout<<src_level<<" "<<upper_level<<" "<<lower_level<<endl;

  float ul_weight = src_level - lower_level;
  float ll_weight = 1-ul_weight; //src_level - upper_level;

  Color ulc = sample_bilinear(tex,u,v,upper_level);
  Color llc = sample_bilinear(tex,u,v,lower_level);

  result.r = ulc.r * ul_weight + llc.r * ll_weight;
  result.g = ulc.g * ul_weight + llc.g * ll_weight;
  result.b = ulc.b * ul_weight + llc.b * ll_weight;
  result.a = ulc.a * ul_weight + llc.a * ll_weight;


  return result;
}

} // namespace CMU462
