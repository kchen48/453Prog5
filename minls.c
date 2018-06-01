/* Kelly Chen (kchen48)
 * Richard Ding (riding)
 *
 * Source code for minls */

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
      printf("usage: minls [ -v ] [ -p num [ -s num] ] imagefile [ path ]\n\
            Options:\n\
            -p part    --- select partition for filsystem (default: none)\n\
            -s sub     --- select subpartition for filesystem (default: none)\n\
            -h help    --- print usage information and exit\n\
            -v verbose --- increase verbosity level\n");
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

   countargs++;

   imgInfo.src = "/";
   if (countargs+1 == argc){
      imgInfo.src = argv[countargs];
      printf("setting src: %s\n", imgInfo.src);
   }

   imgInfo.place = 0;
   openImg(&imgInfo);
   partAndSubpart(&imgInfo);
   superBlock(&imgInfo);
   printFile(&imgInfo);
   fclose(imgInfo.f);

   return 0;
}

