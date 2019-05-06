/**
* Description: Program that handles cryptographic aspects of the Disk Encryption
*/

#include "fs.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#include <errno.h>

#include <sys/mman.h>
#include "aes.h"

#define CBC 1


static void phex(uint8_t* str)
{
  uint8_t len = 16;

  unsigned char i;
  for (i = 0; i < len; ++i)
      printf("%.2x", str[i]);
  printf("\n");
}



/*
* Given a block of binary data, a key, and which chaining mode of operation to use,
* En/Decrypts that data using AES
* if mode==1, ECB else CBC
* if crypt==1, encrypt else decrypt
* length is size of data list
*/
void * code(void * data, int mode, int crypt, int length){
  uint8_t i;
  uint8_t key[16] = { (uint8_t) 0x2b, (uint8_t) 0x7e, (uint8_t) 0x15, (uint8_t) 0x16, (uint8_t) 0x28, (uint8_t) 0xae, (uint8_t) 0xd2, (uint8_t) 0xa6, (uint8_t) 0xab, (uint8_t) 0xf7, (uint8_t) 0x15, (uint8_t) 0x88, (uint8_t) 0x09, (uint8_t) 0xcf, (uint8_t) 0x4f, (uint8_t) 0x3c };

  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, key);

  if(mode == 1){
    if(crypt == 1){
      for (i = 0; i < length/16; ++i)
      {
        AES_ECB_encrypt(&ctx, data + (i * 16));
      }
    }else{
      for (i = 0; i < length/16; ++i)
      {
        AES_ECB_decrypt(&ctx, data + (i * 16));
      }
    }
  }else{
    if(crypt == 1){
      AES_CBC_encrypt_buffer(&ctx, data, length);
    }
    else{
      AES_CBC_decrypt_buffer(&ctx, data, length);
    }

  }

  return data;

}
