#ifndef __MOUNTER_H__
#define __MOUNTER_H__
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "crypto.h"

void mountRange(int fd, int start, int end, int mode, int dir, char* name);
int zerosize(int fd);
#endif
