/* Kelly Chen (kchen48)
 * Richard Ding (riding)
 *
 * Source code for minls */

#include <stdio.h>
#include <stdint.h>
#include "min.h"

int main(int argc, char *argv[]){
   if (argc == 1){
      printf("usage: minls [ -v ] [ -p num [ -s num] ] imagefile [ path ]\n\
      Options:\n\
      -p part    --- select partition for filsystem (default: none)\n\
      -s sub     --- select subpartition for filesystem (default: none)\n\
      -h help    --- print usage information and exit\n\
      -v verbose --- increase verbosity level\n");
   }

   return 0;
}

