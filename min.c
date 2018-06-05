#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "min.h"
#include <time.h>

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
   
   if (imgInfo->sub == -1){
      return;
   }
   
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
      fclose(imgInfo->f);
      exit(3);
   }

   /* onto partition table */
   partTable = (struct pt*)(blck+PTABLE_OFFSET);
   if (imgInfo->verbose != -1){
      printPT(partTable);
   }

   /* add partition num */
   partTable+=imgInfo->part;
   
   /* check minix */
   if (partTable->type != MINIXPART){
      printf("Not MINIX subpartition, cannot open disk image\n");
      fclose(imgInfo->f);
      exit(3);
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
      fclose(imgInfo->f);
      exit(-1);
   }

   /* onto subpartition table */
   subpartTable = (struct pt*)(blck+PTABLE_OFFSET);
   if (imgInfo->verbose != -1 ){
      printPT(subpartTable);
   }

   /* add subpartition num */
   subpartTable+=imgInfo->sub;

   /* check minix */
   if (subpartTable->type != MINIXPART){
      printf("Not MINIX subpartition, cannot open disk image\n");
      fclose(imgInfo->f);
      exit(-1);
   }

   /* move past sectors */
   imgInfo->place = subpartTable->lFirst*LENSECTORS;
}

void superBlock(struct info *imgInfo){
   uint8_t blck[LENBLOCKS];
   struct superblock *superBlock;

   /* move to correct block */
   fseek(imgInfo->f, imgInfo->place+LENBLOCKS, SEEK_SET);
   fread((void*)blck, LENBLOCKS, 1, imgInfo->f);

   /* check magic number */
   superBlock = (struct superblock*) blck;
   if (superBlock->magic != MIN_MAGIC){
      printf("Bad magic number. (0x%04x)\n", superBlock->magic);
      printf("This doesn't look like a MINIX filesystem.\n");
      fclose(imgInfo->f);
      exit(255);
   }

   /* get zonesize and set superblock info for later */
   imgInfo->zonesize = superBlock->blocksize << superBlock->log_zone_size;
   imgInfo->superBlock = *superBlock;
   if (imgInfo->verbose != -1){
      printSuperBlock(superBlock);
   }
}

void findInode(struct info *imgInfo, struct inode *node, uint32_t num){
   /* find inode place */
   uint16_t blcksize = imgInfo->superBlock.blocksize;
   long offset = imgInfo->place + (2*blcksize);
   offset += blcksize*(imgInfo->superBlock.i_blocks);
   offset += blcksize*(imgInfo->superBlock.z_blocks);
   offset += (num-1) * sizeof(struct inode);
 
   /* move to correct place */
   fseek(imgInfo->f, offset, SEEK_SET);
   fread((void*)node, sizeof(struct inode), 1, imgInfo->f);
}

uint32_t getInodePath(struct info *imgInfo, char *path){
   size_t s = strlen(path);
   uint32_t i;
   char *path_copy = NULL;

   path_copy = (char*) checked_malloc(s);
   /* if path */
   if (s > 1){
      /* copy over */
      strcpy(path_copy, path+1);
      /* get inode with path */
      path_copy[s-1] = '\0';
      /* printf("PATH COPY IN GETINODEPATH: %s\n", path_copy);*/
      i = getInodeReg(imgInfo, path_copy, 1);
      free(path_copy);
   }
   else{
      i = 1;
   }

   return i;
}


uint32_t getInodeReg(struct info *imgInfo, char *path, uint32_t i){
   char filename[LENFILENAMES+1];
   char *point;
   int i2, i3;
   int total;


   /* opens file and gets entries */
   struct f *fil = openFile(imgInfo, i);
   struct fileent *ent = fil->ents;

   /* null terminate to go through path */
   if (point=strstr(path, "/")){
     *point++ = '\0';
   }
   filename[LENFILENAMES] = '\0';

   
   /* copy over entry name */
   total = fil->node.size/sizeof(struct fileent);
/*   printf("total: %d\n", total); */

   /*printf("looking for: %s\n", path);*/
   for (i2=0; i2<total; i2++, ent++){
     /* printf("ent: %s\n", ent->name);*/
      memcpy(filename, ent->name, LENFILENAMES);
      if (!strcmp(path, filename)){
         break;
      }
   }
   if (i2==total){
      freeFile(fil);
      exit(1);
   }

   /* gets inode */
   i = ent->ino;
   freeFile(fil);
   
   /* gets next inode to go through path */
   if (point&&*point){
         return getInodeReg(imgInfo, point, i);
   }
   return i;

}


struct f* findFilePath(struct info *imgInfo, char *path){
   struct f *fil = (struct f*) checked_malloc(sizeof(struct f));
   uint32_t i;

   /* get correct path into struct */
   if (path[0]!='/'){
      fil->path = (char*) checked_malloc(strlen(path)+1);
      fil->path[0] = '/';
      strcpy(fil->path+1, path);
   }
   else{
      fil->path = (char*) checked_malloc(strlen(path));
      strcpy(fil->path, path);
   }
   
   i = getInodePath(imgInfo, fil->path);
   if (!i){
      free(fil);
      return NULL;
   }

   /* move to correct inode place and set up file struct */
   findInode(imgInfo, &fil->node, i);
   fil->cont = NULL;
   fil->ents = NULL;
   fil->numEnts =0;

   /* if directory */
   if (MIN_ISDIR(fil->node.mode)){
      loadDirectory(imgInfo, fil);
   }
   return fil;
}

struct f* openFile(struct info *imgInfo, uint32_t i){
   struct f *fil = (struct f*) checked_malloc(sizeof(struct f));
   
   /* move to correct place and set up file struct */
   findInode(imgInfo, &fil->node, i);
   fil->cont = NULL;
   fil->ents = NULL;
   fil->path = NULL;
   fil->numEnts = 0;

   /* if directory */
   if (MIN_ISDIR(fil->node.mode)){
      loadDirectory(imgInfo, fil);
   }
   return fil;

}


void loadFile(struct info *imgInfo, struct f *file){
   long s = file->node.size;
   int zones = imgInfo->zonesize/4;

   if (s){
      uint8_t *point;
      uint8_t *hold;
      int i, j;
      long move, offset;

      /* goes through zonesize and puts into file contents */
      hold = (uint8_t*) checked_malloc(imgInfo->zonesize);
      file->cont = (uint8_t*)checked_malloc(s);
      point = file->cont;

      /* direct zones */
      for (i=0; i< DIRECT_ZONES && s>0; i++){
         if (s < imgInfo->zonesize){
            move=s;
         }
         else{
            move=imgInfo->zonesize;
         }

         /* move to next zone */
         if (file->node.zone[i]){
            offset = imgInfo->place+imgInfo->zonesize*file->node.zone[i];
            /*printf("node.zone[%d]: %d\n", i, file->node.zone[i]);*/
            fseek(imgInfo->f, offset, SEEK_SET);
            /*printf("OFFSET: %d\n", offset);*/
            fread((void*)hold, move, 1, imgInfo->f);
            memcpy(point, hold, move);
         }
         else{
            memset(point, 0, move);
         }
         s -= imgInfo->zonesize;
         point += imgInfo->zonesize;
      } 

      if (s>0){
         if (file->node.indirect){
            uint32_t *curr_zone = malloc(sizeof(uint32_t)*imgInfo->zonesize);
            fseek(imgInfo->f,
                  imgInfo->place+file->node.indirect*imgInfo->zonesize,
                  SEEK_SET);
            fread((void*)hold, imgInfo->zonesize, 1, imgInfo->f);
            memcpy(curr_zone, hold, imgInfo->zonesize);

            for (i=0; i< zones &&s>0; i++){
               if (s < imgInfo->zonesize){
                  move = s;
               }
               else{
                  move= imgInfo->zonesize;
               }
               if (curr_zone[i]){
                  offset = imgInfo->place+imgInfo->zonesize*curr_zone[i];
                  fseek(imgInfo->f, offset, SEEK_SET);
                  fread((void*)hold, move, 1, imgInfo->f);
                  memcpy(point, hold, move);
               }
               else{
                  memset(point, 0, move);
               }
               s-=imgInfo->zonesize;
               point+=imgInfo->zonesize;
            }
            free(curr_zone);
         }
      }

      if (s>0){
         if (file->node.two_indirect){
            uint32_t *curr_zone_two = 
               malloc(sizeof(uint32_t)*imgInfo->zonesize);
            uint32_t *di_zone = malloc(sizeof(uint32_t)*imgInfo->zonesize);
            fseek(imgInfo->f,
                  imgInfo->place+file->node.two_indirect*imgInfo->zonesize,
                  SEEK_SET);
            fread((void*)hold, imgInfo->zonesize, 1, imgInfo->f);
            memcpy(di_zone, hold, imgInfo->zonesize);

            int j;
            for (j=0; j< zones&&s>0; j++){
               if (di_zone[j]){
                  fseek(imgInfo->f,
                        imgInfo->place+di_zone[j]*imgInfo->zonesize,
                        SEEK_SET);
                  fread((void*)hold, imgInfo->zonesize, 1, imgInfo->f);
                  memcpy(curr_zone_two, hold, imgInfo->zonesize);

                  for (i=0; i< zones &&s>0; i++){
                     if (s < imgInfo->zonesize){
                        move = s;
                     }
                     else{
                        move= imgInfo->zonesize;
                     }
                     if (curr_zone_two[i]){
                       offset=imgInfo->place+imgInfo->zonesize*curr_zone_two[i];
                        fseek(imgInfo->f, offset, SEEK_SET);
                        fread((void*)hold, move, 1, imgInfo->f);
                        memcpy(point, hold, move);
                     }
                     else{
                        memset(point, 0, move);
                     }
                     s-=imgInfo->zonesize;
                     point+=imgInfo->zonesize;
                  }
               }

            }
            free(curr_zone_two);
            free(di_zone);
         }
      }
      /* indirect zone, FIX HOLES */
      /*  if (s>0){
          int zones = imgInfo->zonesize / 4; 
          printf("zones: %d\n", zones);
          offset = offset+imgInfo->zonesize;
          for (i=0; i<zones && s>0; i++){
          if (s < imgInfo->zonesize){
          move = s;
          }
          else{
          move= imgInfo->zonesize;
          }
          if (i>=1){
          offset= imgInfo->place;
          offset+=imgInfo->zonesize*file->node.indirect;
          }
          if (offset){
          fseek(imgInfo->f, offset+imgInfo->zonesize*i, SEEK_SET);
          fread((void*)hold, move, 1, imgInfo->f);
          memcpy(point, hold, move);
          }
          else{
          memset(point, 0, move);
          }
          s -= imgInfo->zonesize;
          point += imgInfo->zonesize;
          }
          }*/
      /*
         if (s>0){
         long two_offset = offset+imgInfo->zonesize;
         while (s>0){
         for (i=0; i< DIRECT_ZONES && s>0; i++){
         if (s < imgInfo->zonesize){
         move = s;
         }
         else{
         move= imgInfo->zonesize;
         }
         if (file->node.two_indirect){
         if (i >= 1){
         offset = imgInfo->place;
         offset+=imgInfo->zonesize*two_offset;
         }
         else{
         offset = two_offset+imgInfo->zonesize;
         }
         fseek(imgInfo->f, offset+(imgInfo->zonesize*i), SEEK_SET);
         printf("i: %d\n", i);
         fread((void*)hold, move, 1, imgInfo->f);
         memcpy(point, hold, move);
         }
         else{
         memset(point, 0, move);
         }
         s -= imgInfo->zonesize;
         point += imgInfo->zonesize;
         }

         }
         two_offset += imgInfo->zonesize;
         }

*/

      /* FIX: double indirect zone :( 
         if (s>0){
         long two_offset = offset+imgInfo->zonesize;
         long two_offset2;
         while (s>0){
         for (i=0; i< DIRECT_ZONES && s>0; i++){
         if (s < imgInfo->zonesize){
         move = s;
         }
         else{
         move= imgInfo->zonesize;

         two_offset2 = imgInfo->place+two_offset+(imgInfo->zonesize*i);
         if (two_offset2){
         fseek(imgInfo->f, two_offset2, SEEK_SET);
         fread((void*)hold, move, 1, imgInfo->f);
         memcpy(point, hold, move);
         }
         else{
         memset(point, 0, move);
         }
         s -= imgInfo->zonesize;
         point += imgInfo->zonesize;
         }
         two_offset += imgInfo->zonesize;
         }

         }*/



      free(hold);
   }
         else {
            file->cont = NULL;
         }
   }

   void loadDirectory(struct info *imgInfo, struct f *direct){
      int i;
      size_t size;
      loadFile(imgInfo, direct);

      /* get entries */
      if (direct->cont){
         /* goes through directory for entries */
         struct fileent *point, *entry = (struct fileent*) direct->cont;
         direct->numEnts = direct->node.size/sizeof(struct fileent);
         /*printf("number of dir entries: %d\n", direct->numEnts);*/
         size = direct->numEnts * sizeof(struct fileent);
         direct->ents = (struct fileent*)checked_malloc(size);

         point=direct->ents;
         for (i=0; i <direct->numEnts; i++){
            *point = *entry;
            /*  printf("entry loaded: %s\n", point->name);*/
            point++;
            entry++;
         }
      }
   }

   void openImg(struct info *imgInfo){
      imgInfo->f = fopen(imgInfo->image, "rb");
      if (!imgInfo->f){
         printf("Cannot open disk image\n");
         exit(3);
      }
   }

   void freeFile(struct f *file){
      free(file->cont);
      free(file->ents);
      free(file);
   }

   void printPermissions(uint16_t mode){
      char *i;
      char permissions[LENPERMS];
      i = permissions;

      *i = MIN_ISDIR(mode) ? 'd' : '-';
      i++;
      *i = mode & MIN_IRUSR ? 'r' : '-';
      i++;
      *i = mode & MIN_IWUSR ? 'w' : '-';
      i++;
      *i = mode & MIN_IXUSR ? 'x' : '-';
      i++;
      *i = mode & MIN_IRGRP ? 'r' : '-';
      i++;
      *i = mode & MIN_IWGRP ? 'w' : '-';
      i++;
      *i = mode & MIN_IXGRP ? 'x' : '-';
      i++;
      *i = mode & MIN_IROTH ? 'r' : '-';
      i++;
      *i = mode & MIN_IWOTH ? 'w' : '-';
      i++;
      *i = mode & MIN_IXOTH ? 'x' : '-';
      i++;
      *i = '\0';
      printf("%s", permissions);
   }


   void printInode(struct inode *i){
      int i2;
      char filenames[LENFILENAMES];
      time_t t;

      printf("\n");
      printf("File inode:\n");
      printf("  unsigned short mode         0x%04x", i->mode);
      printf("\t(");
      printPermissions(i->mode);
      printf(")\n");

      printf("  unsigned short links %13u\n", i->links);
      printf("  unsigned short uid %15u\n", i->uid);
      printf("  unsigned short gid %15u\n", i->gid);
      printf("  unsigned long  size %14u\n", i->size);

      t = i->atime;
      strftime(filenames, LENFILENAMES, "%a %b %e %T %Y", localtime(&t));
      printf("  unsigned long  atime %13u\t--- %s\n", i->atime, filenames);
      t = i->mtime;
      strftime(filenames, LENFILENAMES, "%a %b %e %T %Y", localtime(&t));
      printf("  unsigned long  mtime %13u\t--- %s\n", i->mtime, filenames);
      t = i->ctime;
      strftime(filenames, LENFILENAMES, "%a %b %e %T %Y", localtime(&t));
      printf("  unsigned long  ctime %13u\t--- %s\n", i->ctime, filenames);

      printf("\n  Direct zones:\n");
      for (i2 = 0; i2 < DIRECT_ZONES; i2++){
         printf("              zone[%d]   = %10u\n", i2, i->zone[i2]);
      }

      printf("  unsigned long  indirect %10u\n", i->indirect);
      printf("  unsigned long  double %12u\n", i->two_indirect);
   }

   void printSuperBlock(struct superblock *superBlock){
      uint32_t zone = superBlock->blocksize << superBlock->log_zone_size;
      printf("\nSuperblock Contents:\n");
      printf("Stored Fields:\n");
      printf("  ninodes %12u\n", superBlock->ninodes);
      printf("  i_blocks %11d\n", superBlock->i_blocks);
      printf("  z_blocks %11d\n", superBlock->z_blocks);
      printf("  firstdata %10u\n", superBlock->firstdata);
      printf("  log_zone_size %6d", superBlock->log_zone_size);
      printf(" (zone size: %u)\n", zone);
      printf("  max_file %11u\n", superBlock->max_file);
      printf("  magic         0x%04x\n", superBlock->magic);
      printf("  zones %14u\n", superBlock->zones);
      printf("  blocksize %10u\n", superBlock->blocksize);
      printf("  subversion %9u\n", superBlock->subversion);
   }

   void printPT(struct pt *partTable){
      int i;
      for (i = 0; i < 4; i++) {
         printf("boot      0x%02x\n", partTable->bootind);
         printf("head      %4u\n", partTable->start_head);
         printf("start sec %4u\n", partTable->start_sec);
         printf("start cyl %4u\n", partTable->start_cyl);
         printf("type      0x%02x\n", partTable->type);
         printf("head      %4u\n", partTable->end_head);
         printf("end sec   %4u\n", partTable->end_sec);
         printf("end cyl   %4u\n", partTable->end_cyl);
         printf("lFirst    %10u\n", partTable->lFirst);
         printf("size      %10u\n", partTable->size);
         partTable++;
      }
   }

   void printEnts(struct info *imgInfo, struct fileent *ent){
      char *name = (char*) malloc(LENFILENAMES +1);
      struct inode i;

      /* moves to right place and copies name to print */
      findInode(imgInfo, &i, ent->ino);
      memcpy(name, ent->name, LENFILENAMES);
      name[LENFILENAMES] = '\0';
      printPermissions(i.mode);
      printf("%10d %s\n", i.size, name);
      free(name);
   }

   void printFile(struct info *imgInfo){
      struct f *fil;
      struct fileent *ent;
      int i;

      fil = findFilePath(imgInfo, imgInfo->src);
      if (fil){
         if (imgInfo->verbose != -1){
            printInode(&fil->node);
         }
         if (fil->ents){
            printf("%s:\n", imgInfo->src);
            ent = fil->ents;
            for (i=0; i<fil->numEnts; i++){
               if (ent->ino){
                  printEnts(imgInfo, ent);
               }
               ent++;
            }
         }
         else{
            printPermissions(fil->node.mode);
            printf("%10d %s\n", fil->node.size, imgInfo->src);
         }
         freeFile(fil);
      }
      else{
         printf("File not found: %s\n", imgInfo->src);
         fclose(imgInfo->f);
         exit(1);
      }
   }

   void writeOut(struct info *imgInfo){
      struct f *fil;
      FILE *f;
      fil = findFilePath(imgInfo, imgInfo->src);
      if (fil){
         if (!MIN_ISREG(fil->node.mode)){
            printf("Not regular file: %s\n", imgInfo->src);

            fclose(imgInfo->f);
            exit(-1);
         }
         loadFile(imgInfo, fil);
         if (imgInfo->dstpath){
            f = fopen(imgInfo->dstpath, "wb");
            if (!f){
               printf("Cannot open file for output\n");
               freeFile(fil);
               fclose(imgInfo->f);
               exit(1);
            }
            fwrite(fil->cont, fil->node.size, 1, f);
            fclose(f);
         }
         else{
            write(STDOUT_FILENO, fil->cont, fil->node.size);
         }
         freeFile(fil);
      }
      else{
         printf("Cannot find file: %s\n", imgInfo->src);
         fclose(imgInfo->f);
         exit(1);
      }
   }

   void* checked_malloc(size_t size){
      void *p = malloc(size);
      if (p==NULL){
         perror("Malloc error\n");
         exit(-1);
      }
      return p;
   }

