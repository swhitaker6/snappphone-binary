/*
*   A byte-oriented AES-256-CTR implementation.
*   Based on the code available at http://www.literatecode.com/aes128
*   Complies with RFC3686, http://tools.ietf.org/html/rfc3686
*
*   This demo uses RFC3686 Test Vector #9 for CTR and test vector from
*   FIPS PUB 197, Appendix C.3 for core ECB
*
*/
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "png.h"
#include "chacha20_simple.h"
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
// #include "aes128.h"
#include "demo.h"


#define UNUSED_    __attribute__((unused))


uint8_t ctxKey[32];
uint8_t ctxNonce[8];
uint64_t ctxCounter = 0;

uint8_t svcKey[32];
uint8_t svcNonce[8];
uint64_t svcCounter = 0;

int offset = 0;
png_bytep mem_ptr;

// void dump(char *s, uint32_t *buf, size_t sz);
int mem_isequal(const uint32_t *x, const uint32_t *y, size_t sz);


static const uint32_t RFC3686_TV3[] = {
	0xC1, 0xCF, 0x48, 0xA8, 0x9F, 0x2F, 0xFD, 0xD9,
	0xCF, 0x46, 0x52, 0xE9, 0xEF, 0xDB, 0x72, 0xD7,
	0x45, 0x40, 0xA4, 0x2B, 0xDE, 0x6D, 0x78, 0x36,
	0xD5, 0x9A, 0x5C, 0xEA, 0xAE, 0xF3, 0x10, 0x53,
	0x25, 0xB2, 0x07, 0x2F
};


static const uint32_t aes128_TV[] = {
	0x87, 0x4d, 0x61, 0x91, 0xb6, 0x20, 0xe3, 0x26,
	0x1b, 0xef, 0x68, 0x64, 0x99, 0x0d, 0xb6, 0xce
};



struct pointer {
	char point[4];
} user;

int num_row_bytes;
int BYTES_PER_PIXEL;
png_uint_32 png_offset = 0;
png_bytep row_array;
png_bytep *row_pointers;
// png_structp read_ptr;
// png_structp write_ptr;
// png_infop info_ptr;
// png_infop write_info_ptr;

png_structp png_ptr;
png_structp read_ptr;
png_infop info_ptr, read_info_ptr, end_info_ptr;

png_structp write_ptr;
png_infop write_info_ptr;
png_infop write_end_info_ptr;
int interlace_preserved = 1;

png_bytep row_buf;
png_uint_32 y;
png_uint_32 width, height;
int depth;
int channels;
int num_pass = 1, pass;
int bit_depth, color_type;
int total_bytes = 0;
int file_size;

int pngSize;
int chunkBytes = 8192;
int bufferSize;
png_bytep pngBuffer;
png_bytep processedBytes;
int expansion_factor = 6;



// void read_data(png_structp png_ptr, png_bytep data, png_size_t length, uint32_t *texture) {

// 	void *texture = mem_ptr;

// 	for(int i=0; i < length; i++) {

// 		data[i] = texture[i];

// 	}

// }


void hex2byte(const char *hex, uint8_t *byte)
{
  
  while (*hex) { 
    
    sscanf(hex, "%2hhx", byte++); 
    hex += 2; 
    
  }

}




void write_data(png_structp png_ptr, png_bytep data, png_size_t length) {

	png_bytep png = mem_ptr;

	total_bytes = total_bytes + length;

	// if(total_bytes > file_size) {
	// 	printf("%s%u\n\t", "REALLOCATING BUFFER: ", file_size);
	// 	realloc(png, file_size * 3);
	// 	file_size = file_size * 3;

	// }

	int write_length = ((length < 64) ? length : 64);

				printf("%s%u\n\t", "current write data length: ", (unsigned int)length);
				printf("%s%u\n\t", "total bytes written: ", total_bytes);				
				for (int l = 0; l < write_length; l++)
					printf("%02x%s", data[l], ((l % 16 == 15)/* && (l < 64 - 1)*/) ? "\n\t" : " ");
				printf("\n ");			

	for(int i=0; i < (int)length; i++) {

		png[png_offset + i] = data[i];

	}

	png_offset = png_offset + length;

}



void output_flush(png_structp  png_ptr) {

}


void BACKUP_info_callback(png_structp png_ptr, png_infop info) {

	png_start_read_image(png_ptr);

}



void info_callback(png_structp png_ptr, png_infop info) {


	int channels;


   //Transferring info struct
   {
      int interlace_type, compression_type, filter_type;
      printf("INSIDE INFO_CALLBACK FUNTION");
      if (png_get_IHDR(read_ptr, read_info_ptr, &width, &height, &bit_depth,
          &color_type, &interlace_type, &compression_type, &filter_type) != 0)
      {
         printf("SETTING IHDR INFO ITEMS IN WRITE INFO STRUCT TO...");
         printf("width = %u, height = %u, bit_depth = %i \n");
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

		num_row_bytes = png_get_rowbytes(png_ptr, read_info_ptr);

      }
   }
   //Transferring PNG_FIXED_POINT
   {
      //Transferring PNG_cHRM
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

      //Transferring PNG_gAMA
      {
            png_fixed_point gamma;

            if (png_get_gAMA_fixed(read_ptr, read_info_ptr, &gamma) != 0)
            png_set_gAMA_fixed(write_ptr, write_info_ptr, gamma);
      }
   }
   //Transferring PNG_FLOATING_POINT"
   {
      //Transferring PNG_cHRM_SUPPORTED
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

      //Transferring PNG_gAMA_SUPPORTED
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

//    // PNG_FIXED_POINT_SUPPORTED
//    {
//       int unit;
//       png_charp scal_width, scal_height;

//       if (png_get_sCAL_s(read_ptr, read_info_ptr, &unit, &scal_width,
//           &scal_height) != 0)
//       {
//          png_set_sCAL_s(write_ptr, write_info_ptr, unit, scal_width,
//              scal_height);
//       }
//    }
   
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





	// png_start_read_image(png_ptr);

}






void row_callback(png_structp png_ptr, png_bytep new_row, png_uint_32 row_num, int pass) {


	// int channels, num_row_bytes;

	// if(color_type == PNG_COLOR_TYPE_GRAY) {
	// 	channels = 1
	// } else if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
	// 	channels = 2
	// } else if (color_type == PNG_COLOR_TYPE_PALETTE) {
	// 	channels = 1
	// } else if (color_type == PNG_COLOR_TYPE_RGB) {
	// 	channels = 3
	// } else if (color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
	// 	channels = 4
	// }
	
	// int BYTES_PER_PIXEL = (bit_depth/8) * channels;
	
	// num_row_bytes = width * BYTES_PER_PIXEL;


		// printf("%s%u\n\t", "new row: ", row_num);
		// printf("%s%u\n\t", "new row pointer: ", new_row);
		// for (int l=0; l < 64; l++)
		// 	printf("%02x%s", new_row[l], ((l % 16 == 15)/* && (l < 64 - 1)*/) ? "\n\t" : " ");
		// printf("\n ");			

	
	// if(row_num < 1) {

	// 	printf("%s%u\n\t", "callback new row: ", row_num);
	// 	printf("%s%u\n\t", "callback new row pointer: ", new_row);
	// 	for (int l=0; l < 384; l++)
	// 		printf("%02x%s", new_row[l], ((l % 16 == 15)/* && (l < 64 - 1)*/) ? "\n\t" : " ");
	// 	printf("\n ");			

	// }

	// if(row_num == 64) {

	// 	printf("%s%u\n\t", "callback new row: ", row_num);
	// 	printf("%s%u\n\t", "callback new row pointer: ", new_row);
	// 	for (int l=0; l < 384; l++)
	// 		printf("%02x%s", new_row[l], ((l % 16 == 15)/* && (l < 64 - 1)*/) ? "\n\t" : " ");
	// 	printf("\n ");			

	// }


	BYTES_PER_PIXEL = 3;



	if(!row_pointers[row_num]) {

		row_pointers[row_num] = png_malloc(png_ptr, num_row_bytes);
		// row_pointers[row_num] = row_array = malloc(128 * BYTES_PER_PIXEL);

	}

	png_progressive_combine_row(png_ptr, row_pointers[row_num], new_row);

	

	// for(int i=0; i < num_row_bytes; i++) {
	// for(int i=0; i < 384; i++) {

	// 	row_array[i] = new_row[i];

	// }


	// row_pointers[row_num] = &row_array;
	// row_pointers[row_num] = new_row;

		// printf("%s%u\n\t", "row array: ", row_num);
		// printf("%s%u\n\t", "row array pointer: ", row_pointers[row_num]);
		// for (int l=0; l < 64; l++)
		// 	printf("%02x%s", row_array[l], ((l % 16 == 15)/* && (l < 64 - 1)*/) ? "\n\t" : " ");
		// printf("\n ");			





}


void end_callback(png_structp png_ptr, png_infop info) {


}



int initialize_png_writer() {
// int initialize_png_writer(png_structp png_ptr) {

	void (*write_data_fn_ptr)(png_structp png_ptr, png_bytep data, png_size_t length) = &write_data;

	void (*output_flush_fn_ptr)(png_structp png_ptr) = &output_flush;

	write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	write_info_ptr = png_create_info_struct(write_ptr);

   write_end_info_ptr = png_create_info_struct(write_ptr);	

	if(setjmp(png_jmpbuf(write_ptr))){
		png_destroy_write_struct(&write_ptr, (png_infopp)NULL);
		return 0;
	}

	png_set_write_fn(write_ptr, NULL, write_data_fn_ptr, output_flush_fn_ptr);


	return 1;

}



int initialize_png_reader() {


	read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	info_ptr = png_create_info_struct(read_ptr);

    end_info_ptr = png_create_info_struct(read_ptr);	

	if(setjmp(png_jmpbuf(read_ptr))){
		png_destroy_read_struct(&read_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return 0;

	}

	png_set_progressive_read_fn(read_ptr, &user, info_callback, row_callback, end_callback);
	// png_set_progressive_read_fn(png_ptr, (void *)user_ptr, info_callback, row_callback, end_callback);

	return 1;

}




// int aes_encrypt_decrypt(aes_context ctx, uint32_t *png, int sz, int page, int w, int h, bool env) {

// 	int imgSize;
// 	int rem;
// 	int nBlks;

// 	imgSize = process_data(png, sz);

// 	rem = imgSize % 16 ? 1 : 0;

// 	nBlks = (imgSize/16)+rem;

// 	printf("# wasm-CTR test\n");
	
// 	dump("SANITY CHECK...PNG File BEFORE:", png, sz); // PRINT PNG FILE BYTES TO STDOUT

// 	dump("PNG Image BEFORE:", *row_pointers, sz); // PRINT IMAGE DATA BEFORE

// 	aes128_setExpKey(&ctx, ctx.enckey);  // LOAD ENCRYPTION KEY INTO THE CONTEXT

// 	aes128_offsetCtr(&ctx.blk.ctr[0], page, nBlks);

// 	aes128_encrypt_progressive(&ctx, row_pointers, num_row_bytes, w, h, env); // todo: remove temp 'kstr' argument
	

// 	ctx.blk.ctr[0] = 0x00;
// 	ctx.blk.ctr[1] = 0x00;
// 	ctx.blk.ctr[2] = 0x00;
// 	ctx.blk.ctr[3] = 0x00;


// 	aes128_done(&ctx);


// }






int process_data(png_bytep buffer, png_uint_32 length){

	int height;
	int rowBytes;
	
	if(setjmp(png_jmpbuf(read_ptr))){

		png_destroy_read_struct(&read_ptr, &info_ptr, (png_infopp)NULL);
		return 0;
	}

	png_process_data(read_ptr, info_ptr, buffer, length);

	height =  png_get_image_height(read_ptr, info_ptr);

	rowBytes = png_get_rowbytes(read_ptr, info_ptr);

	return height * rowBytes;

}


void write_row_callback(png_structp png_ptr, png_uint_32 row, int pass)
{
  printf("row: %u, pass: %i\n", row, pass);
  
}





void read_row_callback(png_structp png_ptr, png_uint_32 row, int pass)
{
   printf("INSIDE READ_ROW_CALLBACK");
  printf("row: %u, pass: %i\n", row, pass);
  
}




void png_read_fn(png_structp png_ptr, png_bytep data, size_t read_length) {

      printf("INSIDE PNG_READ_FN");
      
      png_bytep filebytes;

      png_byte* png_data;

      printf("ABOUT TO CAL PNG_GET_IO_PTR");
      filebytes = (png_bytep)png_get_io_ptr(png_ptr);

      png_data = filebytes + offset;

      printf("read_length: %u\n", (unsigned int)read_length);

      offset += read_length;

      printf("offset: %u\n", (unsigned int)offset);

      memcpy(data, png_data, read_length);
      printf("FINISHED MEMCPY CALL");



}










                
int genTexture(int page, uint32_t *pngBuffer, int bufferSize, uint32_t *pngOut, int pngOutSize, int w, int h, int env) {
// int genTexture(int page, uint32_t *pFile, int szFile, uint32_t *pFileOut, int szFileOut, int w, int h, int env) {
// int genTexture(int page, uint32_t *png, int sz, int w, int h, int env) {


  hex2byte("1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0", ctxKey);
  hex2byte("0000000000000002", ctxNonce);

  hex2byte("1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0", svcKey);
  hex2byte("0000000000000002", svcNonce);


	mem_ptr = (png_bytep) pngBuffer;
	// mem_ptr = (png_bytep) pFile;

	file_size = bufferSize;
	// file_size = szFile;
	// int sz = w*h;
	// uint32_t kstr[sz];

	// int BYTES_PER_PIXEL = 3;
	int imgSize;
	int rem;
	int nBlks;
	// int rem = sz % 16 ? 1 : 0;
	// int nBlks = (sz/16)+rem;

	//INIT_AES_CONTEXT(ctx);
	chacha20_ctx ctx;
	chacha20_ctx svc;
	// static aes_context ctx;
    const uint8_t *key = 0;

	int rc;
	// uint32_t i;
	int row_bytes;

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

  for (size_t i = 0; i < height; i++)
  {

    chacha20_encrypt(&ctx, &svc, row_pointers[i], outbuf, row_bytes, env);

    memcpy(row_pointers[i], outbuf, row_bytes);

  }


  printf("\n AFTER encrypt - ");

  for(int i=0; i<row_bytes; i++) {

    printf("%X", row_pointers[(unsigned int)floor(height/2)][i]);

  }



	// chacha_encrypt_decrypt(png, sz);

/////////////////////////////////////////////////////////////////////////////////////////


  png_set_rows(write_ptr, write_info_ptr, row_pointers);    

  png_set_crc_action(write_ptr, PNG_CRC_QUIET_USE, PNG_CRC_QUIET_USE);

  offset = 0;


//   pngOutSize = height*row_bytes+chunkBytes;

  printf("pngOutSize = %u \n", pngOutSize);

//   pngOut = (png_bytep)png_malloc(read_ptr, pngOutSize);


  for(int i=0; i<8; i++) {

    printf("buffer cleared: %X\n", pngOut[i]);

  }

  png_set_write_fn(write_ptr, pngOut, png_write_fn, output_flush_fn);
 
  png_set_write_status_fn(png_ptr, write_row_callback);

  png_write_png(write_ptr, write_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);


  for(int i=0; i<8; i++) {

    printf("buffer written: %X\n", pngOut[i]);

  }


  png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);

  png_destroy_write_struct(&write_ptr, &write_info_ptr);

      return 0;


}




// int genTexture_sequential(int page, uint32_t *texture, int height, int width, bool env) {

// 	mem_ptr = texture;

// 	void (*read_data_fn)(png_structp png_ptr, png_bytep data, png_size_t length) = &read_data;

// 	void (*write_data_fn)(png_structp png_ptr, png_bytep data, png_size_t length) = &write_data;

// 	uint32_t kstr[sz];
// 	int *row_pointers;

// 	int rem = sz % 16 ? 1 : 0;
// 	int nBlks = (sz/16)+rem;

// 	INIT_AES_CONTEXT(ctx);
// 	// static aes_context ctx;

// 	int rc;
// 	// uint32_t i;

// 	bool is_png = !png_sig_cmp(texture, 0, 8);
// 	if(!is_png) {

// 		printf("NOT A PNG file\n");

// 	}


// 	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
// 	if(!png_ptr) {

// 		printf('ERROR: Could not create read struct');

// 	}

// 	png_infop info_ptr = png_create_info_struct(png_ptr);

// 	png_infop end_ptr = png_create_info_struct(png_ptr);

// 	if(setjmp(png_jmpbuf(png_ptr))) {

// 		png_destroy_read_struct($png_str, &info_ptr, (png_infopp)NULL)

// 	}

// 	png_set_read_fn(png_ptr, NULL, read_data_fn);

// 	row_pointers = png_malloc(png_ptr, height*sizeof(png_bytep));

// 	png_image = png_malloc(png_ptr, height*width*pixel_size);
// 	for(int i=0; i<height; i++) {

// 		row_pointers[i] = png_image+(i*width);

// 	}

// 	png_set_rows(png_ptr, info_ptr, &row_pointers);

// 	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);



// 	printf("# wasm-CTR test\n");
	
// 	dump("Texture:", row_pointers, sz); // PRINT PLAINTEXT BYTES TO STDOUT
// 	// dump("Texture:", texture, sz); // PRINT PLAINTEXT BYTES TO STDOUT

// 	aes128_setExpKey(&ctx, ctx.enckey);  // LOAD ENCRYPTION KEY INTO THE CONTEXT

// 	aes128_offsetCtr(&ctx.blk.ctr[0], page, nBlks);

// 	aes128_encrypt(&ctx, row_pointers, sz, env, kstr); // todo: remove temp 'kstr' argument
// 	// aes128_encrypt(&ctx, texture, sz, env, kstr); // todo: remove temp 'kstr' argument
	
// 	dump("keyStr:", kstr, sz); // PRINT CYPHERTEXT RESULT
	
// 	dump("Result:", row_pointers, sz); // PRINT CYPHERTEXT RESULT
//  	// dump("Result:", texture, sz); // PRINT CYPHERTEXT RESULT
	
// 	rc = mem_isequal(row_pointers, aes128_TV, 16);// COMPARE TO AES TEST VECTOR
// 	// rc = mem_isequal(texture, aes128_TV, 16);// COMPARE TO AES TEST VECTOR

// 	printf("\t^ %s\n", (rc == 0) ? "Ok" : "INVALID" );



// 	png_structp write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

// 	png_infop info_ptr = png_create_info_struct(png_ptr);

// 	png_set_write_fn(write_ptr, NULL, write_data_fn, NULL, NULL);

// 	png_set_rows(row_pointers);

// 	png_write_png(write_ptr, info_ptr, PNG_TRANSFORM_IDENTITY);



// 	/* reset the counter to decrypt */
// 	ctx.blk.ctr[0] = 0x00;
// 	ctx.blk.ctr[1] = 0x00;
// 	ctx.blk.ctr[2] = 0x00;
// 	ctx.blk.ctr[3] = 0x00;

// 	// gen_table_mod_x(0x02);
// 	// gen_table_mod_x(0x03);

// 	// aes128_done(&ctx);

// 	// return 0;
// }




// int genTexture_original(int page, uint32_t *texture, int sz, bool env) {


// 	uint32_t kstr[sz];

// 	int rem = sz % 16 ? 1 : 0;
// 	int nBlks = (sz/16)+rem;

// 	INIT_AES_CONTEXT(ctx);
// 	// static aes_context ctx;

// 	int rc;
// 	// uint32_t i;

// 	printf("# wasm-CTR test\n");
	
// 	dump("Texture:", texture, sz); // PRINT PLAINTEXT BYTES TO STDOUT

// 	aes128_setExpKey(&ctx, ctx.enckey);  // LOAD ENCRYPTION KEY INTO THE CONTEXT

// 	aes128_offsetCtr(&ctx.blk.ctr[0], page, nBlks);

// 	aes128_encrypt(&ctx, texture, sz, env, kstr); // todo: remove temp 'kstr' argument
	
// 	dump("keyStr:", kstr, sz); // PRINT CYPHERTEXT RESULT
	
// 	dump("Result:", texture, sz); // PRINT CYPHERTEXT RESULT
	
// 	rc = mem_isequal(texture, aes128_TV, 16);// COMPARE TO AES TEST VECTOR

// 	printf("\t^ %s\n", (rc == 0) ? "Ok" : "INVALID" );

// 	/* reset the counter to decrypt */
// 	ctx.blk.ctr[0] = 0x00;
// 	ctx.blk.ctr[1] = 0x00;
// 	ctx.blk.ctr[2] = 0x00;
// 	ctx.blk.ctr[3] = 0x00;

// 	// gen_table_mod_x(0x02);
// 	// gen_table_mod_x(0x03);

// 	// aes128_done(&ctx);

// 	// return 0;
// }









uint32_t getTexture(uint32_t page, uint32_t *tex) {


   //PREINIT CODE//////////////////////////////////////////////////////////////////////////
	// var imgTypedArray = new Uint8TypedArray(text);
	// var buf = Module._malloc(imgTypedArray.length*imgTypedArray.BYTES_PER_ELEMENT);
	// Module.HEAPU8.writeArrayToMemory(imgTypedArray, buf);
	// var genTexture = Module.cwrap(
	// 	'_genTexture', // name of C function
	// 	'number', // return type
	// 	['number', 'number'],
	// 	[page, buf]
	// ); // argument types:
	//END PREINIT/////////////////////////////////////////////////////////////////////////



   //PREINIT CODE//////////////////////////////////////////////////////////////////////////
	// var imgTypedArray = new Uint8TypedArray(text);
	// // var imgTypedArray = new Uint8TypedArray(imgData);
	// var buf = Module._malloc(imgTypedArray.length*imgTypedArray.BYTES_PER_ELEMENT);
	// Module.HEAPU8.writeArrayToMemory(imgTypedArray, buf);
	// // Module.HEAPU8.set(imgTypedArray, buf);
	// var genTexture = Module.cwrap(
	// 	'_genTexture', // name of C function
	// 	'number', // return type
	// 	['number', 'number'],
	// 	[page, buf]
	// ); // argument types:
	//END PREINIT/////////////////////////////////////////////////////////////////////////



	// genTexture(1, buf)); // 30
	// THREE.texture.load(1, Module.HEAPU8.get(imgTypedArray, buf));

	// genTexture(2, buf)); // 50
	// THREE.texture.load(1, Module.HEAPU8.get(imgTypedArray, buf));

	// Module._free(buf);


	// var imgTypedArray = new Uint8TypedArray(imgData);
	// var buf = Module._malloc(imgTypedArray.length*imgTypedArray.BYTES_PER_ELEMENT);
	// Module.HEAPU8.set(imgTypedArray, buf);
	// Module.ccall('genTexture', 'number', ['number', 'number'], [page, buf], { async : true })
	// .then(function (response) {
	// 	var texture = Module.HEAPU8.get(imgTypedArray, buf);
	// 	Module._free(buf);
	// })

   return 0;

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

      for(int i=0; i<16 && i<(int)read_length; i++) {

         printf("%X", png_data[i]);

      }

      printf("...\n");

}





void output_flush_fn(png_structp png_ptr) {

    printf("output_flushed: %X\n", 1);


}






   /* Table of CRCs of all 8-bit messages. */
   unsigned long crc_table[256];
   
   /* Flag: has the table been computed? Initially false. */
   int crc_table_computed = 0;
   
   /* Make the table for a fast CRC. */
   void make_crc_table(void)
   {
     unsigned long c;
     int n, k;
   
     for (n = 0; n < 256; n++) {
       c = (unsigned long) n;
       for (k = 0; k < 8; k++) {
         if (c & 1)
           c = 0xedb88320L ^ (c >> 1);
         else
           c = c >> 1;
       }
       crc_table[n] = c;
     }
     crc_table_computed = 1;
   }
   
   /* Update a running CRC with the bytes buf[0..len-1]--the CRC
      should be initialized to all 1's, and the transmitted value
      is the 1's complement of the final running CRC (see the
      crc() routine below)). */
   
   unsigned long update_crc(unsigned long crc, unsigned char *buf,
                            int len)
   {
     unsigned long c = crc;
     int n;
   
     if (!crc_table_computed)
       make_crc_table();
     for (n = 0; n < len; n++) {
       c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
     }
     return c;
   }
   
   /* Return the CRC of the bytes buf[0..len-1]. */
   unsigned long crc(unsigned char *buf, int len)
   {
     return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
   }









int main ()
// int main (UNUSED_ int argc, UNUSED_ char *argv[])
{

  hex2byte("1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0", ctxKey);
  hex2byte("0000000000000002", ctxNonce);

  hex2byte("1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0", svcKey);
  hex2byte("0000000000000002", svcNonce);

  int env = 1;
  const uint8_t *key = 0;
  chacha20_ctx ctx;
  chacha20_ctx svc;
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


  FILE *pfile = fopen("images/sand.png", "rb");
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
       printf("COULD NOT CREATE WRITE INFO STRUCT");
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

  chacha20_setup(&ctx, ctxKey, sizeof(ctxKey), ctxNonce);
  chacha20_setup(&svc, svcKey, sizeof(svcKey), svcNonce);
  
  memset(outbuf, 0, row_bytes);
  chacha20_counter_set(&ctx, ctxCounter);
  chacha20_counter_set(&svc, svcCounter);

  for (size_t i = 0; i < (unsigned int)height; i++)
  {

    chacha20_encrypt(&ctx, &svc, row_pointers[i], outbuf, row_bytes, env);

    memcpy(row_pointers[i], outbuf, row_bytes);

  }

  printf("\n AFTER encrypte/decrypt - ");

  for(int i=0; i<row_bytes; i++) {

    printf("%X", row_pointers[height/2][i]);

  }

//   ctxCounter = 0;  
//   svcCounter = 0;  
//   memset(outbuf, 0, len);
//   chacha20_counter_set(&ctx, ctxCounter);
//   chacha20_counter_set(&svc, svcCounter);

//   for (size_t i = 0; i < (unsigned int)height; i++)
//   {

//     chacha20_encrypt(&ctx, &svc, row_pointers[i], outbuf, row_bytes, env);

//     memcpy(row_pointers[i], outbuf, row_bytes);

//   }

//   printf("\n AFTER encrypte/decrypt - ");

//   for(int i=0; i<row_bytes; i++) {

//     printf("%X", row_pointers[height/2][i]);

//   }





  ret = 0;

 err:
  /* Clean up all the resources we allocated */
  if (ret != 0) {
    // ERR_print_errors_fp(stderr);
    return ret;
  }



  png_set_rows(write_ptr, write_info_ptr, row_pointers);    


  row_pointers = png_get_rows(write_ptr, write_info_ptr);


   printf("\n AFTER PNG_SET_ROWS \n");

  for(int i=0; i<row_bytes; i++) {

    printf("%X", row_pointers[height/2][i]);

  }

  offset = 0;

  memset(pngBuffer, 0, bufferSize); 

   printf("/n");
  for(int i=0; i<8; i++) {

    printf("buffer cleared: %X\n", pngBuffer[i]);

  }

  png_set_write_fn(write_ptr, pngBuffer, png_write_fn, output_flush_fn);

  png_set_write_status_fn(png_ptr, write_row_callback);

  png_write_png(write_ptr, write_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);


  for(int i=0; i<8; i++) {

    printf("buffer written: %X\n", pngBuffer[i]);

  }


//   pfile = fopen("images/sand.png", "wb");
//   // FILE *pfile = fopen("images/hello_world_file.txt", "rb");
//   if (!pfile) {
//     printf("cannot open file\n");
//     return 1;
//   }

//   pngSize = offset;

//   fwrite(pngBuffer, 1, pngSize, pfile);
//   if (!png_check_sig(pngBuffer, 8)) {
//     printf("bad signature\n");
//     return 2;   /* bad signature */
//   } else {
//     printf("this is a PNG file\n");
//   }



//   fseek(pfile, 0, SEEK_END);

//   pngSize = ftell(pfile);

//   fseek(pfile, 0, SEEK_SET);

//   printf("finalWriteOffset = %u \n", offset);

//   printf("pngSize = %u \n", pngSize);


//   fclose (pfile);

//   free(pngBuffer);

  png_destroy_read_struct(&read_ptr, &read_info_ptr, NULL);

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


//   SDL_Init(SDL_INIT_VIDEO);

//   SDL_RWops *rw = SDL_RWFromMem(pngBuffer, pngSize);

//   SDL_Surface *temp = IMG_LoadPNG_RW(rw);

//   if (temp == NULL) { 
//    printf("Unable to load png: %s\n", SDL_GetError()); exit(1);
//   }

// //   SDL_Surface *image; 
  
//   image = SDL_DisplayFormat(temp);

//   SDL_FreeSurface(temp);
// //   image = IMG_Load("images/sand.png");
  
//   if (!image)
//   {
//      printf("IMG_Load: %s\n", IMG_GetError());
//      return 0;
//   }

//   printf("Image height is %u \n", image->h); 
//   printf("Image width is %u \n", image->w); 
//   printf("BitsPerPixel is %u \n", image->format->BitsPerPixel);
//   printf("BytesPerPixel is %u \n", image->format->BytesPerPixel);
//   printf("Image stride is %u \n", image->pitch);


//   screen = SDL_SetVideoMode(image->w, image->h, 32, SDL_SWSURFACE);

//   SDL_BlitSurface (image, NULL, screen, NULL);
//   SDL_FreeSurface (image);


//   SDL_Flip(screen);

//   SDL_Quit();

  free(pngBuffer);
//   pfile = fopen("images/sand.png", "rb");
//   // FILE *pfile = fopen("images/hello_world_file.txt", "rb");
//   if (!pfile) {
//     printf("cannot open file\n");
//     return 1;
//   }
  
// //   unsigned char sig[8];


//   fread(sig, 1, 8, pfile);
//   if (!png_check_sig(sig, 8)) {
//     printf("bad signature\n");
//     return 2;   /* bad signature */
//   } else {
//     printf("this is a PNG file\n");
//   }


//   // initialize_png_reader();
//   read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);

//   if (!read_ptr) {
//       return 3;
//   }

//   read_info_ptr = png_create_info_struct(read_ptr);

//   if (!read_info_ptr) {
//       png_destroy_read_struct(&read_ptr,
//           (png_infopp)NULL, (png_infopp)NULL);
//       return 4;
//   }


//   if (setjmp(png_jmpbuf(read_ptr)))
//   {
//       png_destroy_read_struct(&read_ptr, &read_info_ptr,
//           (png_infopp)NULL);
//       fclose(pfile);
//       return 5;
//   }





//   fseek(pfile, 0, SEEK_END);

//   pngSize = ftell(pfile);

//   fseek(pfile, 0, SEEK_SET);

//   printf("pngSize = %u \n", pngSize);

//   bufferSize = pngSize+1000;



  

//   printf("bufferSize = %u \n", bufferSize);

//   pngBuffer = (png_bytep)png_malloc(read_ptr, bufferSize);

//   fread(pngBuffer, 1, pngSize, pfile);

//   fclose (pfile);



//   /////////////////////////////////////////////////// pngBuffer, pngSize, bufferSize

//   for(int i=0; i<8; i++) {

//     printf("signature: %X\n", pngBuffer[i]);

//   }


//   png_set_read_fn(read_ptr, pngBuffer, png_read_fn);

//   png_set_read_status_fn(read_ptr, read_row_callback);

//   png_read_png(read_ptr, read_info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

// //   info_callback(read_ptr, read_info_ptr);
//    printf("ABOUT TO CALL PNG_GET_ROWS");
//   row_pointers = png_get_rows(read_ptr, read_info_ptr);


//   height = (unsigned int)png_get_image_height(read_ptr, read_info_ptr);
//   width = (unsigned int)png_get_image_width(read_ptr, read_info_ptr);   
//   depth = (unsigned int)png_get_bit_depth(read_ptr, read_info_ptr);
//   channels = (unsigned int)png_get_channels(read_ptr, read_info_ptr);
//   row_bytes = (unsigned int)png_get_rowbytes(read_ptr, read_info_ptr);

//   printf("Image height is %u \n", (unsigned int)png_get_image_height(read_ptr, read_info_ptr)); 
//   printf("Image width is %u \n", (unsigned int)png_get_image_width(read_ptr, read_info_ptr)); 
//   printf("BitsPerPixel is %u \n", depth * channels);
//   printf("BytesPerPixel is %u \n", (unsigned int)png_get_channels(read_ptr, read_info_ptr));
//   printf("Image stride is %u \n", (unsigned int)png_get_rowbytes(read_ptr, read_info_ptr));


//   printf("BEFORE encrypt/decrypt - ");

//   for(int i=0; i<row_bytes; i++) {

//     printf("%X", row_pointers[height/2][i]);

//   }


//   free(pngBuffer);

//   png_destroy_write_struct(&read_ptr, &read_info_ptr);



//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////


	// static uint32_t enckey[16] = {
	// 	0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
	// 	0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
	// };

	// static uint32_t text[] = {
	// 	0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
	// 	0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
	// 	0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
	// 	0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
	// 	0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
	// 	0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
	// 	0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
	// 	0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
	// };

	// static uint32_t kstr[sizeof(text)];


	// static rfc3686_blk ctr_blk = {
	// 	{0xf0, 0xf1, 0xf2, 0xf3},                         /* nonce   */
	// 	{0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb}, /* IV      */
	// 	// {0x00, 0x00, 0x00, 0x01}                          /* counter */
	// 	{0xfc, 0xfd, 0xfe, 0xff}                          /* counter */
	// };


	// // static rfc3686_blk ctr_blk = {
	// // 	{0x32, 0x43, 0xf6, 0xa8},                         /* nonce   */
	// // 	{0x88, 0x5a, 0x30, 0x8d, 0x31, 0x31, 0x98, 0xa2}, /* IV      */
	// // 	{0xe0, 0x37, 0x07, 0x34}                          /* counter */
	// // };

	// static aes_context ctx;
	// int rc;
	// uint32_t i;
	// /*  **********************************************************
	// *   First we test CTR
	// */

	// printf("# CTR test\n");
	// dump("Text:", text, sizeof(text)); // PRINT PLAINTEXT BYTES TO STDOUT
	// // dump("Key:", enckey, sizeof(enckey)); // PRINT ENCRYPTION KEY BYTES TO STDOUT

	// aes128_setExpKey(&ctx, enckey);  // LOAD ENCRYPTION KEY INTO THE CONTEXT
	// aes128_setCtrBlk(&ctx, &ctr_blk); // LOAD COUNTER BLOCK INTO THE CONTEXT

	// // aes128_encrypt(&ctx, text, kstr, sizeof(text)); // ENCRYPT PLAINTEXT
	// dump("keyStr:", kstr, sizeof(kstr)); // PRINT CYPHERTEXT RESULT
	// dump("Result:", text, sizeof(text)); // PRINT CYPHERTEXT RESULT
	// rc = mem_isequal(text, aes128_TV, sizeof(aes128_TV));// COMPARE TO AES TEST VECTOR
	// printf("\t^ %s\n", (rc == 0) ? "Ok" : "INVALID" );

	// /* reset the counter to decrypt */
	// ctr_blk.ctr[0] = 0xfc;
	// ctr_blk.ctr[1] = 0xfd;
	// ctr_blk.ctr[2] = 0xfe;
	// ctr_blk.ctr[3] = 0xff;
	// // ctr_blk.ctr[0] = ctr_blk.ctr[1] = ctr_blk.ctr[2] = 0;
	// // ctr_blk.ctr[3] = 1;
	// aes128_setCtrBlk(&ctx, &ctr_blk);
	// // aes128_decrypt(&ctx, text, kstr, sizeof(text));
	// dump("keyStr:", kstr, sizeof(kstr)); // PRINT CYPHERTEXT RESULT
	// dump("Text:", text, sizeof(text));

	// gen_table_mod_x(0x02);
	// gen_table_mod_x(0x03);

	// aes128_done(&ctx);

	return 0;
} /* main */





/* -------------------------------------------------------------------------- */
// void dump(char *s, uint32_t *buf, size_t sz)
// {
// 	size_t i;


// 	printf("%s\t", s);
// 	for (i = 0; i < sz; i++)
// 		printf("%02x%s", buf[i], ((i % 16 == 15) && (i < sz - 1)) ? "\n\t" : " ");
// 	printf("\n ");
// } /* dump */


/* -------------------------------------------------------------------------- */
int mem_isequal(const uint32_t *x, const uint32_t *y, size_t sz)
{
	size_t i;
	int rv = -1 ;

	if ( (sz > 0) && (x != NULL) && (y != NULL) )
		for (i = 0, rv = 0; i < sz; i++)
			rv |= (x[i] ^ y[i]);

	return rv;
} /* mem_isequal */
