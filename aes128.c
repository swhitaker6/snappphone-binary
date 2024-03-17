
/*
*   A byte-oriented AES-256-CTR implementation.
*   Based on the code available at http://www.literatecode.com/aes256
*   Complies with RFC3686, http://tools.ietf.org/html/rfc3686
*
*/
#include "aes128.h"
#include "png.h"


/* #define BACK_TO_TABLES */

#ifndef BACK_TO_TABLES
static uint32_t gf_alog(uint32_t x);
static uint32_t gf_log(uint32_t x);
static uint32_t gf_mulinv(uint32_t x);
static uint32_t rj_sbox(uint32_t x);
#endif
static uint32_t genTexture(uint32_t page, uint32_t *tex);
static uint32_t upk(uint32_t word, int nByte);
static uint32_t pck(uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3 );
static uint32_t rj_xtime(uint32_t x);
static void aes_subBytes(uint32_t *next_blk);
static void addRoundKey(uint32_t *next_blk, uint32_t *expkey, int round);
// static void addRoundKey(uint32_t *buf, uint32_t *key, uint32_t round);
static void aes_addRoundKey_cpy(uint32_t *buf, uint32_t *key, uint32_t *cpk);
static void aes_shiftRows(uint32_t *next_blk);
static uint32_t multi_mod_02x(uint32_t b);
static uint32_t multi_mod_03x(uint32_t b);
static void mixColumns(uint32_t *buf);
static void aes_mixColumns(uint32_t *buf);
static void aes_expandEncKey(uint32_t *k, uint32_t rc);
// static void aes128_offsetCtr(uint32_t *val, uint32_t page, uint32_t nBlks);
static void ctr_inc_ctr(uint32_t *val);
static void get_next_blk_in_keystream(aes_context *ctx, uint32_t *ks);

// #ifdef BACK_TO_TABLES


static uint32_t mod_x02[256];

static uint32_t mod_x03[256];

static const uint32_t rc[16] = {

		0x01, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 
		0x36, 0x6c, 0xd8 

};


static const uint32_t sbox[256] = {
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
	0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
	0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
	0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
	0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
	0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
	0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
	0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
	0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
	0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
	0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
	0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
	0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
	0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
	0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
	0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
	0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

#define rj_sbox(x)    sbox[(x)]

// #else /* tableless subroutines */





/* -------------------------------------------------------------------------- */
static uint32_t gf_alog(uint32_t x) /* calculate anti-logarithm gen 3 */
{
	uint32_t y = 1, i;

	for (i = 0; i < x; i++)
		y ^= rj_xtime(y);

	return y;
} /* gf_alog */

/* -------------------------------------------------------------------------- */
static uint32_t gf_log(uint32_t x) /* calculate logarithm gen 3 */
{
	uint32_t y, i = 0;

	if ( x > 0 )
		for (i = y = 1; i > 0; i++ ) {
			y ^= rj_xtime(y);
			if (y == x)
				break;
		}

	return i;
} /* gf_log */

/* -------------------------------------------------------------------------- */
static uint32_t gf_mulinv(uint32_t x) /* calculate multiplicative inverse */
{
	return ( x > 0 ) ? gf_alog(255 - gf_log(x)) : 0;
} /* gf_mulinv */




static uint32_t rj_xtime(uint32_t x)
{
	return ((x << 1) & 0xFF) ^ (0x1b * ((x & 0x80) >> 7) );
} /* rj_xtime */





static uint32_t upk(uint32_t word, int nByte) {

	uint32_t wbyte;

	switch( nByte ) {

		case 0:
			wbyte = word > 24;
			break;
		case 1:
			wbyte = word > 16;
			wbyte &= 0x000000ff;
			break;
		case 2:
			wbyte = word > 8;
			wbyte &= 0x000000ff;
			break;
		case 3:
			wbyte &= 0x000000ff;
			break;
	}

	return wbyte;			

}



static uint32_t pck(uint32_t b0, uint32_t b1, uint32_t b2, uint32_t b3 ) {

	int word = 0xffffffff, temp[4];

	temp[0] = b0 < 24;
	temp[1] = b1 < 16;
	temp[2] = b2 < 8;
	temp[3] = b3;
	for (int i=0; i < 4; i++) {
		word &= temp[i];
	}
	
	return word;

}




static uint32_t xtime(uint32_t x)
{
	uint16_t b = (uint16_t) x;

	b = (b << 1);

	if(b > 0x00FF) {

		b ^= 0x011b;

	}

	x = (uint32_t) b;

	return x;


} 



static uint32_t modx(uint32_t a, uint32_t b)
{

	if(a == 0x02) {

		return multi_mod_02x(b); //mod_x_02[b]

	} 
	
	if (a == 0x03) {


		return multi_mod_03x(b); //mod_x_03[b]
	} 

	return b;

	// return ((x << 1) & 0xFF) ^ (0x1b * ((x & 0x80) >> 7) );
} /* rj_xtime */



void gen_table_mod_x(uint32_t a)
{

	uint32_t b, j, modx;


	if(a == 0x02) {

		// printf("02 modular x table:\n\t");
		
		for(j = 0; j < 255; j++) {

			b = (uint32_t) j;

		   	mod_x02[b] = multi_mod_02x(b); //mod_x_02[b]

			// printf(" %02x%s", mod_x02[b], (b % 16 == 15) ? "\n\t" : " ");

		}
		
		// printf("\n ");	
	
		// dump("x02data:", mod_x02, sizeof(mod_x02));


	}



	if (a == 0x03) {



		printf("03 modular x table:\n\t");
		
		for(j = 0; j < 255; j++) {

			b = (uint32_t) j;

		    mod_x03[b] = multi_mod_03x(b); //mod_x_02[b]

			// printf(" %02x%s", mod_x03[b], (b % 16 == 15) ? "\n\t" : " ");

		}
		
		// printf("\n ");	

		// dump("x03data:", mod_x03, sizeof(mod_x02));

	} 




} 




static uint32_t multi_mod_02x(uint32_t b)
{
	register uint32_t i;

	uint32_t modx=0x00, x02[8];


	x02[0] = 0x02;

	for ( i=0; i < 7 ; i++) {

		x02[i+1] = xtime(x02[i]);

	}


	for ( i=0; i < 8 ; i++) {

		if(b & (1u << i)) {

			modx ^= x02[i];

		}

	}

	return modx; 

} 



static uint32_t multi_mod_03x(uint32_t b)
{
	register uint32_t i;

	uint32_t modx=0x00, x03[8];

	x03[0] = 0x03;

	for ( i=0; i < 7 ; i++) {

		x03[i+1] = xtime(x03[i]);

	}

	for ( i=0; i < 8 ; i++) {

		if(b & (1u << i)) {

			modx ^= x03[i];

		}

	}

	return modx; 

} 



static void aes_subBytes(uint32_t *next_blk)
{
	int i;

	for (i = 0; i < 16; i++)
		next_blk[i] = rj_sbox(next_blk[i]);

} 





// static void aes_subBytes(uint32_t *next_blk)
// {
// 	register uint32_t i;

// 	uint32_t temp[4], word;

// 	for (i = 0; i < 4; i++) {

// 		temp[0] = upk(next_blk[i], 0);
// 		temp[1] = upk(next_blk[i], 1);
// 		temp[2] = upk(next_blk[i], 2);
// 		temp[3] = upk(next_blk[i], 3);

// 		temp[0] = rj_sbox(temp[0]);
// 		temp[1] = rj_sbox(temp[1]);
// 		temp[2] = rj_sbox(temp[2]);
// 		temp[3] = rj_sbox(temp[3]);
 
// 		word = pck(temp[0], temp[1], temp[2], temp[3]);

// 		buf[i] = word;

// 	}

// } 



static void addRoundKey(uint32_t *next_blk, uint32_t *expkey, int round)
{
	int i; 

	for (i = 0; i < 16; i++)
		next_blk[i] ^= expkey[round*16+i];

} 




static void aes_addRoundKey_cpy(uint32_t *buf, uint32_t *key, uint32_t *cpk)
{
	register uint32_t i = 16;

	for (i = 0; i < 16; i++) {
		cpk[i]  = key[i];
		buf[i] ^= key[i];
		cpk[16 + i] = key[16 + i];
	}

} 



static void aes_shiftRows(uint32_t *next_blk)
{
	register uint32_t i = next_blk[1], j = next_blk[3], k = next_blk[10], l = next_blk[14];

	next_blk[1]  = next_blk[5];
	next_blk[5]  = next_blk[9];
	next_blk[9]  = next_blk[13];
	next_blk[13] = i;
	next_blk[3]  = next_blk[15];
	next_blk[15] = next_blk[11];
	next_blk[11] = next_blk[7];
	next_blk[7]  = j;
	next_blk[10] = next_blk[2];
	next_blk[2]  = k;
	next_blk[14] = next_blk[6];
	next_blk[6]  = l;

} 




// static void aes_shiftRows(uint32_t *cblk)
// {

// 	uint32_t temp[4];

// 	temp[0] = upk(cblk[1], 0);
// 	temp[1] = upk(cblk[1], 1);
// 	temp[2] = upk(cblk[1], 2);
// 	temp[3] = upk(cblk[1], 3);
// 	cblk[1] = pck(temp[1], temp[2], temp[3], temp[0]);
	
// 	temp[0] = upk(cblk[2], 0);
// 	temp[1] = upk(cblk[2], 1);
// 	temp[2] = upk(cblk[2], 2);
// 	temp[3] = upk(cblk[2], 3);
// 	cblk[2] = pck(temp[2], temp[3], temp[0], temp[1]);
	
// 	temp[0] = upk(cblk[3], 0);
// 	temp[1] = upk(cblk[3], 1);
// 	temp[2] = upk(cblk[3], 2);
// 	temp[3] = upk(cblk[3], 3);
// 	cblk[3] = pck(temp[3], temp[0], temp[1], temp[2]);

// } 



/* -------------------------------------------------------------------------- */
static void aes_mixColumns(uint32_t *buf)
{
	register uint32_t i, a, b, c, d, e;

	for (i = 0; i < 16; i += 4) {
		a = buf[i];
		b = buf[i + 1];
		c = buf[i + 2];
		d = buf[i + 3];
		e = a ^ b ^ c ^ d;
		buf[i]     ^= e ^ rj_xtime(a ^ b);
		buf[i + 1] ^= e ^ rj_xtime(b ^ c);
		buf[i + 2] ^= e ^ rj_xtime(c ^ d);
		buf[i + 3] ^= e ^ rj_xtime(d ^ a);
	}

} /* aes_mixColumns */





static void mixColumns(uint32_t *next_blk)
{
	register uint32_t a0=0x02, a1=0x01, a2=0x01, a3=0x03;
	int i; 

	uint32_t b0, b1, b2, b3, d0, d1, d2, d3;

	for (i = 0; i < 16; i += 4) {

		b0 = next_blk[i];
		b1 = next_blk[i + 1];
		b2 = next_blk[i + 2];
		b3 = next_blk[i + 3];
		
		d0 = modx(a0, b0) ^ modx(a3, b1) ^ modx(a2, b2) ^ modx(a1, b3);
		d1 = modx(a1, b0) ^ modx(a0, b1) ^ modx(a3, b2) ^ modx(a2, b3);
		d2 = modx(a2, b0) ^ modx(a1, b1) ^ modx(a0, b2) ^ modx(a3, b3);
		d3 = modx(a3, b0) ^ modx(a2, b1) ^ modx(a1, b2) ^ modx(a0, b3);
	
		next_blk[i] = d0;
		next_blk[i + 1] = d1;
		next_blk[i + 2] = d2;
		next_blk[i + 3] = d3;

	}

}







// static void mixColumns(uint32_t *cblk)
// {
// 	register uint32_t i, a0=0x02, a1=0x01, a2=0x01, a3=0x03;

// 	uint32_t b0, b1, b2, b3, d0, d1, d2, d3;

// 	for (i = 0; i < 4; i += 4) {

// 		b0 = upk(cblk[i], 0);
// 		b1 = upk(cblk[i], 1);
// 		b2 = upk(cblk[i], 2);
// 		b3 = upk(cblk[i], 3);

// 		d0 = modx(a0, b0) ^ modx(a3, b1) ^ modx(a2, b2) ^ modx(a1, b3);
// 		d1 = modx(a1, b0) ^ modx(a0, b1) ^ modx(a3, b2) ^ modx(a2, b3);
// 		d2 = modx(a2, b0) ^ modx(a1, b1) ^ modx(a0, b2) ^ modx(a3, b3);
// 		d3 = modx(a3, b0) ^ modx(a2, b1) ^ modx(a1, b2) ^ modx(a0, b3);
	
// 		cblk[i] = pck(d0, d1, d2, d3); 

// 	}

// }







static void expandKey(uint32_t *enckey, uint32_t *expkey, int nk) 
{
	int i, j, r, word=0x04, nb=0x04, nr=0x0a; 

	uint32_t temp[4];


	for ( i=0 ; i < nk ; i++) {

		for ( j=0 ; j < word ; j++ ) {

			expkey[i*nk + j] = enckey[i*nk + j]; 

		}

	}


	for ( i=nk ; i < nb*(nr+1) ; i++) {

		temp[0] = expkey[(i-1)*nk+0];
		temp[1] = expkey[(i-1)*nk+1];
		temp[2] = expkey[(i-1)*nk+2];
		temp[3] = expkey[(i-1)*nk+3];

		if (i % nk == 0) {

			r = temp[0];
			temp[0] = temp[1];
			temp[1] = temp[2];
			temp[2] = temp[3];
			temp[3] = r;

			temp[0] = sbox[temp[0]];
			temp[1] = sbox[temp[1]];
			temp[2] = sbox[temp[2]];
			temp[3] = sbox[temp[3]];

			temp[0] ^= rc[i/nk];
			temp[1] ^= 0x00;
			temp[2] ^= 0x00;
			temp[3] ^= 0x00;

		}

		expkey[i*nk+0] = expkey[(i-nk)*nk+0] ^ temp[0];
		expkey[i*nk+1] = expkey[(i-nk)*nk+1] ^ temp[1];
		expkey[i*nk+2] = expkey[(i-nk)*nk+2] ^ temp[2];
		expkey[i*nk+3] = expkey[(i-nk)*nk+3] ^ temp[3];


	}



} 





// static void expandKey(uint32_t *enckey, uint32_t *expkey, int nk) 
// {
// 	register uint32_t i, r, nb=0x04, nr=0x0a; 

// 	uint32_t temp[4], word;


// 	for(i=0; i < nk; i++) {

// 		expkey[i] = pck(enckey[i*nk+0], enckey[i*nk+1], enckey[i*nk+2], enckey[i*nk+3]); 
// 		// expkey[i] = enckey[i]; 

// 	}


// 	for(i=nk; i < nb * (nr + 1); i++) {

// 		word = expkey[i-1];

// 		if (i % Nk = 0) {

// 			temp[0] = upk(expkey[i-1], 0);
// 			temp[1] = upk(expkey[i-1], 1);
// 			temp[2] = upk(expkey[i-1], 2);
// 			temp[3] = upk(expkey[i-1], 3);

// 			r = temp[0];
// 			temp[0] = temp[1];
// 			temp[1] = temp[2];
// 			temp[2] = temp[3];
// 			temp[3] = r;

// 			temp[0] = sbox[temp[0]];
// 			temp[1] = sbox[temp[1]];
// 			temp[2] = sbox[temp[2]];
// 			temp[3] = sbox[temp[3]];

// 			temp[0] ^= rc[i/nk];
// 			temp[1] ^= 0x00;
// 			temp[2] ^= 0x00;
// 			temp[3] ^= 0x00;

// 			word = pck(temp[0], temp[1], temp[02, temp[3]);

// 		}

// 		expkey[i] = expkey[i-nk] ^ word;

// 	}



// }




/* -------------------------------------------------------------------------- */
static void aes_expandEncKey(uint32_t *k, uint32_t rc)
{
	register uint32_t i;

	k[0] ^= rj_sbox(k[29]) ^ rc;
	k[1] ^= rj_sbox(k[30]);
	k[2] ^= rj_sbox(k[31]);
	k[3] ^= rj_sbox(k[28]);

	for(i = 4; i < 16; i += 4) {
		k[i]     ^= k[i - 4];
		k[i + 1] ^= k[i - 3];
		k[i + 2] ^= k[i - 2];
		k[i + 3] ^= k[i - 1];
	}
	
	k[16] ^= rj_sbox(k[12]);
	k[17] ^= rj_sbox(k[13]);
	k[18] ^= rj_sbox(k[14]);
	k[19] ^= rj_sbox(k[15]);

	for(i = 20; i < 32; i += 4) {
		k[i]     ^= k[i - 4];
		k[i + 1] ^= k[i - 3];
		k[i + 2] ^= k[i - 2];
		k[i + 3] ^= k[i - 1];
	}

} 



uint32_t *aes128_setExpKey(aes_context *ctx, uint32_t *enckey)
{
	int i, j;

	for (i = 0; i < sizeof(ctx->enckey); i++)
		ctx->enckey[i] = enckey[i];

	expandKey(ctx->enckey, ctx->expkey, 4);

	return 0;

} 




void aes128_done(aes_context *ctx)
{
	int i;

	for (i = 0; i < sizeof(ctx->enckey); i++) {
		ctx->enckey[i] = 0;
		ctx->blk.nonce[i % sizeof(ctx->blk.nonce)] = 0;
		ctx->blk.iv[i % sizeof(ctx->blk.iv)] = 0;
		ctx->blk.ctr[i % sizeof(ctx->blk.ctr)] = 0;
	}

} /* aes256_done */






void aes_encrypt_next_block(aes_context *ctx, uint32_t *next_blk)
{
	int i, j, nr=0x0a;
	uint32_t *next_ctr_blk = (uint32_t *)&ctx->blk;

	// printf("%s \n\t", "Key:");
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", ctx->enckey[0*16+j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	

	// printf("%s\n\t", "ctr block: before AddRoundKey()");
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	

	addRoundKey(next_blk, ctx->expkey, 0);
	// printf("%s %u\n\t", "Starting ROUND KEY >", i);
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", ctx->expkey[0*16+j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	

	// printf("%s\n\t", "ctr block: after AddRoundKey()");
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	


	for(i = 1; i < nr; ++i) {

		// printf("%s %u\n\t", "Start of Round >", i);
		// for (j = 0; j < sizeof(ctx->blk); j++)
		// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
		// printf("\n ");	



		aes_subBytes(next_blk);
		// printf("%s\n\t", "ctr block: after SubBytes()");
		// for (j = 0; j < sizeof(ctx->blk); j++)
		// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
		// printf("\n ");	



		aes_shiftRows(next_blk);
		// printf("%s\n\t", "ctr block: after ShiftRows()");
		// for (j = 0; j < sizeof(ctx->blk); j++)
		// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
		// printf("\n ");	


		mixColumns(next_blk);
		// printf("%s\n\t", "ctr block: after mixColumns()");
		// for (j = 0; j < sizeof(ctx->blk); j++)
		// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
		// printf("\n ");	


		addRoundKey(next_blk, ctx->expkey, i);
		// printf("%s %u\n\t", "ROUND KEY for round >", i);
		// for (j = 0; j < sizeof(ctx->blk); j++)
		// 	printf("%02x%s", ctx->expkey[i*16+j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
		// printf("\n ");	

		// printf("%s %u\n\t", "COUNTER BLK for round >", i);
		// for (j = 0; j < sizeof(ctx->blk); j++)
		// 	printf("%02x%s", next_ctr_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
		// printf("\n ");	



	}

	// printf("%s %u\n\t", "COUNTER BLK for this encrypt block >", i);
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", next_ctr_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	

	// printf("%s\n\t", "LAST ROUND:");
	// // for (j = 0; j < sizeof(ctx->blk); j++)
	// // 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	


	// printf("%s\n\t", "ctr block: start of last round");
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	


	aes_subBytes(next_blk);
	// printf("%s\n\t", "ctr block: after SubBytes()");
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	


	aes_shiftRows(next_blk);
	// printf("%s\n\t", "ctr block: after ShiftRows()");
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	

	// printf("%s %u\n\t", "last ROUND KEY >", i);
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", ctx->expkey[i*16+j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	

	addRoundKey(next_blk, ctx->expkey, i);
	// printf("%s\n\t", "ctr block: after last addRoundKey()");
	// for (j = 0; j < sizeof(ctx->blk); j++)
	// 	printf("%02x%s", next_blk[j], ((j % 16 == 15) && (j < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
	// printf("\n ");	


}



void aes128_offsetCtr(uint32_t *val, int page, int nBlks)
{

	int offset = (page-1)*nBlks;

	for(int i=0; i < offset; i++) {

		ctr_inc_ctr(val);

	} 

} 



uint32_t ctr_inc_ctr_uint32_0(uint32_t *ctr)
{

	uint32_t value_before_inc = ctr[0];

	if ( ++ctr[0] > 255 ) { 		

		ctr[0] = 0;

	}

	return value_before_inc;


} 



uint32_t ctr_inc_ctr_uint32_1(uint32_t *ctr)
{

	uint32_t value_before_inc = ctr[1];

	if ( ++ctr[1] > 255 ) { 		

		ctr[1] = 0;

	}

	return value_before_inc;


} 



uint32_t ctr_inc_ctr_uint32_2(uint32_t *ctr)
{

	uint32_t value_before_inc = ctr[2];

	if ( ++ctr[2] > 255 ) { 		

		ctr[2] = 0;

	}

	return value_before_inc;


} 



uint32_t ctr_inc_ctr_uint32_3(uint32_t *ctr)
{

	uint32_t value_before_inc = ctr[3];

	if ( ++ctr[3] > 255 ) { 		

		ctr[3] = 0;

	}

	return value_before_inc;


} 



static void ctr_inc_ctr(uint32_t *val)
{
	if ( val != NULL )
		if ( ++val[3] == 0 )
			if ( ++val[2] == 0 )
				if ( ++val[1] == 0 )
					val[0]++;

} 



static void get_next_blk_in_keystream(aes_context *ctx, uint32_t *next_blk)
{
	int i, j;
	uint32_t *next_ctr_blk = (uint32_t *)&ctx->blk;


	if ( (ctx != NULL) && (next_blk != NULL) ) {
		for (i = 0; i < sizeof(ctx->blk); i++)
			next_blk[i] = next_ctr_blk[i];

	// printf("Counter[3] BEFORE increment:%d", &ctx->blk.ctr[3]);
	// printf("Counter[2] BEFORE increment:%d", &ctx->blk.ctr[2]);

		aes_encrypt_next_block(ctx, next_blk);

		if(ctr_inc_ctr_uint32_3(&ctx->blk.ctr) > 254) {

			if(ctr_inc_ctr_uint32_2(&ctx->blk.ctr) > 254) {

				if(ctr_inc_ctr_uint32_1(&ctx->blk.ctr) > 254) {

					ctr_inc_ctr_uint32_0(&ctx->blk.ctr);

				}

			}
			
		}

		// ctr_inc_ctr_uint32(&ctx->blk.ctr);
		// ctr_inc_ctr(&ctx->blk.ctr[0]);
		
	}



} 





uint32_t *aes128_setCtrBlk(aes_context *ctx, rfc3686_blk *blk)
{
	uint32_t *p = (uint32_t *)&ctx->blk, *v = (uint32_t *)blk;
 	int i;
	if ( (ctx != NULL) && (blk != NULL) )
		for (i = 0; i < sizeof(ctx->blk); i++)
			p[i] = v[i];

	size_t j;

	return p;

} 




void dumpData(char *s, uint32_t *buf, size_t sz)
{
	// sz = 64;
	int offset = 0;	
	size_t i;

	printf("%s\n\t", s);
	for (i = offset; i < offset + sz; i++) {
		printf("%02x%s", buf[i], ((i % 16 == 15) && (i < (offset + sz) - 1)) ? "\n\t" : " ");
	}
	printf("\n ");

} 





void dump(char *s, uint32_t *buf, size_t sz)
{
	sz = 64;
	int offset = 0;		
	// int offset = 768;	
	// int offset = 448;
	size_t i;

	printf("%s\n\t", s);
	for (i = offset; i < offset + sz; i++) {
		printf("%02x%s", buf[i], ((i % 16 == 15) && (i < (offset + sz) - 1)) ? "\n\t" : " ");
	}
	printf("\n ");

} 






uint32_t *aes128_encrypt_progressive_data(aes_context *ctx, char **row_data, int w, int h, bool env)
{
	png_bytep row;
	int BYTES_PER_PIXEL = 3;
	int row_bytes = w * BYTES_PER_PIXEL;
	int sz = w*h;
	int  i, k, l;
	uint32_t next_blk[sizeof(ctx->blk)];
	size_t j = sizeof(ctx->blk);
	bool isGL2 = true; //env;


	for(k=0; k < h; k++) {

		// row = *row_pointers;
		// row = row_pointers[k];
		// row_pointers++;

				printf("%s%u\n\t", "before encrypt row: ", k);
				for (l = 0; l < 64; l++)
					printf("%02x%s", row_data[k][l], ((l % 16 == 15)/* && (l < 64 - 1)*/) ? "\n\t" : " ");
				printf("\n ");			

		for (i = 0; i < 128 /*row_bytes*/; i++) {

			if ( j == sizeof(ctx->blk) ) {
				j = 0;
				get_next_blk_in_keystream(ctx, next_blk);

				// printf("%s%u\n\t", "aes128 encrypt byte #", i);

				// for (l = 0; l < sizeof(ctx->blk); l++)
				// 	printf("%02x%s", next_blk[l], ((j % 16 == 15) && (l < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
				// printf("\n ");	

				// for (l = 0; l < sizeof(ctx->blk.ctr); l++)
				// 	printf("%02x%s", ctx->blk.ctr[l], ((j % 16 == 15) && (l < sizeof(ctx->blk.ctr) - 1)) ? "\n\t" : " ");				
				// printf("\n ");	


			} 
			else
			{
				j = j + 1;
			} 

			if(isGL2) {

				row_data[k][i] = next_blk[j];
						// row[i] = next_blk[j++];

			}else{

				row_data[k][i] ^= next_blk[j];
						// row[i] ^= next_blk[j++];

			}

		}
				// printf("%s%u\n\t", "after encrypt row: ", k);
				// for (l = 0; l < 64; l++)
				// 	printf("%02x%s", row[l], ((j % 16 == 15) && (l < 64 - 1)) ? "\n\t" : " ");
				// printf("\n ");					

	}

	return 1;

} 



uint32_t *aes128_encrypt_progressive(aes_context *ctx, png_bytep *row_pointers, int row_bytes, int w, int h, bool env)
{
	png_bytep row;
	// int BYTES_PER_PIXEL = 3; //4;
	// int row_bytes = w * BYTES_PER_PIXEL;
	int sz = w*h;
	int  i, k, l;
	uint32_t next_blk[sizeof(ctx->blk)];
	size_t j = sizeof(ctx->blk);
	bool isGL2 = false; //env;


	for(k=0; k < h; k++) {

		// row = *row_pointers;
		row = row_pointers[k];
		// row_pointers++;

				// printf("%s%u\n\t", "before encrypt row: ", k);
				// for (l = 0; l < 64; l++)
				// 	printf("%02x%s", row[l], ((l % 16 == 15)/* && (l < 64 - 1)*/) ? "\n\t" : " ");
				// printf("\n ");			

		for (i = 0; i < row_bytes; i++) {

			if ( j == sizeof(ctx->blk) ) {
				j = 0;
				get_next_blk_in_keystream(ctx, next_blk);

				// printf("%s%u\n\t", "aes128 encrypt byte #", i);

				// for (l = 0; l < sizeof(ctx->blk); l++)
				// 	printf("%02x%s", next_blk[l], ((j % 16 == 15) && (l < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
				// printf("\n ");	

				// for (l = 0; l < sizeof(ctx->blk.ctr); l++)
				// 	printf("%02x%s", ctx->blk.ctr[l], ((j % 16 == 15) && (l < sizeof(ctx->blk.ctr) - 1)) ? "\n\t" : " ");				
				// printf("\n ");	


			} 
			else
			{
				j = j + 1;
			} 

			if(isGL2) {

				row[i] = next_blk[j];
						// row[i] = next_blk[j++];

			}else{

				row[i] ^= next_blk[j];
						// row[i] ^= next_blk[j++];

			}

		}
				// printf("%s%u\n\t", "after encrypt row: ", k);
				// for (l = 0; l < 64; l++)
				// 	printf("%02x%s", row[l], ((l % 16 == 15) /*&& (l < 64 - 1)*/) ? "\n\t" : " ");
				// printf("\n ");					

	}

	return 1;

} 



uint32_t *aes128_encrypt(aes_context *ctx, uint32_t *texture, int sz, bool env, uint32_t *kstr)
{
	int  i, k;
	uint32_t next_blk[sizeof(ctx->blk)];
	size_t j = sizeof(ctx->blk);
	bool isGL2 = true; //env;

	for (i = 0; i < sz; i++) {

		if ( j == sizeof(ctx->blk) ) {
			j = 0;
			get_next_blk_in_keystream(ctx, next_blk);

			// printf("%s%u\n\t", "aes128 encrypt byte #", i);
			// for (k = 0; k < sizeof(ctx->blk); k++)
			// 	printf("%02x%s", next_blk[k], ((j % 16 == 15) && (k < sizeof(ctx->blk) - 1)) ? "\n\t" : " ");
			// printf("\n ");	

		}

		if(isGL2) {

			kstr[i] = next_blk[j];
			texture[i] = next_blk[j++];

		}else{

			kstr[i] = next_blk[j];
			texture[i] ^= next_blk[j++];

		}




	}

	return 1;

} 


