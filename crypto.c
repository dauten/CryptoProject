/**
*
* Description: Program that handles cryptological aspects of the Disk Encryption
*
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


/*
* Given a 128 or less bit block of binary data, run it through AES with the given key.  key must be 128 bits
*/
void * crypt(void * data, char * key){



}
