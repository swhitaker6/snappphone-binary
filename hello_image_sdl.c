/*
 * Copyright 2013 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <SDL.h>
#include <SDL_image.h>
// #include <SDL/SDL.h>
// #include <SDL/SDL_image.h>
#include <assert.h>
#include <emscripten.h>
// #include <emscripten/html5.h>
// #include <unistd.h>


SDL_Window *window = 0;
SDL_Renderer *renderer = 0;
SDL_Surface *image = 0;
SDL_Texture *texture = 0;



static void render() {

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);  
  SDL_RenderPresent(renderer);  

}

static void mainloop() {

  render();

}

int showImage(const char* fileName) {

  SDL_Init(SDL_INIT_VIDEO);

  char filename[] = "images/sand.png";

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
  
  SDL_FreeSurface (image);

  window = SDL_CreateWindow(NULL, 0, 0, image->w, image->h, SDL_WINDOW_SHOWN);
  renderer = SDL_CreateRenderer(window, -1, 0);
  texture = IMG_LoadTexture(renderer, filename);

  return 1;
}

int main() {

  int result;

  result = showImage("images/sand.png"); // absolute path

  assert(result != 0);

  printf("you should see an image.\n");

  emscripten_set_main_loop(mainloop, 0, 0);
  return 0;
}

