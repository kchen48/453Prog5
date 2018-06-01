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
      printf("no partitioning\n");
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
      fclose(imgInfo->f);
      exit(-1);
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
      exit(-1);
   }

   /* get zonesize and set superblock info for later */
   imgInfo->zonesize = superBlock->blocksize << superBlock->log_zone_size;
   imgInfo->superBlock = *superBlock;
   if (imgInfo->verbose != -1){
      printf("verbose set\n");
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

   /* if path */
   if (s > 1){
      /* copy over */
      path_copy = (char*) malloc (s);
      strcpy(path_copy, path+1);
      /* get inode with path */
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
   int i2;
   int total;

   struct f *fil = openFile(imgInfo, i);
   struct fileent *ent = fil->ents;

   if ((point=strstr(path, "/"))){
     *point++ = '\0';
   }
   filename[LENFILENAMES] = '\0';

   total = fil->node.size/sizeof(struct fileent);
   for (i2=0; i2<total; i2++){
      memcpy(filename, ent->name, LENFILENAMES);
      ent++;
      if (!strcmp(path, filename)){
         break;
      }
   }

   if (i2==total){
      freeFile(fil);
   }
   if (point){
      if (*point){
         return getInodeReg(imgInfo, point, i);
      }
   }
   return i;
}


struct f* findFilePath(struct info *imgInfo, char *path){
   struct f *fil = (struct f*) malloc(sizeof(struct f));
   uint32_t i;

   /* get correct path into struct */
   if (path[0]!='/'){
      fil->path = (char*) malloc(strlen(path)+1);
      fil->path[0] = '/';
      strcpy(fil->path+1, path);
   }
   else{
      fil->path = (char*) malloc(strlen(path));
      strcpy(fil->path, path);
   }
   
   i = getInodePath(imgInfo, fil->path);
   if (!i){
      free(fil);
      return NULL;
   }

   findInode(imgInfo, &fil->node, i);
   fil->cont = NULL;
   fil->ents = NULL;
   fil->numEnts =0;

   if (MIN_ISDIR(fil->node.mode)){
      loadDirectory(imgInfo, fil);
   }
   return fil;
}

struct f* openFile(struct info *imgInfo, uint32_t i){
   struct f *fil = (struct f*) malloc(sizeof(struct f));
   findInode(imgInfo, &fil->node, i);
   fil->cont = NULL;
   fil->ents = NULL;
   fil->path = NULL;
   fil->numEnts = 0;
   if (MIN_ISDIR(fil->node.mode)){
      loadDirectory(imgInfo, fil);
   }
   return fil;

}


void loadFile(struct info *imgInfo, struct f *file){
   long s = file->node.size;
   if (s){

      uint8_t *point;
      uint8_t *hold;
      int i;
      long move, offset;
      hold = (uint8_t*) malloc(imgInfo->zonesize);
      file->cont = (uint8_t*)malloc(s);
      point = file->cont;

      for (i=0; i< DIRECT_ZONES; i++){
         if (s <= 0){
            break;
         }
         if (s < imgInfo->zonesize){
            move=s;
         }
         else{
            move=imgInfo->zonesize;
         }
         if (file->node.zone[i]){
            offset = imgInfo->place+imgInfo->zonesize*file->node.zone[i];
            fseek(imgInfo->f, offset, SEEK_SET);
            fread((void*)hold, move, 1, imgInfo->f);
            memcpy(point, hold, move);
         }
         else{
            memset(point, 0, move);
         }
         s -= imgInfo->zonesize;
         point += imgInfo->zonesize;
      }  
      free(hold);
   }
   else {
      file->cont = NULL;
   }
}

void loadDirectory(struct info *imgInfo, struct f *direct){
   int i;

   loadFile(imgInfo, direct);
   if (direct->cont){
      struct fileent *point, *entry = (struct fileent*) direct->cont;
      direct->numEnts = direct->node.size/sizeof(struct fileent);
      direct->ents = (struct fileent*)malloc(direct->numEnts * sizeof(struct fileent));
      
      point = direct->ents;
      for (i=0; i <direct->numEnts; i++){
         *point = *entry;
         point++;
         entry++;
      }
   }
}

void openImg(struct info *imgInfo){
   imgInfo->f = fopen(imgInfo->image, "rb");
   if (!imgInfo->f){
      printf("Cannot open disk image\n");
      exit(-1);
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
   for (i2 = 0; i2 < DIRECT_ZONES; i2++)
      printf("              zone[%d]   = %10u\n", i2, i->zone[i2]);

   printf("  unsigned long  indirect %10u\n", i->indirect);
   printf("  unsigned long  double %12u\n", i->unused);
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
      printf("boot 0x%02x\n", partTable->bootind);
      printf("head %4u\n", partTable->start_head);
      printf("start sec %4u\n", partTable->start_sec);
      printf("start cyl %4u\n", partTable->start_cyl);
      printf("type 0x%02x\n", partTable->type);
      printf("head %4u\n", partTable->end_head);
      printf("end sec %4u\n", partTable->end_sec);
      printf("end cyl %4u\n", partTable->end_cyl);
      printf("lFirst%10u\n", partTable->lFirst);
      printf("size %10u\n", partTable->size);
      partTable++;
   }
}

void printEnts(struct info *imgInfo, struct fileent *ent){
   char *name = (char*) malloc(LENFILENAMES +1);
   struct inode i;
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

   fil = findFilePath(imgInfo, imgInfo->image);
   if (fil){
      if (imgInfo->verbose != -1){
         printInode(&fil->node);
      }
      if (fil->ents){
         printf("%s:\n", imgInfo->image);
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
         printf("%10d %s\n", fil->node.size, fil->path);
      }
      freeFile(fil);
   }
   else{
      printf("File not found: %s\n", imgInfo->image);
      fclose(imgInfo->f);
      exit(-1);
   }
}

void writeOut(struct info *imgInfo){
   struct f *fil;
   FILE *f;
   fil = findFilePath(imgInfo, imgInfo->src);
   if (fil){
      if (MIN_ISDIR(fil->node.mode)){
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
      exit(-1);
   }
}

