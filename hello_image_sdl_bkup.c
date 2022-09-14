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

  // image = IMG_Load(fileName);
  SDL_RWops *rw = SDL_RWFromFile(fileName, "rb");
  // SDL_RWops *rw = SDL_RWFromMem(pngBuffer, pngSize);

  SDL_Surface *temp = IMG_LoadPNG_RW(rw);

  assert(temp->format->BitsPerPixel == 32);
  assert(temp->format->BytesPerPixel == 4);
  assert(temp->pitch == 4*temp->w);
  printf("Image height is %u \n", temp->h); 
  printf("Image width is %u \n", temp->w); 
  printf("BitsPerPixel is %u \n", temp->format->BitsPerPixel);
  printf("BytesPerPixel is %u \n", temp->format->BytesPerPixel);
  printf("Image stride is %u \n", temp->pitch);


  if (temp == NULL) { 
   printf("Unable to load png: %s\n", SDL_GetError()); exit(1);
  }

    SDL_Window *window;
//   SDL_Surface *image; 

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "An SDL2 window",                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        temp->w,                               // width, in pixels
        temp->h,                               // height, in pixels
        SDL_WINDOW_BORDERLESS                  // flags - see below
    );
  
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }



  image = SDL_ConvertSurfaceFormat(temp, SDL_GetWindowPixelFormat(window), 0);
  // image = SDL_DisplayFormat(temp);
  // image = SDL_DisplayFormatAlpha(temp);  

  SDL_FreeSurface(temp);
  // SDL_DestroyWindow(window);

//   image = IMG_Load("images/sand.png");
  
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

