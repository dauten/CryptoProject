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

struct block{
  long numb;
  long size;
  int content[496];
};

struct inode{
  long numb;
  long type; //file or dir
  //if type = 1 (file) then size will be number of blocks this has and
  //if type = 2 (directory) then size will be number of items and content will be location of other inodes
  long size; //we dont need a long but its same type as stat.size returns
  long inuse;
  char name[100];
  short int content[100];
};

struct dir{
  char name[256]; //255 chars plus terminator
  int inodeNum;
};

struct superblock{
  int numOfBlocks;
  int numOfInodes;
  int sizeOfInodes;
};

struct bitmap{

};

void mapfs(int fd){
  if ((fs = mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) == NULL){
      perror("mmap failed");
      exit(EXIT_FAILURE);
  }
}




//print binary data file to stdout in hex
void out(void * file, int length){
  int j;
  for(j = 0; j < length; ++j)
    printf("%02x", ((u_int8_t*) file)[j]);
}

//print binary data file to stdout as raw
void dump(void * file, int length){
  int j;
  for(j = 0; j < length; ++j)
    printf("%c", ((char*) file)[j]);
}

//read an inode, printing content
void checkInode(struct inode * I){

  for(int i = 0; i < I->size; i++){
    printf("%d\n", I->content[i]);
  }
}

// read from file described by fd, starting at start and going to end.  returns it as a bit string
void * readRange(int fd, int start, int end){
  void * data = malloc(end-start);
  //seek to start bit then read to end
  if (lseek(fd, start, SEEK_SET) == -1){
    perror("seek failed");
    exit(EXIT_FAILURE);
  }
  else{
    read(fd, data, (end-start));
  }

  return data;
}

//given arbitrary data write that to start and end locations
void writeRange(int fd, void * data, int start, int end){
  //seek to start bit then write to end
  if (lseek(fd, start, SEEK_SET) == -1){
    perror("seek failed");
    exit(EXIT_FAILURE);
  }
  else{
    write(fd, data, end-start);
  }
}


void unmapfs(){
  munmap(fs, FSSIZE);
}

/*
* Instantiate filesystem
*/
void formatfs(int fd){
  printf("formatting FS\n");
  //installs superblock and bitmap in FS
  struct superblock *M=malloc(sizeof(struct superblock));
  M->numOfBlocks = 10000;
  M->numOfInodes = 10000;
  M->sizeOfInodes = sizeof(struct inode);

  writeRange(fd, M, 0, 12);

  //make root directory
  int i_zero = sizeof(struct superblock) + M->numOfBlocks/8; // first/0th inode
  //search for first unfilled inode and put this there.
  struct inode * temp = malloc(sizeof(struct inode));
  temp->type = 2;
  temp->name[0] = ' ';
  temp->inuse = 1;
  writeRange(fd, temp, i_zero, i_zero + sizeof(struct inode));

}


//given the free block list return the integer correspondng to the first free block
//and update FBL so that bit is no longer 0
int freeBlockSearch(void * FBL){
  int j;
  int r = 0;
  int length = 1250;

  //iterate through the FBL searching fot the first 0 bit.
  for(j = 0; j < length; ++j){
    //if a given byte is 0xFF then its all ones, skip to next byte
    if( 0xFF == ((int*) FBL)[j]);
    else{
      int backup = j;
      //else it has at least one 0, at least one free block.
      j = ((int*) FBL)[j];

      //while the first byte is one, we count how many left shits it takes to find 0
      while( (j & 0x80 ) != 0 ){
        r++;
        j <<= 1;
      }

      ((int *)FBL)[backup] |= 0x80>>(r%8);  //update FBL with bitwise ORing it with string where corresponging free bit is 1

      //return number of bits from left
      return r;
    }
    //in for loop if a byte has no 0's we count up eight, that entire byte
    r += 8;
  }
  //failed state
  return -1;
}

//return int address of given block
int blockAddress(short int block, int fd){
  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int start = sizeof(struct superblock) + N->numOfBlocks/8; //beginning of inodes
  start += N->numOfInodes * N->sizeOfInodes;

  return start + block*512;

}

//creates in fd an empty directory with given path
//"mkdir' was taken :(
struct inode * makeDir(struct inode * parent, int fd, char name[]){

  //find and allocate an inode directory with name names
  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode
  int I = i_zero;
  //search for first unfilled inode and put this there.
  struct inode * temp = malloc(sizeof(struct inode));
  struct inode * next = malloc(sizeof(struct inode));
  next->numb = -1;
  strcpy(next->name, name);
  next->type = 2;

  do{
    //read one inode at a time until we find a free one, keeping track of that inode's number
    temp = readRange(fd, i_zero, i_zero+sizeof(struct inode));
    next->numb++;

    //once we find our free inode...
    if(temp->inuse == 0){
      //add that to parent.content
      parent->content[parent->size] = next->numb;
      next->inuse = 1;
      parent->size++;

      //rewrite parent to update and write the chosen child inode
      writeRange(fd, parent, I + sizeof(struct inode)*parent->numb, I + sizeof(struct inode)*(parent->numb+1));
      writeRange(fd, next, I + sizeof(struct inode)*next->numb, I + sizeof(struct inode)*(next->numb+1));

      return next;
    }
    //update index
    i_zero += sizeof(struct inode);
  }
  while(temp->inuse == 1);

}


//given the inode for a file, reconstruct file and out() it
void readFile(struct inode * F, int fd, int mode, int otf){

  //iterate through every block
  for(int i = 0; i < F->size; i++){

    //get location in file where that block starts
    int addr = blockAddress(F->content[i], fd);

    //read that block
    struct block * B = malloc(sizeof(struct block));
    B = readRange(fd, addr, addr+512);


    //extract specifically the data from that block
    void * data = readRange(fd, addr+16, addr+B->size);
    if(otf){
      void * O = code(B->content, mode, 0, B->size);
      //dumps raw data to stdout
      dump(O, B->size);
    }
    else{
      //dumps raw data to stdout
      dump(B->content, B->size);
    }


  }
}

//given an array of directory names ending in filename, searches for that file's inode
struct inode * getInodeByNumber(short int numb, int fd){
  struct inode * I;


  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode

  int i = -1;
  struct inode * temp = malloc(sizeof(struct inode));
  do{
    //count up until we calculate location of this inode
    i_zero+=sizeof(struct inode);
    i++;

  }
  while(i != numb);

  //read that Inode
  I = readRange(fd, i_zero, i_zero+sizeof(struct inode));

  if(I->inuse == 0){
    printf("This is so emberassing.  That led to an unused inode so that file may have been removed and the FS didn't get properly cleaned\n");
    exit(1);
  }

  return I;
}


//given an inode print that inode and all items below it
void printTree(struct inode * I, int fd, int depth){

  //print this particular inode
  for(int i = 0; i < depth; i++){
    printf("  ");
  }
  printf("%s", I->name);
  if(I->type == 2){
    printf("/");
  }
  printf("\n");

  //iterate through all items below this inode
  for(int i = 0; i < I->size; i++){
    //gets every number in I->content and calls this function
    //recursively on its inode
    struct inode * temp = getInodeByNumber(I->content[i], fd);

    //if that item is a directory, recurse.  Else print the file info
    if(temp->type == 2)
      printTree(temp, fd, depth + 1);
    else{
      for(int i = 0; i <= depth+1; i++){
        printf("  ");
      }
      printf("%s\n", temp->name);
    }

  }
}


/*
* Get root inode and call printTree on it, printing it and everything beneath it
*/
void lsfs(int fd){
  //get first inode and printTree it
  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode

  //search for first unfilled inode and put this there.
  struct inode * temp = malloc(sizeof(struct inode));
  temp = readRange(fd, i_zero, i_zero+sizeof(struct inode));  //get root inode

  printTree(temp, fd, 0);

}



//given a string pathname, converts it to array of string correspondng to component
//files and directories (ie '/root/home/dale/sample.txt' becomes
//['root', 'home', 'dale', 'sample.txt'])
//returns number of components, 4 in above case
int pathNameConvert(char * file, char * path[], int length){
  int i = 0;
  char * pch;
  length--;
  pch = strtok (file,"/");
  while (pch != NULL)
  {
    path[i] = pch;
    i++;
    pch = strtok (NULL, "/");
  }

  return i-1;
}

int pathLength(char * file){
  int len = strlen(file);
  int count = 1;
  for(int i = 0; i < len; i++){
    if(file[i] == '/')
      count++;
  }
  return count;
}

/*
* Grab free block list
*/
void * readFBL(int fd){
  //get location of FBL
  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int temp = N->numOfBlocks/8;
  void * FBL = malloc(temp);

  //read that location
  FBL = readRange(fd, sizeof(struct superblock), sizeof(struct superblock)+temp);

  return FBL;
}

/*
* Rewrite free block list
*/
void updateFBL(void * FBL, int fd){
  //get location of FBL
  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int temp = N->numOfBlocks/8;

  //write given data to FBL
  writeRange(fd, FBL, sizeof(struct superblock), sizeof(struct superblock)+temp);
}


/*
* Write a given inode given its absolute path, updating parent directory
*/
void addInode(struct inode * I, char * path[], int fd, int len){

  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode

  //search for first unfilled inode and put this there.
  struct inode * temp = malloc(sizeof(struct inode));
  temp = readRange(fd, i_zero, i_zero+sizeof(struct inode));  //get root inode

  struct inode * temp2 = malloc(sizeof(struct inode));

  int curEl = 0;
  int exists = 0;

  //starting at first inode
  do{

    exists = 0;
    //for every item in this directory, check if that item is next part of path
    for(int i = 0; i < temp->size; i++){

      //grab that child inode of parent
      temp2 = getInodeByNumber(temp->content[i], fd);
      //check if that child is next directory on our path.
      if(strcmp(temp2->name, path[curEl]) == 0){
        i = temp->size;
        exists = 1;
      }
    }

    //if none of the files were next thing on path we ned to make it
    if(!exists){
      temp2 = makeDir(temp, fd, path[curEl]);
    }


    curEl++;

    temp = temp2;
  }
  while(strcmp(temp->name, path[len-3]) != 0); //while inode is a directory

  //at this point we have inode to add and we searched for its parent inode

  //create file like we used to only update temp to add it's number to temp.content
  i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode
  do{
    //search for zone we can write inode
    temp = readRange(fd, i_zero, i_zero+sizeof(struct inode));
    I->numb++;

    //once that zone if found...
    if(temp->inuse == 0){
      //write child
      writeRange(fd, I, i_zero, i_zero+sizeof(struct inode));
      i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode

      //update temp2->content (the parent) and write
      temp2->content[temp2->size] = I->numb;
      temp2->size++;
      writeRange(fd, temp2, i_zero + sizeof(struct inode)*temp2->numb, i_zero + sizeof(struct inode)*(temp2->numb+1));

      return;
    }
    i_zero += sizeof(struct inode);
  }
  while(temp->inuse == 1);




}

/*
* Given a filename construct inode for that and write its content to data blocks refered to by that inode
*/
void addfilefs(char* fname, int fd, int mode, int otf){
  int in;
  if ((in = open(fname, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) == -1){
    perror("open failed");
    exit(EXIT_FAILURE);
  }
  else{
    //get size of file so we know how much to write
    struct stat st;
    fstat(in, &st);
    long size = st.st_size;
    void * d;
    void * FBL = readFBL(fd);

    //we don't have indirect block pointer yet
    if(size > 49600){
      printf("oops the indirect block pointer hasn't been implemented and that file looks like its too big!\n");
      return;
    }
    else{
      //tokenize filename into series of directories
      int pNum = pathLength(fname);
      char * paths[pNum];
      pathNameConvert(fname, paths, pNum);
      struct inode * I = malloc(sizeof(struct inode));
      I->size = (size/496)+1; //this may be an issue when size%496==0

      //copy name of file itself into that inode
      for(int s = 0; s < strlen(paths[pNum-2]); s++){
        I->name[s] = paths[pNum-2][s];
      }

      I->numb = -1;
      I->inuse = 1;
      I->type = 1; //for file

      int r = size%496;
      int s;

      //iterate through and write one block at a time
      for(int i = 0; i < 1+(size/496); i++){
        if(i == (size/496)){
          s = r;
        }
        else{
          s = 496;
        }

        //read block by block
        d = readRange(in, (i*496), s + (i)*496);  //read next block of 496 bytes
        struct block * B = malloc(sizeof(struct block));

        for(int b = 0; b < s; b++){
          B->content[b] = ((int *)d)[b];
        }

        //find next free block
        B->size = s;
        B->numb = freeBlockSearch(FBL); //FBL needs to be instantiated first tho

        I->content[i] = B->numb;
        int start = blockAddress(B->numb, fd);

        if(otf)
        {
          //encrypt data and return to B->content
          void * dat = code(B->content, mode, 1, s);
          memcpy(B->content, dat, s);
        }
        //write that block
        writeRange(fd, B, start, start+512);
      }
      //write the inode and update parents
      addInode(I, paths, fd, pNum);

      //write the updated FBL
      updateFBL(FBL, fd);

    }
  }
}

//marks blocks of numbers in blocks as free
void freeBlock(void * FBL, short int blocks){
  int chunk = blocks / 8; //which byte # in FBL are we accessing?
  short int target = 1;  //what specific with in chunk are we reseting?
  for(int i = 7; i > (blocks % 8); i--)
    target *= 2;
  ((int *)FBL)[chunk] ^= target;
}

//clears content and marks as not in use
void deleteInode(struct inode * F, int fd, char * paths[]){

  //get free block and update it
  void * FBL = readFBL(fd);
  F->inuse = 0;
  F->name[0] = 0;
  for(int i = 0; i < F->size; i++){
    freeBlock(FBL, F->content[i]);
    F->content[i] = 0;

  }

  //grab inode
  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode

  i_zero = i_zero + F->numb * sizeof(struct inode);

  //write updated inode
  writeRange(fd, F, i_zero, i_zero+sizeof(struct inode));
  //write FBL
  updateFBL(FBL, fd);
}

//given a list of directory names returns array of inode numbs of those names
short int * pathToNumb(char * path[], int fd, int len){
  short int * order = malloc(sizeof(short int) * len);
  order[0] = 0; //first item in every path is root inode

  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode

  //search for first unfilled inode and put this there.
  struct inode * temp = malloc(sizeof(struct inode));
  temp = readRange(fd, i_zero, i_zero+sizeof(struct inode));  //get root inode
  struct inode * temp2 = malloc(sizeof(struct inode));

  int curEl = 0;


  for(int y = 1; y < len; y++){
    //for every item in this directory, check if that item is next part of path
    for(int i = 0; i < temp->size; i++){
      temp2 = getInodeByNumber(temp->content[i], fd);

      if(strcmp(temp2->name, path[curEl]) == 0){
        temp = temp2;
        i = 99999;

      }
    }

    curEl++;

    order[y] = temp2->numb;
  }



  return order;
}

//given an array of directory names ending in filename, searches for that file's inode
struct inode * getInode(char * path[], int fd, int len){

  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode

  //search for first unfilled inode and put this there.
  struct inode * temp = malloc(sizeof(struct inode));
  temp = readRange(fd, i_zero, i_zero+sizeof(struct inode));  //get root inode
  struct inode * temp2 = malloc(sizeof(struct inode));

  int curEl = 0;

  do{
    //for every item in this directpry, check if that item is next part of path
    for(int i = 0; i < temp->size; i++){
      temp2 = getInodeByNumber(temp->content[i], fd);
      if(strcmp(temp2->name, path[curEl]) == 0){
        temp = temp2;
        i = 99999;

      }
    }

    curEl++;

    temp = temp2;
  }
  while(temp->type != 1); //while inode is a directory

  return temp;
}

void removefilefs(char* fname, int fd){

  int pNum = pathLength(fname);
  char * paths[pNum];
  pathNameConvert(fname, paths, pNum);
  short int * numbs = pathToNumb(paths, fd, pNum);
  struct inode * I = getInode(paths, fd, pNum);
  deleteInode(I, fd, paths);

  pNum -= 2;  //this is parent of inode

  printf("updateing directory structure...\n");

  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode


  while(pNum >= 0){

    struct inode * parent = getInodeByNumber(numbs[pNum], fd);

    for(int i = 0; i < parent->size; i++){
      //search parent->content for removed inode and delete it
      if(parent->content[i] == numbs[pNum+1]){
        parent->content[i] = 0;
      }
    }
    parent->size--; //reduce size of parent by one

    if(parent->size == 0){
      //if no items remoain in directory mark as not inuse and iterate down to this parent
      pNum--;
      parent->inuse = (parent->numb==0);//only mark as not in use if its not root

      //write parent to its location
      writeRange(fd, parent, sizeof(struct inode)*parent->numb + i_zero, sizeof(struct inode)*(1+parent->numb) + i_zero);
      I = parent;
    }
    else{
      //write parent to its location then exit
      writeRange(fd, parent, sizeof(struct inode)*parent->numb + i_zero, sizeof(struct inode)*(1+parent->numb) + i_zero);

      return;
    }
  }
}

//get inode of file and read it
void extractfilefs(char* fname, int fd, int mode, int otf){
  int pNum = pathLength(fname);
  char * paths[pNum];

  pathNameConvert(fname, paths, pNum);
  struct inode * I = getInode(paths, fd, pNum);

  readFile(I, fd, mode, otf);
}

/*
* Print info directly about the inodes
*/
void meta(int fd){

  struct inode * I;

  struct superblock *N=malloc(sizeof(struct superblock));
  N = readRange(fd, 0, sizeof(struct superblock));
  int i_zero = sizeof(struct superblock) + N->numOfBlocks/8; // first/0th inode
  //search for first unfilled inode and put this there.
  int i = -1;
  struct inode * temp = malloc(sizeof(struct inode));

  //go through list of inodes and print them if it has been instantiated
  do{
    I = readRange(fd, i_zero, i_zero+sizeof(struct inode));
    i_zero+=sizeof(struct inode);
    i++;
    //printf info about inodes
    if(I->type != 0)
      printf("Inode %d has type %d, usage %d, and size %d and name of %s\n", I->numb, I->type, I->inuse, I->size, I->name);

  }
  while(i_zero < blockAddress(0, fd));
  printf("any remaining inodes have never been allocated and are empty.\n");

}
