#ifndef __FS_H__
#define __FS_H__
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define FSSIZE 10000000

unsigned char* fs;

void mapfs(int fd);
void unmapfs();
void formatfs();
void loadfs();
void lsfs(int fd);
void addfilefs(char* fname, int fd);
void removefilefs(char* fname, int fd);
void extractfilefs(char* fname, int fd);
void meta(int fd);

#endif
