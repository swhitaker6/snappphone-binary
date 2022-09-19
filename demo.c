// Copyright 2012 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "demo.h"
#include "png.h"
#include "chacha20_simple.h"
#include <SDL.h>
#include <SDL_image.h>
#include <assert.h>
#include <emscripten.h>
#include <emscripten/html5.h>

SDL_Window *window = 0;
SDL_Renderer *renderer = 0;
SDL_Surface *image = 0;
SDL_Texture *texture = 0;

struct {
	char point[4];
} user_ptr;

uint8_t ctxKey[32];
uint8_t ctxNonce[8];
uint64_t ctxCounter = 0;

uint8_t svcKey[32];
uint8_t svcNonce[8];
uint64_t svcCounter = 0;

int num_row_bytes;
int BYTES_PER_PIXEL = 4;
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
int chunkBytes = 8192;
// int dataSize = 1024;
int bufferSize;
png_bytep pngBuffer;
int pngOutSize;
png_bytep pngOut;
png_bytep processedBytes;
int expansion_factor = 6;

const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";


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
        return 5;
    }

    
    return 0;

}




int initialize_png_writer()
{


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
       return 8;
    }
    
    
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

      // printf("read_length: %u\n", (unsigned int)read_length);

      offset += read_length;

      // printf("offset: %u\n", (unsigned int)offset);

      memcpy(data, png_data, read_length);



}





void png_write_fn(png_structp png_ptr, png_bytep data, size_t read_length) {

      png_bytep filebytes;

      png_byte* png_data;

      filebytes = (png_bytep)png_get_io_ptr(png_ptr);

      png_data = filebytes + offset;

      // printf("write_length: %u\n", (unsigned int)read_length);

      offset += read_length;

      // printf("offset: %u\n", (unsigned int)offset);

      memcpy(png_data, data, read_length);



}





void output_flush_fn(png_structp png_ptr) {

    printf("output_flushed: %X\n", 1);


}




int intN(int n) { return rand() % n; }




char *genKey(int len) {
  char *rstr = malloc((len + 1) * sizeof(char));
  int i;
  for (i = 0; i < len; i++) {
    rstr[i] = alphabet[intN(strlen(alphabet))];
  }
  rstr[len] = '\0';
  return rstr;
}





int genTexture(int page, png_bytep pngBuffer, int bufferSize, png_bytep pngOut, int pngOutSize, int width, int height, int env) {
// int genTexture(int page, uint32_t *pngBuffer, int bufferSize, uint32_t *pngOut, int pngOutSize, int width, int height, int env) {

 
    hex2byte("nijzmtp37y7dcornqo8pm363uixgdttn5v6uqi40xo3itojer7annfado12frknq", ctxKey);
   //  hex2byte("zrswda1cp9dq1oygwjo83f46332790vvauqg2wj0o7iti3oixs81x2fck6m9biiu", ctxKey);
   //  hex2byte("pvccrvd464zmmrxqpv3n5hfubc5kx0aum9ntlufqc0usepo9loxpss7a0d235v93", ctxKey);
   //  hex2byte("3iy5bbp85uw2gvwly0g7v6i8488cmtl584auxlegefpgq6cug3krlax9f7b7cdu8", ctxKey);
   //  hex2byte("x7q5v7t6aygcynttj11kuti4hmx8ehm1lut2iic6wn3udykaxbmy7tf2jzh0coa5", ctxKey);
   //  hex2byte("1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0", ctxKey);
    hex2byte("0000000000000002", ctxNonce);

    hex2byte("p2g7bsli69lre3cbf37bhca8dwpll3adhgo40kb0lhlmke01fpkvktcbv8v3zhn2", svcKey);
   //  hex2byte("1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0", svcKey);
    hex2byte("0000000000000002", svcNonce);

    const uint8_t *key = 0;
    chacha20_ctx ctx;
    chacha20_ctx svc;
    unsigned int len = 0;
    unsigned char *outfilebytes = NULL;
    int ret = 1;
    SDL_Surface *image;
    SDL_Surface *screen;
    SDL_Surface *surface;
    Uint32 rMask, gMask, bMask, aMask;
    int depth;
    int channels;
    int row_bytes;
    int do_encrypt = 1;   


    for(int i=0; i<8; i++) {

      printf("signature: %X\n", (unsigned int)pngBuffer[i]);

    }


    initialize_png_reader();

    initialize_png_writer();

    png_set_read_fn(read_ptr, pngBuffer, png_read_fn);

    png_set_read_status_fn(read_ptr, read_row_callback);

    png_set_crc_action(read_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

    png_read_png(read_ptr, read_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    info_callback(read_ptr, read_info_ptr);

    row_pointers = png_get_rows(read_ptr, read_info_ptr);

    free(pngBuffer);

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

    printf("BEFORE encrypt - ");

    for(int i=0; i<row_bytes; i++) {

      printf("%X", row_pointers[(unsigned int)floor(height/2)][i]);

    }


    unsigned char outbuf[row_bytes];

    chacha20_setup(&ctx, ctxKey, sizeof(ctxKey), ctxNonce);
    chacha20_setup(&svc, svcKey, sizeof(svcKey), svcNonce);
    
    memset(outbuf, 0, row_bytes);
    chacha20_counter_set(&ctx, ctxCounter);
    chacha20_counter_set(&svc, svcCounter);

    for (int i = 0; i < height; i++)
    {

      chacha20_encrypt(&ctx, &svc, row_pointers[i], outbuf, row_bytes, env);

      memcpy(row_pointers[i], outbuf, row_bytes);

    }

    printf("\n AFTER encrypt - ");

    for(int i=0; i<row_bytes; i++) {

      printf("%X", row_pointers[(unsigned int)floor(height/2)][i]);

    }



   //  ctxCounter = 0;  
   //  svcCounter = 0;  
   //  memset(outbuf, 0, len);
   //  chacha20_counter_set(&ctx, ctxCounter);
   //  chacha20_counter_set(&svc, svcCounter);

   //  for (size_t i = 0; i < height; i++)
   //  {

   //    chacha20_encrypt(&ctx, &svc, row_pointers[i], outbuf, row_bytes, env);

   //    memcpy(row_pointers[i], outbuf, row_bytes);

   //  }

   //  printf("\n AFTER decrypt - ");

   //  for(int i=0; i<row_bytes; i++) {

   //    printf("%X", row_pointers[(unsigned int)floor(height/2)][i]);

   //  }

    printf("\n");



    ret = 0;

  err:
    if (ret != 0) {
      return ret;
    }


    png_set_rows(write_ptr, write_info_ptr, row_pointers);    

    png_set_crc_action(write_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

    offset = 0;


    png_set_write_fn(write_ptr, pngOut, png_write_fn, output_flush_fn);
  
    png_set_write_status_fn(png_ptr, write_row_callback);

    png_write_png(write_ptr, write_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);

    png_destroy_write_struct(&write_ptr, &write_info_ptr);


  return 0;

}






uint32_t getTexture(uint32_t page, uint32_t *tex) {

  return 0;

}  







static void render() {

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);  
  SDL_RenderPresent(renderer);  

}

static void mainloop() {

  render();

}



int main() {


    int env = 1;
    int height = 797;
    int width = 1200;   
    char filename[] = "images/sand.png";
    int page = 0;
    

    FILE *pfile = fopen(filename, "rb");
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



    fseek(pfile, 0, SEEK_END);

    pngSize = ftell(pfile);

    fseek(pfile, 0, SEEK_SET);

    printf("pngSize = %u \n", pngSize);

    bufferSize = pngSize;  

    

    printf("bufferSize = %u \n", bufferSize);

    pngBuffer = (png_bytep)malloc(bufferSize);  

    fread(pngBuffer, 1, pngSize, pfile);

    fclose (pfile);



    pngOutSize = height*width*BYTES_PER_PIXEL+chunkBytes;

    printf("pngOutSize = %u \n", pngOutSize);

    pngOut = (png_bytep)malloc(pngOutSize);



    genTexture(page, pngBuffer, pngSize, pngOut, pngOutSize, width, height, env);



    for(int i=0; i<8; i++) {

      printf("buffer written: %X\n", pngOut[i]);

    }


    pfile = fopen(filename, "wb");
    if (!pfile) {
      printf("cannot open file\n");
      return 1;
    }


    pngSize = offset;

    unsigned int result = fwrite(pngOut, 1, pngOutSize, pfile);
    if (!png_check_sig(pngOut, 8)) {
      printf("bad signature\n");
      return 2;   /* bad signature */
    } else {
      printf("this is a PNG file\n");
    }

    printf("NUMBER OF BYTES WRITTEN = %u \n", result);


    fseek(pfile, 0, SEEK_END);

    pngSize = ftell(pfile);

    fseek(pfile, 0, SEEK_SET);

    printf("finalWriteOffset = %u \n", offset);

    printf("pngSize = %u \n", pngSize);


    fclose (pfile);

    free(pngOut);



   //  SDL_Init(SDL_INIT_VIDEO);


   //  image = IMG_Load(filename);




   //  if (!image)
   //  {
   //    printf("IMG_Load: %s\n", IMG_GetError());
   //    return 0;
   //  }
   //  assert(image->format->BitsPerPixel == 32);
   //  assert(image->format->BytesPerPixel == 4);
   //  assert(image->pitch == 4*image->w);
   //  printf("Image height is %u \n", image->h); 
   //  printf("Image width is %u \n", image->w); 
   //  printf("BitsPerPixel is %u \n", image->format->BitsPerPixel);
   //  printf("BytesPerPixel is %u \n", image->format->BytesPerPixel);
   //  printf("Image stride is %u \n", image->pitch);
    
   //  SDL_FreeSurface (image);

   //  window = SDL_CreateWindow(NULL, 0, 0, image->w, image->h, SDL_WINDOW_SHOWN);
   //  renderer = SDL_CreateRenderer(window, -1, 0);
   //  texture = IMG_LoadTexture(renderer, filename);

   //  emscripten_set_main_loop(mainloop, 0, 0);




    return 0;

}

