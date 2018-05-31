#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "min.h"

int checkFlag(char *flag, int argc, char *argv[]){
   int i;
   for (i=0; i<argc; i++){
      if (strcmp(argv[i], flag) == 0){
         return i;
      }
   }
   return -1;
}

void partAndSubpart(struct info *imgInfo){
   if (imgInfo->part == -1){
      return;
   }
   if (imgInfo->verbose != -1){
      printf("\n");
      printf("Partition table:\n");
   }
   partition(imgInfo);
   if (imgInfo->verbose != -1){
      printf("\n");
      printf("Subpartition table:\n");
   }
   subpartition(imgInfo);
}

void partition(struct info *imgInfo){
   uint8_t blck[LENBLOCKS];
   struct pt *partTable;

   /* move to correct block */
   fseek(imgInfo->f, imgInfo->place, SEEK_SET);
   fread((void*)blck, LENBLOCKS, 1, imgInfo->f);
  
   /* check signature */
   if ((blck[SIGBYTE510] != PMAGIC510) || (blck[SIGBYTE511] != PMAGIC511)){
      printf("Invalid partition table, cannot open disk image\n");
      fclose(infoImg->f);
      exit(-1);
   }

   /* onto partition table */
   partTable = (struct pt*)(blck+PTABLE_OFFSET);
   if (imgInfo->verbose){
      printPT(partTable);
   }

   /* add partition num */
   partTable+=imgInfo->part;
   
   /* check minix */
   if (partTable->type != MINIXPART){
      printf("Not MINIX subpartition, cannot open disk image\n");
      fclose(infoImg->f);
      exit(-1);
   }
   /* move past sectors */
   imgInfo->place = partTable->lFirst*LENSECTORS;
}

void subpartition(struct info *imgInfo){
   uint8_t blck[LENBLOCKS];
   struct pt *subpartTable;

   /* move to correct block */
   fseek(imgInfo->f, imgInfo->place, SEEK_SET);
   fread((void*)blck, LENBLOCKS, 1, imgInfo->f);
   
   /* check signature */
   if ((blck[SIGBYTE510] != PMAGIC510) || (blck[SIGBYTE511] != PMAGIC511)){
      printf("Invalid partition table, cannot open disk image\n");
      fclose(infoImg->f);
      exit(-1);
   }

   /* onto subpartition table */
   subpartTable = (struct pt*)(blck+PTABLE_OFFSET);
   if (imgInfo->verbose){
      printPT(subpartTable);
   }

   /* add subpartition num */
   subpartTable+=imgInfo->sub;

   /* check minix */
   if (subpartTable->type != MINIXPART){
      printf("Not MINIX subpartition, cannot open disk image\n");
      fclose(infoImg->f);
      exit(-1);
   }

   /* move past sectors */
   imgInfo->place = subpartTable->lFirst*LENSECTORS;
}

void superBlock(struct info *imgInfo){
   uint8_t blck[LENBLOCKS];

   /* move to correct block */
   fseek(imgInfo->f, imgInfo->place+LENBLOCKS, SEEK_SET);
   fread((void*)blck, LENBLOCKS, 1, imgInfo->f);

   /* check magic number */
   struct superblock *superBlock = (struct superblock*) blck;
   if (superBlock->magic != MIN_MAGIC){
      printf("Bad magic number. (0x%04x)\n", superBlock->magic);
      printf("This doesn't look like a MINIX filesystem.\n");
      fclose(imgInfo->f);
      exit(-1);
   }

   /* get zonesize and set superblock info for later */
   imgInfo->zonesize = superBlock->blocksize << superBlock->log_zone_size;
   imgInfo->superBlock = *superBlock;
   if (imgInfo->verbose){
      printSuperBlock(superBlock);
   }
}

void findInode(struct info *imgInfo, struct inode *node, uint32_t num){
}

void getInodePath(struct info *imgInfo, char *path){
}

void getInodeReg(struct info *imgInfo, char *path, uint32_t i){
}

struct f* findFilePath(struct info *imgInfo, char *path){
}

struct f* openFile(struct info *imgInfo, uint32_t i){
}

void loadFile(struct info *imgInfo, struct f *file){
}

void loadDirectory(struct info *imgInfo, struct f *direct){
}

void openImg(struct info *imgInfo){
}

void freeFile(struct f *file){


}

void printPermissions(uint16_t mode){
   char permissions[LENPERMS];
   *permissions = MIN_ISDIR(mode) ? 'd' : '-';
   permissions++;
   *permissions = mode & MIN_IRUSR ? 'r' : '-';
   permissions++;
   *permissions = mode & MIN_IWUSER ? 'w' : '-';
   permissions++;
   *permissions = mode & MIN_IXUSR ? 'x' : '-';
   permissions++;
   *permissions = mode & MIN_IRGRP ? 'r' : '-';
   permissions++;
   *permissions = mode & MIN_IWGRP ? 'w' : '-';
   permissions++;
   *permissions = mode & MIN_IXGRP ? 'x' : '-';
   permissions++;
   *permissions = mode & MIN_IROTH ? 'r' : '-';
   permissions++;
   *permissions = mode & MIN_IWOTH ? 'w' : '-';
   permissions++;
   *permissions = mode & MIN_IXOTH ? 'x' : '-';
   permissions++;
   *permissions = '\0';
   printf("%s\n", permissions);
}
}

void printInode(struct inode *i){
}

void printSuperBlock(struct superblock *superBlock){
}

void printPT(struct pt *partTable){
}


