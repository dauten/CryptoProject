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
#include "crypto.h"

//this file defines the "mounting/unmounting" of a virtual file system.
//that is, when the disk is mounted it takes as input a FS and creates the unencrypted version
//unmounting does the reverse, taking raw FS and encrypting it

int zerosize(int fd){
  struct stat stats;
  fstat(fd, &stats);
  if(stats.st_size == 0)
    return 1;
  return 0;
}


// read from file described by fd, starting at start and going to end.  returns it as a bit string
void mountRange(int fd, int start, int end, int mode, int dir, char* name){
  int newfs;
  void * data = malloc(end-start);
  //seek to start bit then read to end
  if (lseek(fd, start, SEEK_SET) == -1){
    perror("seek failed");
    exit(EXIT_FAILURE);
  }
  else{
    read(fd, data, (end-start));
  }
  void * temp = code(data, mode, dir, end-start);


  if ((fd = open(name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1){
    perror("open failed");
    exit(EXIT_FAILURE);
  }


  write(fd, temp, end-start);
}
