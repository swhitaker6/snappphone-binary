
/* pngtest.c - a simple test program to test libpng
 *
 * Last changed in libpng 1.6.17 [March 26, 2015]
 * Copyright (c) 1998-2015 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 *
 * This program reads in a PNG image, writes it out again, and then
 * compares the two files.  If the files are identical, this shows that
 * the basic chunk handling, filtering, and (de)compression code is working
 * properly.  It does not currently test all of the transforms, although
 * it probably should.
 *
 * The program will report "FAIL" in certain legitimate cases:
 * 1) when the compression level or filter selection method is changed.
 * 2) when the maximum IDAT size (PNG_ZBUF_SIZE in pngconf.h) is not 8192.
 * 3) unknown unsafe-to-copy ancillary chunks or unknown critical chunks
 *    exist in the input file.
 * 4) others not listed here...
 * In these cases, it is best to check with another tool such as "pngcheck"
 * to see what the differences between the two files are.
 *
 * If a filename is given on the command-line, then this file is used
 * for the input, rather than the default "pngtest.png".  This allows
 * testing a wide variety of files easily.  You can also test a number
 * of files at once by typing "pngtest -m file1.png file2.png ..."
 */

#define _POSIX_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Defined so I can write to a file on gui/windowing platforms */
/*  #define STDERR stderr  */
#define STDERR stdout   /* For DOS */

#include "png.h"
#include "zlib.h"


#define PNG_tIME_STRING_LENGTH 29
static int tIME_chunk_present = 0;
static char tIME_string[PNG_tIME_STRING_LENGTH] = "tIME chunk is not present";


static int verbose = 0;
static int strict = 0;
static int relaxed = 0;
static int unsupported_chunks = 0; /* chunk unsupported by libpng in input */
static int error_count = 0; /* count calls to png_error */
static int warning_count = 0; /* count calls to png_warning */


#define png_jmpbuf(png_ptr) png_ptr->jmpbuf




static void PNGCBAPI
pngtest_read_data(png_structp png_ptr, png_bytep data, png_size_t length) 
{
   png_size_t check = 0;
   png_voidp io_ptr;

   /* fread() returns 0 on error, so it is OK to store this in a png_size_t
    * instead of an int, which is what fread() actually returns.
    */
   io_ptr = png_get_io_ptr(png_ptr);
   if (io_ptr != NULL)
   {
      check = fread(data, 1, length, (png_FILE_p)io_ptr);
   }

   if (check != length)
   {
      png_error(png_ptr, "Read Error");
   }


}


static void PNGCBAPI
pngtest_flush(png_structp png_ptr)//////{15}
{
   /* Do nothing; fflush() is said to be just a waste of energy. */
   PNG_UNUSED(png_ptr)   /* Stifle compiler warning */
}


/* This is the function that does the actual writing of data.  If you are
 * not writing to a standard C stream, you should create a replacement
 * write_data function and use it at run time with png_set_write_fn(), rather
 * than changing the library.
 */
static void PNGCBAPI
pngtest_write_data(png_structp png_ptr, png_bytep data, png_size_t length) //////{14}
{
   png_size_t check;

   check = fwrite(data, 1, length, (png_FILE_p)png_get_io_ptr(png_ptr));

   if (check != length)
   {
      png_error(png_ptr, "Write Error");
   }


}





/* Utility to save typing/errors, the argument must be a name */
#define MEMZERO(var) ((void)memset(&var, 0, sizeof var))

/* Example of using row callbacks to make a simple progress meter */
static int status_pass = 1;
static int status_dots_requested = 0;
static int status_dots = 1;

static void PNGCBAPI
read_row_callback(png_structp png_ptr, png_uint_32 row_number, int pass)
{
   if (png_ptr == NULL || row_number > PNG_UINT_31_MAX)
      return;

   if (status_pass != pass)
   {
      fprintf(stdout, "\n Pass %d: ", pass);
      status_pass = pass;
      status_dots = 31;
   }

   status_dots--;

   if (status_dots == 0)
   {
      fprintf(stdout, "\n         ");
      status_dots=30;
   }

   fprintf(stdout, "r");
}


static void PNGCBAPI
write_row_callback(png_structp png_ptr, png_uint_32 row_number, int pass)
{
   if (png_ptr == NULL || row_number > PNG_UINT_31_MAX || pass > 7)
      return;

   fprintf(stdout, "w");
}









/* This function is called when there is a warning, but the library thinks
 * it can continue anyway.  Replacement functions don't have to do anything
 * here if you don't want to.  In the default configuration, png_ptr is
 * not used, but it is passed in case it may be useful.
 */
typedef struct
{
   PNG_CONST char *file_name;
}  pngtest_error_parameters;

static void PNGCBAPI
pngtest_warning(png_structp png_ptr, png_const_charp message)
{
   PNG_CONST char *name = "UNKNOWN (ERROR!)";
   pngtest_error_parameters *test =
      (pngtest_error_parameters*)png_get_error_ptr(png_ptr);

   ++warning_count;

   if (test != NULL && test->file_name != NULL)
      name = test->file_name;

   fprintf(STDERR, "%s: libpng warning: %s\n", name, message);
}

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in png_set_error_fn().
 */
static void PNGCBAPI
pngtest_error(png_structp png_ptr, png_const_charp message)
{
   ++error_count;

   pngtest_warning(png_ptr, message);
   /* We can return because png_error calls the default handler, which is
    * actually OK in this case.
    */
}






/* Test one file */
static int
genTexture(int page, uint32_t *png, int sz, int w, int h, bool env)
{
   static uint32_t fpin; //png_FILE_p fpin;
   static uint32_t fpout; //png_FILE_p fpout;  /* "static" prevents setjmp corruption */
   pngtest_error_parameters error_parameters;
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

   row_buf = NULL;
//    error_parameters.file_name = inname;

   fpin = png; //fopen(inname, "rb");
 
   fpout = png; //fopen(outname, "wb");

   read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); //////{1}

   png_set_error_fn(read_ptr, &error_parameters, pngtest_error,
      pngtest_warning);

   write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); //////{10}

   png_set_error_fn(write_ptr, &error_parameters, pngtest_error,
      pngtest_warning);

   read_info_ptr = png_create_info_struct(read_ptr); //////{2}
   end_info_ptr = png_create_info_struct(read_ptr);
   write_info_ptr = png_create_info_struct(write_ptr); //////{11}
   write_end_info_ptr = png_create_info_struct(write_ptr);


   pngtest_debug("Setting jmpbuf for read struct");
   if (setjmp(png_jmpbuf(read_ptr))) //////{3}
   {
      fprintf(STDERR, "%s -> %s: libpng read error\n", inname, outname);
      png_free(read_ptr, row_buf);
      row_buf = NULL;

      png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr); ///////{3}
      png_destroy_info_struct(write_ptr, &write_end_info_ptr);
      png_destroy_write_struct(&write_ptr, &write_info_ptr);

      // FCLOSE(fpin);
      // FCLOSE(fpout);
      return (1);
   }

   pngtest_debug("Setting jmpbuf for write struct");

   if (setjmp(png_jmpbuf(write_ptr))) //////{12}
   {
      fprintf(STDERR, "%s -> %s: libpng write error\n", inname, outname);
      png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);
      png_destroy_info_struct(write_ptr, &write_end_info_ptr);
      png_destroy_write_struct(&write_ptr, &write_info_ptr);
      // FCLOSE(fpin);
      // FCLOSE(fpout);
      return (1);
   }


   pngtest_debug("Initializing input and output streams");

   png_set_read_fn(read_ptr, (png_voidp)fpin, pngtest_read_data); //////{4,5,6,7} [ png_set_read_progressive_fn(read_ptr, $user, info_callback, row_callback, end_callback) ]
   png_set_write_fn(write_ptr, (png_voidp)fpout,  pngtest_write_data, pngtest_flush); //////{13,14,15}
//    png_set_write_fn(write_ptr, (png_voidp)fpout,  pngtest_write_data, NULL);


   pngtest_debug("Reading info struct");
   png_read_info(read_ptr, read_info_ptr);


   pngtest_debug("Transferring info struct");
   {
      int interlace_type, compression_type, filter_type;

      if (png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
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
      }
   }
   pngtest_debug("Transferring PNG_FIXED_POINT");
   {
      pngtest_debug("Transferring PNG_cHRM");
      {
            png_fixed_point white_x, white_y, red_x, red_y, green_x, green_y, blue_x,
            blue_y;

            if (png_get_cHRM_fixed(read_ptr, read_info_ptr, &white_x, &white_y,
            &red_x, &red_y, &green_x, &green_y, &blue_x, &blue_y) != 0)
            {
            png_set_cHRM_fixed(write_ptr, write_info_ptr, white_x, white_y, red_x,
                  red_y, green_x, green_y, blue_x, blue_y);
            }
      }

      pngtest_debug("Transferring PNG_gAMA");
      {
            png_fixed_point gamma;

            if (png_get_gAMA_fixed(read_ptr, read_info_ptr, &gamma) != 0)
            png_set_gAMA_fixed(write_ptr, write_info_ptr, gamma);
      }
   }
   pngtest_debug("Transferring PNG_FLOATING_POINT");
   {
      pngtest_debug("Transferring PNG_cHRM_SUPPORTED");
      {
            double white_x, white_y, red_x, red_y, green_x, green_y, blue_x,
            blue_y;

            if (png_get_cHRM(read_ptr, read_info_ptr, &white_x, &white_y, &red_x,
            &red_y, &green_x, &green_y, &blue_x, &blue_y) != 0)
            {
            png_set_cHRM(write_ptr, write_info_ptr, white_x, white_y, red_x,
                  red_y, green_x, green_y, blue_x, blue_y);
            }
      }

      pngtest_debug("Transferring PNG_gAMA_SUPPORTED");
      {
            double gamma;

            if (png_get_gAMA(read_ptr, read_info_ptr, &gamma) != 0)
            png_set_gAMA(write_ptr, write_info_ptr, gamma);
      }
   }
   // PNG_iCCP_SUPPORTED
   {
      png_charp name;
      png_bytep profile;
      png_uint_32 proflen;
      int compression_type;

      if (png_get_iCCP(read_ptr, read_info_ptr, &name, &compression_type,
                      &profile, &proflen) != 0)
      {
         png_set_iCCP(write_ptr, write_info_ptr, name, compression_type,
                      profile, proflen);
      }
   }
   // PNG_sRGB_SUPPORTED
   {
      int intent;

      if (png_get_sRGB(read_ptr, read_info_ptr, &intent) != 0)
         png_set_sRGB(write_ptr, write_info_ptr, intent);
   }

   {
      png_colorp palette;
      int num_palette;

      if (png_get_PLTE(read_ptr, read_info_ptr, &palette, &num_palette) != 0)
         png_set_PLTE(write_ptr, write_info_ptr, palette, num_palette);
   }
   // PNG_bKGD_SUPPORTED
   {
      png_color_16p background;

      if (png_get_bKGD(read_ptr, read_info_ptr, &background) != 0)
      {
         png_set_bKGD(write_ptr, write_info_ptr, background);
      }
   }
   // PNG_hIST_SUPPORTED
   {
      png_uint_16p hist;

      if (png_get_hIST(read_ptr, read_info_ptr, &hist) != 0)
         png_set_hIST(write_ptr, write_info_ptr, hist);
   }
  // PNG_oFFs_SUPPORTED
   {
      png_int_32 offset_x, offset_y;
      int unit_type;

      if (png_get_oFFs(read_ptr, read_info_ptr, &offset_x, &offset_y,
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

      if (png_get_pCAL(read_ptr, read_info_ptr, &purpose, &X0, &X1, &type,
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

      if (png_get_pHYs(read_ptr, read_info_ptr, &res_x, &res_y,
          &unit_type) != 0)
         png_set_pHYs(write_ptr, write_info_ptr, res_x, res_y, unit_type);
   }

   // PNG_sBIT_SUPPORTED
   {
      png_color_8p sig_bit;

      if (png_get_sBIT(read_ptr, read_info_ptr, &sig_bit) != 0)
         png_set_sBIT(write_ptr, write_info_ptr, sig_bit);
   }

   // PNG_sCAL_SUPPORTED
   //PNG_FLOATING_POINT_SUPPORTED
   // PNG_FLOATING_ARITHMETIC_SUPPORTED
   {
      int unit;
      double scal_width, scal_height;

      if (png_get_sCAL(read_ptr, read_info_ptr, &unit, &scal_width,
         &scal_height) != 0)
      {
         png_set_sCAL(write_ptr, write_info_ptr, unit, scal_width, scal_height);
      }
   }

   // PNG_FIXED_POINT_SUPPORTED
   {
      int unit;
      png_charp scal_width, scal_height;

      if (png_get_sCAL_s(read_ptr, read_info_ptr, &unit, &scal_width,
          &scal_height) != 0)
      {
         png_set_sCAL_s(write_ptr, write_info_ptr, unit, scal_width,
             scal_height);
      }
   }
   
   // PNG_TEXT_SUPPORTED
   {
      png_textp text_ptr;
      int num_text;

      if (png_get_text(read_ptr, read_info_ptr, &text_ptr, &num_text) > 0)
      {
         pngtest_debug1("Handling %d iTXt/tEXt/zTXt chunks", num_text);

         pngtest_check_text_support(read_ptr, text_ptr, num_text);

         if (verbose != 0)
         {
            int i;

            printf("\n");
            for (i=0; i<num_text; i++)
            {
               printf("   Text compression[%d]=%d\n",
                     i, text_ptr[i].compression);
            }
         }

         png_set_text(write_ptr, write_info_ptr, text_ptr, num_text);
      }
   }

   // PNG_tIME_SUPPORTED
   {
      png_timep mod_time;

      if (png_get_tIME(read_ptr, read_info_ptr, &mod_time) != 0)
      {
         png_set_tIME(write_ptr, write_info_ptr, mod_time);

         if (png_convert_to_rfc1123_buffer(tIME_string, mod_time) != 0)
            tIME_string[(sizeof tIME_string) - 1] = '\0';

         else
         {
            strncpy(tIME_string, "*** invalid time ***", (sizeof tIME_string));
            tIME_string[(sizeof tIME_string) - 1] = '\0';
         }

         tIME_chunk_present++;

      }
   }

   // PNG_tRNS_SUPPORTED
   {
      png_bytep trans_alpha;
      int num_trans;
      png_color_16p trans_color;

      if (png_get_tRNS(read_ptr, read_info_ptr, &trans_alpha, &num_trans,
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

   // PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   {
      png_unknown_chunkp unknowns;
      int num_unknowns = png_get_unknown_chunks(read_ptr, read_info_ptr,
         &unknowns);

      if (num_unknowns != 0)
      {
         png_set_unknown_chunks(write_ptr, write_info_ptr, unknowns,
           num_unknowns);
#if PNG_LIBPNG_VER < 10600
         /* Copy the locations from the read_info_ptr.  The automatically
          * generated locations in write_end_info_ptr are wrong prior to 1.6.0
          * because they are reset from the write pointer (removed in 1.6.0).
          */
         {
            int i;
            for (i = 0; i < num_unknowns; i++)
              png_set_unknown_chunk_location(write_ptr, write_info_ptr, i,
                unknowns[i].location);
         }
#endif
      }
   }

   // PNG_WRITE_SUPPORTED
   pngtest_debug("Writing info struct");

   /* Write the info in two steps so that if we write the 'unknown' chunks here
    * they go to the correct place.
    */
   png_write_info_before_PLTE(write_ptr, write_info_ptr);

   png_write_info(write_ptr, write_info_ptr);



   pngtest_debug("Allocating row buffer...");
   row_buf = (png_bytep)png_malloc(read_ptr,
      png_get_rowbytes(read_ptr, read_info_ptr));

//    pngtest_debug1("\t0x%08lx", (unsigned long)row_buf);

   pngtest_debug("Writing row data");


   num_pass = png_set_interlace_handling(read_ptr);
   if (png_set_interlace_handling(write_ptr) != num_pass)
      png_error(write_ptr, "png_set_interlace_handling: inconsistent num_pass");



   for (pass = 0; pass < num_pass; pass++)
   {
      pngtest_debug1("Writing row data for pass %d", pass);
      for (y = 0; y < height; y++)
      {

         png_read_rows(read_ptr, (png_bytepp)&row_buf, NULL, 1);


         png_write_rows(write_ptr, (png_bytepp)&row_buf, 1);

      }
   }



   pngtest_debug("Reading and writing end_info data");

   png_read_end(read_ptr, end_info_ptr);

   png_write_end(write_ptr, write_end_info_ptr);

#ifdef PNG_EASY_ACCESS_SUPPORTED
   if (verbose != 0)
   {
      png_uint_32 iwidth, iheight;
      iwidth = png_get_image_width(write_ptr, write_info_ptr);
      iheight = png_get_image_height(write_ptr, write_info_ptr);
      fprintf(STDERR, "\n Image width = %lu, height = %lu\n",
         (unsigned long)iwidth, (unsigned long)iheight);
   }
#endif

   pngtest_debug("Destroying data structs");

   pngtest_debug("destroying row_buf for read_ptr");
   png_free(read_ptr, row_buf);
   row_buf = NULL;

   pngtest_debug("destroying read_ptr, read_info_ptr, end_info_ptr");
   png_destroy_read_struct(&read_ptr, &read_info_ptr, &end_info_ptr);

   pngtest_debug("destroying write_end_info_ptr");
   png_destroy_info_struct(write_ptr, &write_end_info_ptr);
   pngtest_debug("destroying write_ptr, write_info_ptr");
   png_destroy_write_struct(&write_ptr, &write_info_ptr);

   pngtest_debug("Destruction complete.");

//    FCLOSE(fpin);
//    FCLOSE(fpout);

   /* Summarize any warnings or errors and in 'strict' mode fail the test.
    * Unsupported chunks can result in warnings, in that case ignore the strict
    * setting, otherwise fail the test on warnings as well as errors.
    */
   if (error_count > 0)
   {
      /* We don't really expect to get here because of the setjmp handling
       * above, but this is safe.
       */
      fprintf(STDERR, "\n  %s: %d libpng errors found (%d warnings)",
         inname, error_count, warning_count);

      if (strict != 0)
         return (1);
   }

#  ifdef PNG_WRITE_SUPPORTED
      /* If there we no write support nothing was written! */
      else if (unsupported_chunks > 0)
      {
         fprintf(STDERR, "\n  %s: unsupported chunks (%d)%s",
            inname, unsupported_chunks, strict ? ": IGNORED --strict!" : "");
      }
#  endif

   else if (warning_count > 0)
   {
      fprintf(STDERR, "\n  %s: %d libpng warnings found",
         inname, warning_count);

      if (strict != 0)
         return (1);
   }

   pngtest_debug("Opening files for comparison");
   if ((fpin = fopen(inname, "rb")) == NULL)
   {
      fprintf(STDERR, "Could not find file %s\n", inname);
      return (1);
   }

   if ((fpout = fopen(outname, "rb")) == NULL)
   {
      fprintf(STDERR, "Could not find file %s\n", outname);
      FCLOSE(fpin);
      return (1);
   }

#ifdef PNG_WRITE_SUPPORTED /* else nothing was written */
   if (interlace_preserved != 0) /* else the files will be changed */
   {
      for (;;)
      {
         static int wrote_question = 0;
         png_size_t num_in, num_out;
         char inbuf[256], outbuf[256];

         num_in = fread(inbuf, 1, sizeof inbuf, fpin);
         num_out = fread(outbuf, 1, sizeof outbuf, fpout);

         if (num_in != num_out)
         {
            fprintf(STDERR, "\nFiles %s and %s are of a different size\n",
                    inname, outname);

            if (wrote_question == 0 && unsupported_chunks == 0)
            {
               fprintf(STDERR,
         "   Was %s written with the same maximum IDAT chunk size (%d bytes),",
                 inname, PNG_ZBUF_SIZE);
               fprintf(STDERR,
                 "\n   filtering heuristic (libpng default), compression");
               fprintf(STDERR,
                 " level (zlib default),\n   and zlib version (%s)?\n\n",
                 ZLIB_VERSION);
               wrote_question = 1;
            }

            FCLOSE(fpin);
            FCLOSE(fpout);

            if (strict != 0 && unsupported_chunks == 0)
              return (1);

            else
              return (0);
         }

         if (num_in == 0)
            break;

         if (memcmp(inbuf, outbuf, num_in))
         {
            fprintf(STDERR, "\nFiles %s and %s are different\n", inname,
               outname);

            if (wrote_question == 0 && unsupported_chunks == 0)
            {
               fprintf(STDERR,
         "   Was %s written with the same maximum IDAT chunk size (%d bytes),",
                    inname, PNG_ZBUF_SIZE);
               fprintf(STDERR,
                 "\n   filtering heuristic (libpng default), compression");
               fprintf(STDERR,
                 " level (zlib default),\n   and zlib version (%s)?\n\n",
                 ZLIB_VERSION);
               wrote_question = 1;
            }

            FCLOSE(fpin);
            FCLOSE(fpout);

            /* NOTE: the unsupported_chunks escape is permitted here because
             * unsupported text chunk compression will result in the compression
             * mode being changed (to NONE) yet, in the test case, the result
             * can be exactly the same size!
             */
            if (strict != 0 && unsupported_chunks == 0)
              return (1);

            else
              return (0);
         }
      }
   }
#endif /* WRITE */

   FCLOSE(fpin);
   FCLOSE(fpout);

   return (0);
}

/* Input and output filenames */
#ifdef RISCOS
static PNG_CONST char *inname = "pngtest/png";
static PNG_CONST char *outname = "pngout/png";
#else
static PNG_CONST char *inname = "pngtest.png";
static PNG_CONST char *outname = "pngout.png";
#endif

int
main(int argc, char *argv[])
{
   int multiple = 0;
   int ierror = 0;

   png_structp dummy_ptr;

   int page = 0;
   int *png;
   int sz = 0;
   int w = 0;
   int h = 0;
   bool env;

      int i;
      for (i = 0; i<3; ++i)
      {
         int kerror;

         kerror = genTexture(page, *png, sz, w, h, env);

         if (kerror == 0)
         {
            if (verbose == 1 || i == 2)
            {
                fprintf(STDERR, " PASS\n");
            }
         }

         else
         {
           fprintf(STDERR, " FAIL\n");
            ierror += kerror;
         }
       }
   


   dummy_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   fprintf(STDERR, " Default limits:\n");
   fprintf(STDERR, "  width_max  = %lu\n",
      (unsigned long) png_get_user_width_max(dummy_ptr));
   fprintf(STDERR, "  height_max = %lu\n",
      (unsigned long) png_get_user_height_max(dummy_ptr));
   if (png_get_chunk_cache_max(dummy_ptr) == 0)
      fprintf(STDERR, "  cache_max  = unlimited\n");
   else
      fprintf(STDERR, "  cache_max  = %lu\n",
         (unsigned long) png_get_chunk_cache_max(dummy_ptr));
   if (png_get_chunk_malloc_max(dummy_ptr) == 0)
      fprintf(STDERR, "  malloc_max = unlimited\n");
   else
      fprintf(STDERR, "  malloc_max = %lu\n",
         (unsigned long) png_get_chunk_malloc_max(dummy_ptr));
   png_destroy_read_struct(&dummy_ptr, NULL, NULL);

   if(ierror == 0)
	   printf("TESTS PASSED\n");
   return (int)(ierror != 0);
}



