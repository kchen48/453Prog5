/* Kelly Chen (kchen48)
 * Richard Ding (riding)
 *
 * Source code for minget */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "min.h"

int main(int argc, char *argv[]){
   int check;
   int countargs = 0;

   struct info imgInfo;


   if ((argc == 1) || (checkFlag("-h", argc, argv)!=-1)){
      printf("usage: minls [ -v ] [ -p num [ -s num] ] imagefile [ path ]\n");
      printf("Options:\n");
      printf("-p part    --- select partition for filsystem (default: none)\n");
      printf("-s sub     --- select subpartition ");
      printf("for filesystem (default: none)\n");
      printf("-h help    --- print usage information and exit\n");
      printf("-v verbose --- increase verbosity level\n");
      return 0;
   }

   if (checkFlag("-v", argc, argv) != -1){
      imgInfo.verbose = 1;
      countargs++;
   }
   else{
      imgInfo.verbose = -1;
   }

   check = checkFlag("-p", argc, argv);
   if (check != -1){
      imgInfo.part = (int) strtol(argv[check+1], (char**)NULL, 10);
      countargs+=2;
   }
   else{
      imgInfo.part = -1;
   }

   check = checkFlag("-s", argc, argv);
   if (check != -1){
      imgInfo.sub = (int) strtol(argv[check+1], (char**)NULL, 10);
      countargs+=2;
   }
   else{
      imgInfo.sub = -1;
   }

   countargs++;
   imgInfo.image = argv[countargs];
   
   if (!imgInfo.image){
      exit(1);
   }

   countargs++;
   imgInfo.src = argv[countargs];
   countargs++;
   if (countargs+1 == argc){
      imgInfo.dstpath = argv[countargs];
   }

   imgInfo.place = 0;

   openImg(&imgInfo);
   partAndSubpart(&imgInfo);
   superBlock(&imgInfo);
   writeOut(&imgInfo);
   fclose(imgInfo.f);

   return 0;
}
