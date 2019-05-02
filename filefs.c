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

int zerosize(int fd);
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
  int disk = 0;

  while ((opt = getopt(argc, argv, "lhdmoka:r:e:f:")) != -1) {
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
      printf("Usage:\n./filefs -a [file to add] -e [file to print] -r [file to delete] -l[to list structure] -m[to use CBC instead of the default EBC] -d[for debugging info] -f <filesystem to use>");
    break;
    case 'o':
      otf = 1;  //if set, use full disk instead of on the fly encryptions
    break;
    case 'k':
      disk = 1;  //if set, use full disk instead of on the fly encryptions
    break;

    default:
      exitusage(argv[0]);
    }
  }


  if (!filefsname){
    exitusage(argv[0]);
  }

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

  if (newfs){
    formatfs(fd);
  }

  if (add){
    addfilefs(toadd, fd, mode, otf);
  }

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


int zerosize(int fd){
  struct stat stats;
  fstat(fd, &stats);
  if(stats.st_size == 0)
    return 1;
  return 0;
}

void exitusage(char* pname){
  fprintf(stderr, "Usage %s [-l] [-a path] [-e path] [-r path] -f name\n", pname);
  exit(EXIT_FAILURE);
}
