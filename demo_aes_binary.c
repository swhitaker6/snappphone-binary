// Copyright 2012 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "png.h"
#include "chacha20_simple.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>


// typedef struct {
//     const png_byte* bytes;
//     const png_size_t size;
// } DataBytes;

// typedef struct PNG {
//     const DataBytes data;
//     png_size_t offset;
// } PNGDataBytes;

struct {
	char point[4];
} user_ptr;

uint8_t key[32];
uint8_t nonce[8];
uint64_t counter = 0;

int num_row_bytes;
int BYTES_PER_PIXEL;
png_uint_32 png_offset = 0;
png_bytep row_array;
png_bytep *row_pointers;

png_structp read_ptr;
png_infop read_info_ptr, end_info_ptr;

png_structp write_ptr;
png_infop write_info_ptr;
png_infop write_end_info_ptr;
int interlace_preserved = 1;

png_bytep row_buf;
png_uint_32 y;
png_uint_32 width, height;
int num_pass = 1, pass;
int bit_depth, color_type;
int total_bytes = 0;
int file_size;
int offset = 0;

png_structp read_ptr;
png_structp write_ptr;
png_structp png_ptr;
png_infop read_info_ptr;
png_infop write_info_ptr;

int pngSize;
int bufferSize;
png_bytep pngBuffer;
png_bytep processedBytes;
int expansion_factor = 6;


void hex2byte(const char *hex, uint8_t *byte)
{
  
  while (*hex) { 
    
    sscanf(hex, "%2hhx", byte++); 
    hex += 2; 
    
  }

}



 void end_callback(png_structp png_ptr, png_infop info) {


}




 void row_callback(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass)
 {


    if(!row_pointers[row_num]) {

      row_pointers[row_num] = (png_bytep)png_malloc(png_ptr, num_row_bytes);

    }

    png_progressive_combine_row(png_ptr, row_pointers[row_num], new_row);

 

 }




 void info_callback(png_structp png_ptr, png_infop info)
 {
	    
      int channels;


      //Transferring info struct
      {
          int interlace_type, compression_type, filter_type;

          if (png_get_IHDR(read_ptr,read_info_ptr, &width, &height, &bit_depth,
              &color_type, &interlace_type, &compression_type, &filter_type) != 0)
          {
            png_set_IHDR(write_ptr, write_info_ptr, width, height, bit_depth,
                color_type, interlace_type, compression_type, filter_type);

            switch (interlace_type)
            {
                case PNG_INTERLACE_NONE:
                  num_pass = 1;
                  break;

                case PNG_INTERLACE_ADAM7:
                  num_pass = 7;
                    break;

                default:
                    png_error(read_ptr, "invalid interlace type");
                    /*NOT REACHED*/
            }

        num_row_bytes = png_get_rowbytes(png_ptr,read_info_ptr);

          }
      }
      //Transferring PNG_FIXED_POINT
      {
          //Transferring PNG_cHRM
          {
                png_fixed_point white_x, white_y, red_x, red_y, green_x, green_y, blue_x,
                blue_y;

                if (png_get_cHRM_fixed(read_ptr,read_info_ptr, &white_x, &white_y,
                &red_x, &red_y, &green_x, &green_y, &blue_x, &blue_y) != 0)
                {
                png_set_cHRM_fixed(write_ptr, write_info_ptr, white_x, white_y, red_x,
                      red_y, green_x, green_y, blue_x, blue_y);
                }
          }

          //Transferring PNG_gAMA
          {
                png_fixed_point gamma;

                if (png_get_gAMA_fixed(read_ptr,read_info_ptr, &gamma) != 0)
                png_set_gAMA_fixed(write_ptr, write_info_ptr, gamma);
          }
      }
      //Transferring PNG_FLOATING_POINT"
      {
          //Transferring PNG_cHRM_SUPPORTED
          {
                double white_x, white_y, red_x, red_y, green_x, green_y, blue_x,
                blue_y;

                if (png_get_cHRM(read_ptr,read_info_ptr, &white_x, &white_y, &red_x,
                &red_y, &green_x, &green_y, &blue_x, &blue_y) != 0)
                {
                png_set_cHRM(write_ptr, write_info_ptr, white_x, white_y, red_x,
                      red_y, green_x, green_y, blue_x, blue_y);
                }
          }

          //Transferring PNG_gAMA_SUPPORTED
          {
                double gamma;

                if (png_get_gAMA(read_ptr,read_info_ptr, &gamma) != 0)
                png_set_gAMA(write_ptr, write_info_ptr, gamma);
          }
      }
      // PNG_iCCP_SUPPORTED
      {
          png_charp name;
          png_bytep profile;
          png_uint_32 proflen;
          int compression_type;

          if (png_get_iCCP(read_ptr,read_info_ptr, &name, &compression_type,
                          &profile, &proflen) != 0)
          {
            png_set_iCCP(write_ptr, write_info_ptr, name, compression_type,
                          profile, proflen);
          }
      }
      // PNG_sRGB_SUPPORTED
      {
          int intent;

          if (png_get_sRGB(read_ptr,read_info_ptr, &intent) != 0)
            png_set_sRGB(write_ptr, write_info_ptr, intent);
      }

      {
          png_colorp palette;
          int num_palette;

          if (png_get_PLTE(read_ptr,read_info_ptr, &palette, &num_palette) != 0)
            png_set_PLTE(write_ptr, write_info_ptr, palette, num_palette);
      }
      // PNG_bKGD_SUPPORTED
      {
          png_color_16p background;

          if (png_get_bKGD(read_ptr,read_info_ptr, &background) != 0)
          {
            png_set_bKGD(write_ptr, write_info_ptr, background);
          }
      }
      // PNG_hIST_SUPPORTED
      {
          png_uint_16p hist;

          if (png_get_hIST(read_ptr,read_info_ptr, &hist) != 0)
            png_set_hIST(write_ptr, write_info_ptr, hist);
      }
      // PNG_oFFs_SUPPORTED
      {
          png_int_32 offset_x, offset_y;
          int unit_type;

          if (png_get_oFFs(read_ptr,read_info_ptr, &offset_x, &offset_y,
              &unit_type) != 0)
          {
            png_set_oFFs(write_ptr, write_info_ptr, offset_x, offset_y, unit_type);
          }
      }

      // PNG_pCAL_SUPPORTED
      {
          png_charp purpose, units;
          png_charpp params;
          png_int_32 X0, X1;
          int type, nparams;

          if (png_get_pCAL(read_ptr,read_info_ptr, &purpose, &X0, &X1, &type,
            &nparams, &units, &params) != 0)
          {
            png_set_pCAL(write_ptr, write_info_ptr, purpose, X0, X1, type,
                nparams, units, params);
          }
      }

      // PNG_pHYs_SUPPORTED
      {
          png_uint_32 res_x, res_y;
          int unit_type;

          if (png_get_pHYs(read_ptr,read_info_ptr, &res_x, &res_y,
              &unit_type) != 0)
            png_set_pHYs(write_ptr, write_info_ptr, res_x, res_y, unit_type);
      }

      // PNG_sBIT_SUPPORTED
      {
          png_color_8p sig_bit;

          if (png_get_sBIT(read_ptr,read_info_ptr, &sig_bit) != 0)
            png_set_sBIT(write_ptr, write_info_ptr, sig_bit);
      }

      // PNG_sCAL_SUPPORTED
      //PNG_FLOATING_POINT_SUPPORTED
      // PNG_FLOATING_ARITHMETIC_SUPPORTED
      {
          int unit;
          double scal_width, scal_height;

          if (png_get_sCAL(read_ptr,read_info_ptr, &unit, &scal_width,
            &scal_height) != 0)
          {
            png_set_sCAL(write_ptr, write_info_ptr, unit, scal_width, scal_height);
          }
      }

      // PNG_tRNS_SUPPORTED
      {
          png_bytep trans_alpha;
          int num_trans;
          png_color_16p trans_color;

          if (png_get_tRNS(read_ptr,read_info_ptr, &trans_alpha, &num_trans,
            &trans_color) != 0)
          {
            int sample_max = (1 << bit_depth);
            /* libpng doesn't reject a tRNS chunk with out-of-range samples */
            if (!((color_type == PNG_COLOR_TYPE_GRAY &&
                (int)trans_color->gray > sample_max) ||
                (color_type == PNG_COLOR_TYPE_RGB &&
                ((int)trans_color->red > sample_max ||
                (int)trans_color->green > sample_max ||
                (int)trans_color->blue > sample_max))))
                png_set_tRNS(write_ptr, write_info_ptr, trans_alpha, num_trans,
                  trans_color);
          }
      }


      // png_start_read_image(png_ptr);


 }





 int process_data(png_bytep buffer, png_uint_32 length)
 {
    if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr, &read_info_ptr,
      (png_infopp)NULL);
      return 1;
    }

    png_process_data(png_ptr, read_info_ptr, buffer, length);

    return 0;
 }





int initialize_png_reader()
{


    read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    
    if (!read_ptr) {
      return 1;
    }

   read_info_ptr = png_create_info_struct(read_ptr);

    if (!read_info_ptr)
    {
      png_destroy_read_struct(&read_ptr,
      (png_infopp)NULL, (png_infopp)NULL);
      return 2;
    }

    if (setjmp(png_jmpbuf(read_ptr)))
    {
      png_destroy_read_struct(&read_ptr, &read_info_ptr,
      (png_infopp)NULL);
      return 3;
    }

    png_set_progressive_read_fn(read_ptr, &user_ptr, info_callback, row_callback, end_callback);
    
    
    return 0;

}




void write_row_callback(png_structp png_ptr, png_uint_32 row, int pass)
{
  printf("row: %u, pass: %i\n", row, pass);
  
}




void read_row_callback(png_structp png_ptr, png_uint_32 row, int pass)
{
  // printf("row: %u, pass: %i\n", row, pass);
  
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






void png_read_fn(png_structp png_ptr, png_bytep data, size_t read_length) {

      png_bytep filebytes;

      png_byte* png_data;

      filebytes = (png_bytep)png_get_io_ptr(png_ptr);

      png_data = filebytes + offset;

      printf("read_length: %u\n", (unsigned int)read_length);

      offset += read_length;

      printf("offset: %u\n", (unsigned int)offset);

      memcpy(data, png_data, read_length);



}





void png_write_fn(png_structp png_ptr, png_bytep data, size_t read_length) {

      png_bytep filebytes;

      png_byte* png_data;

      filebytes = (png_bytep)png_get_io_ptr(png_ptr);

      png_data = filebytes + offset;

      printf("write_length: %u\n", (unsigned int)read_length);

      offset += read_length;

      printf("offset: %u\n", (unsigned int)offset);

      memcpy(png_data, data, read_length);



}





void output_flush_fn(png_structp png_ptr) {

    printf("output_flushed: %X\n", 1);


}



int main() {

  // hex2byte("1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0", key);
  // hex2byte("0000000000000002", nonce);

  
  const uint8_t *key = 0;
  chacha20_ctx ctx;
  unsigned int len = 0;
  unsigned char *outfilebytes = NULL;
  int ret = 1;
  // png_bytepp row_pointers;
  SDL_Surface *image;
  SDL_Surface *screen;
  SDL_Surface *surface;
  Uint32 rMask, gMask, bMask, aMask;
  int height;
  int width;   
  int depth;
  int channels;
  int row_bytes;
  int do_encrypt = 1;   


  FILE *pfile = fopen("images/sandscape.png", "rb");
  // FILE *pfile = fopen("images/hello_world_file.txt", "rb");
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


  // initialize_png_reader();
  read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);

  if (!read_ptr) {
      return 3;
  }

  read_info_ptr = png_create_info_struct(read_ptr);

  if (!read_info_ptr) {
      png_destroy_read_struct(&read_ptr,
          (png_infopp)NULL, (png_infopp)NULL);
      return 4;
  }


  if (setjmp(png_jmpbuf(read_ptr)))
  {
      png_destroy_read_struct(&read_ptr, &read_info_ptr,
          (png_infopp)NULL);
      fclose(pfile);
      return 5;
  }



    write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!write_ptr)
       return 6;

    write_info_ptr = png_create_info_struct(write_ptr);
    if (!write_info_ptr)
    {
       png_destroy_write_struct(&write_ptr,
           (png_infopp)NULL);
       return 7;
    }

   if (setjmp(png_jmpbuf(write_ptr)))
    {
    png_destroy_write_struct(&write_ptr, &write_info_ptr);
       fclose(pfile);
       return 8;
    }



  fseek(pfile, 0, SEEK_END);

  pngSize = ftell(pfile);

  fseek(pfile, 0, SEEK_SET);

  printf("pngSize = %u \n", pngSize);

  bufferSize = pngSize*expansion_factor;



  

  printf("bufferSize = %u \n", bufferSize);

  pngBuffer = (png_bytep)png_malloc(read_ptr, bufferSize);

  fread(pngBuffer, 1, pngSize, pfile);

  fclose (pfile);



  /////////////////////////////////////////////////// pngBuffer, pngSize, bufferSize

  for(int i=0; i<8; i++) {

    printf("signature: %X\n", pngBuffer[i]);

  }


  png_set_read_fn(read_ptr, pngBuffer, png_read_fn);

  png_set_read_status_fn(read_ptr, read_row_callback);

  png_read_png(read_ptr, read_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  info_callback(read_ptr, read_info_ptr);

  row_pointers = png_get_rows(read_ptr, read_info_ptr);


  height = (unsigned int)png_get_image_height(read_ptr, read_info_ptr);
  width = (unsigned int)png_get_image_width(read_ptr, read_info_ptr);   
  depth = (unsigned int)png_get_bit_depth(read_ptr, read_info_ptr);
  channels = (unsigned int)png_get_channels(read_ptr, read_info_ptr);
  row_bytes = (unsigned int)png_get_rowbytes(read_ptr, read_info_ptr);

  printf("Image height is %u \n", (unsigned int)png_get_image_height(read_ptr, read_info_ptr)); 
  printf("Image width is %u \n", (unsigned int)png_get_image_width(read_ptr, read_info_ptr)); 
  printf("BitsPerPixel is %u \n", depth * channels);
  printf("BytesPerPixel is %u \n", (unsigned int)png_get_channels(read_ptr, read_info_ptr));
  printf("Image stride is %u \n", (unsigned int)png_get_rowbytes(read_ptr, read_info_ptr));

  height = (unsigned int)png_get_image_height(write_ptr, write_info_ptr);
  width = (unsigned int)png_get_image_width(write_ptr, write_info_ptr);   
  depth = (unsigned int)png_get_bit_depth(write_ptr, write_info_ptr);
  channels = (unsigned int)png_get_channels(write_ptr, write_info_ptr);
  row_bytes = (unsigned int)png_get_rowbytes(write_ptr, write_info_ptr);
  printf("Image height is %u \n", (unsigned int)png_get_image_height(write_ptr, write_info_ptr)); 
  printf("Image width is %u \n", (unsigned int)png_get_image_width(write_ptr, write_info_ptr)); 
  printf("BitsPerPixel is %u \n", depth * channels);
  printf("BytesPerPixel is %u \n", (unsigned int)png_get_channels(write_ptr, write_info_ptr));
  printf("Image stride is %u \n", (unsigned int)png_get_rowbytes(write_ptr, write_info_ptr));

  printf("BEFORE encrypt/decrypt - ");

  for(int i=0; i<row_bytes; i++) {

    printf("%X", row_pointers[height/2][i]);

  }


  unsigned char outbuf[row_bytes];

  chacha20_setup(&ctx, key, sizeof(key), nonce);
  
  memset(outbuf, 0, len);
  chacha20_counter_set(&ctx, counter);

  for (size_t i = 0; i < height; i++)
  {

    chacha20_encrypt(&ctx, row_pointers[i], outbuf, row_bytes);

    memcpy(row_pointers[i], outbuf, row_bytes);

  }

  printf("\n AFTER encrypte/decrypt - ");

  for(int i=0; i<row_bytes; i++) {

    printf("%X", row_pointers[height/2][i]);

  }

  // counter = 0;  
  // memset(outbuf, 0, len);
  // chacha20_counter_set(&ctx, counter);

  // for (size_t i = 0; i < height; i++)
  // {

  //   chacha20_encrypt(&ctx, row_pointers[i], outbuf, row_bytes);

  //   memcpy(row_pointers[i], outbuf, row_bytes);

  // }

  // printf("\n AFTER encrypte/decrypt - ");

  // for(int i=0; i<row_bytes; i++) {

  //   printf("%X", row_pointers[height/2][i]);

  // }





  ret = 0;

 err:
  /* Clean up all the resources we allocated */
  if (ret != 0) {
    // ERR_print_errors_fp(stderr);
    return ret;
  }



  png_set_rows(write_ptr, write_info_ptr, row_pointers);    

  offset = 0;

  memset(pngBuffer, 0, bufferSize); 

  for(int i=0; i<8; i++) {

    printf("buffer cleared: %X\n", pngBuffer[i]);

  }

  png_set_write_fn(write_ptr, pngBuffer, png_write_fn, output_flush_fn);

  png_set_write_status_fn(png_ptr, write_row_callback);

  png_write_png(write_ptr, write_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);


  for(int i=0; i<8; i++) {

    printf("buffer written: %X\n", pngBuffer[i]);

  }


  pfile = fopen("images/sandscape.png", "wb");
  // FILE *pfile = fopen("images/hello_world_file.txt", "rb");
  if (!pfile) {
    printf("cannot open file\n");
    return 1;
  }

  pngSize = offset;

  fwrite(pngBuffer, 1, pngSize, pfile);
  if (!png_check_sig(pngBuffer, 8)) {
    printf("bad signature\n");
    return 2;   /* bad signature */
  } else {
    printf("this is a PNG file\n");
  }



  fseek(pfile, 0, SEEK_END);

  pngSize = ftell(pfile);

  fseek(pfile, 0, SEEK_SET);

  printf("finalWriteOffset = %u \n", offset);

  printf("pngSize = %u \n", pngSize);


  fclose (pfile);

  free(pngBuffer);

  png_destroy_write_struct(&read_ptr, &read_info_ptr);

  png_destroy_write_struct(&write_ptr, &write_info_ptr);








  // png_free(filebytes);


  // row_pointers = png_get_rows(png_ptr, info_ptr);

  // for( int i=0; i<height; i++) {

  //   printf("row_pointers %p \n", (void*)row_pointers[i]);

  // }

  // for( int i=0; i<row_bytes; i++) {

  //   printf("row_pointers %u \n", row_pointers[240][i]);

  // }



  // //do transform with row_pointers


  // //write transformed row pointers to png_out


  SDL_Init(SDL_INIT_VIDEO);



  // surface = createRGBSurfaceFromPNG((void *)filebytes, width, height, depth, row_bytes);
  // if(!surface) {
  //    printf("Could not create surface: %s\n", SDL_GetError());
  //    return 0;
  // }  

  image = IMG_Load("images/sandscape.png");
  
  if (!image)
  {
     printf("IMG_Load: %s\n", IMG_GetError());
     return 0;
  }

  printf("Image height is %u \n", image->h); 
  printf("Image width is %u \n", image->w); 
  printf("BitsPerPixel is %u \n", image->format->BitsPerPixel);
  printf("BytesPerPixel is %u \n", image->format->BytesPerPixel);
  printf("Image stride is %u \n", image->pitch);


  screen = SDL_SetVideoMode(image->w, image->h, 32, SDL_SWSURFACE);

  SDL_BlitSurface (image, NULL, screen, NULL);
  SDL_FreeSurface (image);



  // screen = SDL_SetVideoMode(surface->w, surface->h, depth, SDL_SWSURFACE);

  // SDL_BlitSurface(surface, NULL, screen, NULL);
  // SDL_FreeSurface(surface);

  SDL_Flip(screen);

  SDL_Quit();




  return 0;
}

