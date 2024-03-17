/*
 * A simple libpng example program
 * http://zarb.org/~gc/html/libpng.html
 *
 * Modified by Yoshimasa Niwa to make it much simpler
 * and support all defined color_type.
 *
 * To build, use the next instruction on OS X.
 * $ brew install libpng
 * $ clang -lz -lpng16 libpng_test.c
 *
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <png.h>
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

//    SDL_Surface *image;
SDL_Surface *screen;
SDL_Surface *surface;
Uint32 rMask, gMask, bMask, aMask;
int depth;
int channels;
int row_bytes;
int do_encrypt = 1;   


int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers = NULL;


static FILE *fp;
static FILE *mem_fp;
png_structp png;
png_infop info;

char* pngReadBuffer;
char* pngWriteBuffer;
uint fileSize;
size_t fileOutSize;


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





void read_png_file(char* filename) {


    int pos;
    int size;



    FILE *fp = fopen(filename, "rb");

    pos = ftell(fp); // store the cursor position


    // go to the end of the file and get the cursor position
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);
    // size = ftell(fp);

    // go back to the old position
    fseek(fp, pos, SEEK_SET);


    // FILE *f = fopen("your_file", "r");
    // size_t size = get_file_size(f);
    pngReadBuffer = malloc(fileSize);
    // pngReadBuffer = malloc(size);

    if (fread(pngReadBuffer, 1, fileSize, fp) != fileSize) { // bytes read != computed file size
        printf("error converting file to buffer");
    }

    // use your buffer...
    for(int i=0; i<2000; i++) {

      // printf("%s", slnkBuffer[(unsigned int)i]);
      printf("%X", pngReadBuffer[(unsigned int)i]);

    }
      printf("\n");



    // don't forget to free and fclose
    // free(buffer);
    fclose(fp);



}










void read_png_buffer() {

    printf("Inside read_png_file func\n");

  printf("about to call fmemopen with strlen = %u\n", fileSize);
  mem_fp = fmemopen(pngReadBuffer, fileSize + 1, "rb");
  // fp = fopen(filename, "rb");


  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
  // png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
  if(!png) abort();

  info = png_create_info_struct(png);
  // png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  printf("about to call png_init_io with strlen = %u\n", fileSize);
  
  png_init_io(png, mem_fp);


}







void BACKUP__read_png_file(char* filename) {

    printf("Inside read_png_file func\n");


  fp = fopen(filename, "rb");

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
  // png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); 
  if(!png) abort();

  info = png_create_info_struct(png);
  // png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);


}







void read_png_info() {

    printf("Inside read_png_in func\n");


//   png_set_crc_action(read_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

printf("about to call png_read_info\n ");
  
  png_read_info(png, info);

printf("AFTER call to png_read_info \n");

  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);

  channels = (unsigned int)png_get_channels(png, info);
  row_bytes = (unsigned int)png_get_rowbytes(png, info);


    printf("Image height is %u \n", (unsigned int)png_get_image_height(png, info)); 
    printf("Image width is %u \n", (unsigned int)png_get_image_width(png, info)); 
    printf("BitsPerPixel is %u \n", depth * channels);
    printf("BytesPerPixel is %u \n", (unsigned int)png_get_channels(png, info));
    printf("Image stride is %u \n", (unsigned int)png_get_rowbytes(png, info));



  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);


}





void read_png_image() {

  if (row_pointers) abort();

  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);

  // fclose(fp);

  png_destroy_read_struct(&png, &info, NULL);


}







void close_png_file() {

  fclose(mem_fp);


}











void write_png_buffer() {
  int y;

  printf("about to call open_memstream \n");
  FILE *fp = open_memstream(&pngWriteBuffer, &fileOutSize);
  // FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);

  if (!row_pointers) abort();

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);
  printf("AFTER call to open_memstream is closed with fileOutSize = %zu\n", fileOutSize);

  png_destroy_write_struct(&png, &info);
}






void write_png_file(char* filename) {

    //file pointer
    FILE *fp = NULL;

    //create the file
    fp = fopen(filename, "wb");
    if(fp == NULL)
    {
        printf("Error in creating the file\n");
        exit(1);
    }
    //Write the buffer in file
    fwrite(pngWriteBuffer, sizeof(pngWriteBuffer[0]), fileOutSize, fp);
    //close the file
    fclose(fp);
    printf("File has been created successfully\n");


}








void BACKUP__write_png_file(char* filename) {
  int y;

  FILE *fp = fopen(filename, "wb");
  if(!fp) abort();

  png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) abort();

  png_infop info = png_create_info_struct(png);
  if (!info) abort();

  if (setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

  // Output is 8bit depth, RGBA format.
  png_set_IHDR(
    png,
    info,
    width, height,
    8,
    PNG_COLOR_TYPE_RGBA,
    PNG_INTERLACE_NONE,
    PNG_COMPRESSION_TYPE_DEFAULT,
    PNG_FILTER_TYPE_DEFAULT
  );
  png_write_info(png, info);

  // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
  // Use png_set_filler().
  //png_set_filler(png, 0, PNG_FILLER_AFTER);

  if (!row_pointers) abort();

  png_write_image(png, row_pointers);
  png_write_end(png, NULL);

  for(int y = 0; y < height; y++) {
    free(row_pointers[y]);
  }
  free(row_pointers);

  fclose(fp);

  png_destroy_write_struct(&png, &info);
}















void hex2byte(const char *hex, uint8_t *byte)
{
  
  while (*hex) { 
    
    sscanf(hex, "%2hhx", byte++); 
    hex += 2; 
    
  }

}





void process_png_file(uint page, uint env) {


    printf("Inside process_png_file func\n");

    hex2byte("nijzmtp37y7dcornqo8pm363uixgdttn5v6uqi40xo3itojer7annfado12frknq", ctxKey);
    // hex2byte("zrswda1cp9dq1oygwjo83f46332790vvauqg2wj0o7iti3oixs81x2fck6m9biiu", ctxKey);
    // hex2byte("pvccrvd464zmmrxqpv3n5hfubc5kx0aum9ntlufqc0usepo9loxpss7a0d235v93", ctxKey);
    // hex2byte("3iy5bbp85uw2gvwly0g7v6i8488cmtl584auxlegefpgq6cug3krlax9f7b7cdu8", ctxKey);
    // hex2byte("x7q5v7t6aygcynttj11kuti4hmx8ehm1lut2iic6wn3udykaxbmy7tf2jzh0coa5", ctxKey);
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
 


    printf("BEFORE encrypt - ");

    for(int i=0; i<row_bytes; i++) {

      printf("%X", row_pointers[(unsigned int)floor(height/2)][i]);

    }


    unsigned char outbuf[row_bytes];

    if(env == 1 || env == 6 || env == 8)
      chacha20_setup(&ctx, ctxKey, sizeof(ctxKey), ctxNonce);
    else
      chacha20_setup(&ctx, svcKey, sizeof(svcKey), svcNonce);
  
    memset(outbuf, 0, row_bytes);
    chacha20_counter_set(&ctx, ctxCounter);

    for (size_t i = 0; i < height; i++)
    {

      chacha20_encrypt(&ctx, &svc, row_pointers[i], outbuf, row_bytes, env);

      memcpy(row_pointers[i], outbuf, row_bytes);

    }

    printf("\n AFTER encrypt - ");

    for(int i=0; i<row_bytes; i++) {

      printf("%X", row_pointers[(unsigned int)floor(height/2)][i]);

    }



    ctxCounter = 0;  
    svcCounter = 0;  
    memset(outbuf, 0, len);
    chacha20_counter_set(&ctx, ctxCounter);
    chacha20_counter_set(&svc, svcCounter);

    for (size_t i = 0; i < height; i++)
    {

      chacha20_encrypt(&ctx, &svc, row_pointers[i], outbuf, row_bytes, env);

      memcpy(row_pointers[i], outbuf, row_bytes);

    }

    printf("\n AFTER decrypt - ");

    for(int i=0; i<row_bytes; i++) {

      printf("%X", row_pointers[(unsigned int)floor(height/2)][i]);

    }

    printf("\n");




}







// void process_png_file() {
//   for(int y = 0; y < height; y++) {
//     png_bytep row = row_pointers[y];
//     for(int x = 0; x < width; x++) {
//       png_bytep px = &(row[x * 4]);
//       // Do something awesome for each pixel here...
//     //   printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
//     }
//   }
// }








static void render() {

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);  
  SDL_RenderPresent(renderer);  

}



static void mainloop() {

  render();

}


uint genTexture(uint page, char* pngBuffer, uint pngSize, char* pngOut, size_t pngOutSize, uint width, uint height, uint env) {

  
    if(pngBuffer) {

      pngReadBuffer = pngBuffer;
      pngWriteBuffer = pngOut;
      fileSize = pngSize;
      fileOutSize = pngOutSize;

    }

    read_png_buffer();
    read_png_info();
    read_png_image();
    close_png_file();
    process_png_file(page, env);
    write_png_buffer();
 
  
  
  return 0;

}  




uint32_t getTexture(uint32_t page, uint32_t *tex) {

  return 0;

}  







int main(int argc, char *argv[]) {
//   if(argc != 3) abort();



    int env = 7; //{svckey: 5}, {svc: 0 || 7}, {spcKey: 6}, {spc: 1 || 8}, { see: 2, share: 3, view: 4}
    // int height = 0;
    // int width = 0;   
    char filename[] = "images/motorcycle.png";
    // char filename[] = "images/stride.png";
    int page = 0;



    read_png_file(filename);
    
    genTexture(0, 0, 0, 0, 0, 0, 0, env);
 
    write_png_file(filename);



    printf("About to initialize SDL\n");
    SDL_Init(SDL_INIT_VIDEO);

    printf("About to load png file just written\n");

    image = IMG_Load(filename);

    if (!image)
    {
      printf("IMG_Load: %s\n", IMG_GetError());
      return 0;
    }
    assert(image->format->BitsPerPixel == 32);
    assert(image->format->BytesPerPixel == 4);
    assert(image->pitch == 4*image->w);
    printf("Image height is %u \n", image->h); 
    printf("Image width is %u \n", image->w); 
    printf("BitsPerPixel is %u \n", image->format->BitsPerPixel);
    printf("BytesPerPixel is %u \n", image->format->BytesPerPixel);
    printf("Image stride is %u \n", image->pitch);

    printf("env = %u \n", env);

    
    SDL_FreeSurface (image);

    window = SDL_CreateWindow(NULL, 0, 0, image->w, image->h, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = IMG_LoadTexture(renderer, filename);

    emscripten_set_main_loop(mainloop, 0, 0);



  return 0;
}