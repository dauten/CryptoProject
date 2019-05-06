#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#include <errno.h>

#include <sys/mman.h>

#include "fs.h"
#include "crypto.h"
#include "mounter.h"


void exitusage(char* pname);


int main(int argc, char** argv){

  int opt;
  int create = 0;
  int list = 0;
  int add = 0;
  int remove = 0;
  int extract = 0;
  char* toadd = NULL;
  char* toremove = NULL;
  char* toextract = NULL;
  char* fsname = NULL;
  int fd = -1;
  int newfs = 0;
  int filefsname = 0;
  int debug = 0;
  int mode = 1;
  int otf = 0;
  int mount = 0;
  int umount = 0;
  char* mpoint = NULL;

  //parse command line arguments to set necessary variables
  while ((opt = getopt(argc, argv, "lhdmon:u:a:r:e:f:")) != -1) {
    switch (opt) {
    case 'l':
      list = 1;
      break;
    case 'a':
      add = 1;
      toadd = strdup(optarg);
      break;
    case 'r':
      remove = 1;
      toremove = strdup(optarg);
      break;
    case 'f':
      filefsname = 1;
      fsname = strdup(optarg);
      break;
    case 'e':
      extract = 1;
      toextract = strdup(optarg);
      break;
    case 'd':
      debug = 1;
    break;
    case 'm':
      mode = 2;
    break;
    case 'h':

    break;
    case 'o':
      otf = 1;  //if set, use full disk instead of on the fly encryptions
    break;
    case 'n':
      mount = 1;  //if set, use full disk instead of on the fly encryptions
      mpoint = strdup(optarg);
    break;
    case 'u':
      umount = 1;
      mpoint = strdup(optarg);
    break;

    default:
      exitusage(argv[0]);
    }
  }


  //exit if we weren't given an FS to work on
  if (!filefsname){
    exitusage(argv[0]);
  }

  //open FS, making a new one if none exists with that name
  if ((fd = open(fsname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1){
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  else{
    if (zerosize(fd)){
      newfs = 1;
    }

    if (newfs)
      if (lseek(fd, FSSIZE-1, SEEK_SET) == -1){
      	perror("seek failed");
      	exit(EXIT_FAILURE);
      }
      else{
        if(write(fd, "\0", 1) == -1){
          perror("write failed");
          exit(EXIT_FAILURE);
        }

      }
  }
  mapfs(fd);

  //mount or unmount it
  if(mount){

    mountRange(fd, 0, FSSIZE, 0, 0, mpoint);

    return 0;
  }
  else if(umount){

    mountRange(fd, 0, FSSIZE, 0, 1, mpoint);

    return 0;
  }
  //format FS (add superblock, FBL, etc)
  if (newfs){
    formatfs(fd);
  }
  //add file
  if (add){
    addfilefs(toadd, fd, mode, otf);
  }
  //etc
  if (remove){
    removefilefs(toremove, fd);
  }
  if (extract){
    extractfilefs(toextract, fd, mode, otf);
  }
  if(list){
    lsfs(fd);
  }
  if(debug){
    meta(fd);
  }

  unmapfs();

  return 0;
}

//print usage if arguments fail
void exitusage(char* pname){
  fprintf(stderr, "Usage:\n%s -a [file to add] -e [file to print] -r [file to delete] -l[to list structure] -m[to use CBC instead of the default EBC] -d[for debugging info] -f <filesystem to use>", pname);
  exit(EXIT_FAILURE);
}
