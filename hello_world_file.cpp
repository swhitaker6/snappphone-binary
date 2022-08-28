// Copyright 2012 The Emscripten Authors.  All rights reserved.
// Emscripten is available under two separate licenses, the MIT license and the
// University of Illinois/NCSA Open Source License.  Both these licenses can be
// found in the LICENSE file.

#include <stdio.h>
#include "png.h"

int main() {
  
  FILE *pfile = fopen("test/hello_world_file.txt", "rb");
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
    printf('this is a PNG file');
  }

  // while (!feof(pfile)) {
  //   char c = fgetc(pfile);
  //   if (c != EOF) {
  //     putchar(c);
  //   }
  // }

  fclose (pfile);
  return 0;
}

