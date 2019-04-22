#ifndef __CRYPTO_H__
#define __CRYPTO_H__
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


static void phex(u_int8_t* str);
void * code(void * data, int mode, int crypt, int length);

#endif
