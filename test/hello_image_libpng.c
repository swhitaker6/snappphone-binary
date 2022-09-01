/*
 * Copyright 2013 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <assert.h>
#include <emscripten.h>
#include <unistd.h>

int showImage(const char* fileName) {

  SDL_Surface *image;
  SDL_Surface *screen;

  SDL_Init(SDL_INIT_VIDEO);

  image = IMG_Load(fileName);
  
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


  screen = SDL_SetVideoMode(image->w, image->h, 32, SDL_SWSURFACE);

  SDL_BlitSurface (image, NULL, screen, NULL);
  SDL_FreeSurface (image);

  SDL_Flip(screen);

  SDL_Quit();

  return 1;
}

int main() {

  int result;

  // SDL_Init(SDL_INIT_VIDEO);
  // SDL_Surface *screen = SDL_SetVideoMode(600, 450, 32, SDL_SWSURFACE);


  result = showImage("test/flower.png"); // absolute path
  // result |= testImage(screen, "test/screenshot.png"); // absolute path
  assert(result != 0);

  // SDL_Flip(screen);

  printf("you should see an image.\n");

  // SDL_Quit();

  return 0;
}

