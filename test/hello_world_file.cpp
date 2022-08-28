// Copyright 2012 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include "png.h"


void read_row_callback(png_structp png_ptr, png_uint_32 row, int pass)
{
  printf("row: %u, pass: %i\n", row, pass);
}



SDL_Surface *createRGBSurfaceFromPNG(void *png_buff, int width, int height, int depth, int pitch) {
  // TODO: Take into account depth and pitch parameters.

  SDL_RWops* buff;
  SDL_Surface *surface;
  int size = height*pitch;

  buff = SDL_RWFromMem(png_buff, size);
  surface = IMG_LoadPNG_RW(buff);

  return surface;
}




int main() {
  

  png_bytepp row_pointers;
  SDL_Surface *screen;
  SDL_Surface *surface;
  Uint32 rMask, gMask, bMask, aMask;
  int height;
  int width;   
  int depth;
  int pitch;   


  FILE *pfile = fopen("test/cube_explosion.png", "rb");
  // FILE *pfile = fopen("test/hello_world_file.txt", "rb");
  if (!pfile) {
    printf("cannot open file\n");
    return 1;
  }
  
  unsigned char sig[8];

  fread(sig, 1, 8, pfile);
  if (!png_check_sig(sig, 8)) {
    printf("bad signature\n");
    return 2;   /* bad signature */
  } else {
    printf("this is a PNG file\n");
  }


  png_structp png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);

  if (!png_ptr) {
      return 3;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if (!info_ptr) {
      png_destroy_read_struct(&png_ptr,
          (png_infopp)NULL, (png_infopp)NULL);
      return 4;
  }


  if (setjmp(png_jmpbuf(png_ptr)))
  {
      png_destroy_read_struct(&png_ptr, &info_ptr,
          (png_infopp)NULL);
      fclose(pfile);
      return 5;
  }

  
  png_init_io(png_ptr, pfile);

  png_set_sig_bytes(png_ptr, 8);

  png_set_read_status_fn(png_ptr, read_row_callback);

  png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  height = (unsigned int)png_get_image_height(png_ptr, info_ptr);
  width = (unsigned int)png_get_image_width(png_ptr, info_ptr);   
  depth = (unsigned int)png_get_bit_depth(png_ptr, info_ptr);
  pitch = (unsigned int)png_get_rowbytes(png_ptr, info_ptr);

  printf("Image height is %u \n", (unsigned int)png_get_image_height(png_ptr, info_ptr)); 
  printf("Image width is %u \n", (unsigned int)png_get_image_width(png_ptr, info_ptr)); 
  printf("BitsPerPixel is %u \n", (unsigned int)png_get_bit_depth(png_ptr, info_ptr));
  // printf("BytesPerPixel is %u \n", png_get_pixel_aspect_ratio(png_ptr, info_ptr));
  printf("Image stride is %u \n", (unsigned int)png_get_rowbytes(png_ptr, info_ptr));

  fclose (pfile);

  // row_pointers = png_get_rows(png_ptr, info_ptr);


  // //do transform with row_pointers


  // //write transformed row pointers to png_out


  // surface = createRGBSurfaceFromPNG((void *)png_out, width, height, depth, pitch);
  // if(!surface) {
  //    printf("Could not create surface: %s\n", SDL_GetError());
  //    return 0;
  // }  


  // screen = SDL_SetVideoMode(surface->w, surface->h, depth, SDL_SWSURFACE);

  // SDL_BlitSurface(surface, NULL, screen, NULL);
  // SDL_FreeSurface(surface);

  // SDL_Flip(screen);

  // SDL_Quit();




  return 0;
}

