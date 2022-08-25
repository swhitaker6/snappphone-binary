/*
*   A byte-oriented AES-256-CTR implementation.
*   Based on the code available at http://www.literatecode.com/aes256
*   Complies with RFC3686, http://tools.ietf.org/html/rfc3686
*
*/
#ifndef bool
#include <stdbool.h>
#endif
#ifndef uint16_t
#include <stdint.h>
#endif
#ifndef uint32_t
#define uint32_t  unsigned char
#endif
#ifndef size_t
#include <stddef.h>
#include <stdio.h>
#endif
#include "png.h"

#define aes128_decrypt  aes128_encrypt

#ifdef __cplusplus
extern "C" {
#endif



typedef struct {
	uint32_t nonce[4];
	uint32_t iv[8];
	uint32_t ctr[4];
} rfc3686_blk;


typedef struct {
	// uint32_t key[16];
	uint32_t enckey[16];
	uint32_t expkey[176];
	rfc3686_blk blk;
} aes_context;


void gen_table_mod_x (uint32_t b);
void dump(char *s, uint32_t *buf, size_t sz);
void dumpData(char *s, uint32_t *buf, size_t sz);
uint32_t *aes128_setExpKey(aes_context *ctx, uint32_t *key);
void aes128_done(aes_context *ctx);
void aes_encrypt_ctr_block(aes_context *ctx, uint32_t *buf);

void aes128_offsetCtr(uint32_t *val, int page, int nBlks);
uint32_t *aes128_setCtrBlk(aes_context *ctx, rfc3686_blk *blk);
uint32_t *aes128_encrypt_progressive_data(aes_context *ctx, char **row_data, int w, int h, bool env);
uint32_t *aes128_encrypt_progressive(aes_context *ctx, png_bytep *row_pointers, int row_bytes, int w, int h, bool env);
uint32_t *aes128_encrypt(aes_context *ctx, uint32_t *text, int sz, bool env, uint32_t *kstr);
// uint32_t *aes128_encrypt(aes_context *ctx, uint32_t *text, uint32_t *kstr, size_t sz);

#ifdef __cplusplus
}
#endif
