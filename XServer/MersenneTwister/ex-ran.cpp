/*************************** EX-RAN.CPP ********************* AgF 2007-09-23 *
*                                                                            *
* Example showing how to use random number generators from class library.    *
*                                                                            *
* Instructions:                                                              *
* Compile for console mode, and run.                                         *
*                                                                            *
* This example uses the "Mersenne twister" random number generator.          *
* To use the "Mother of all" random number generator, replace the class name *
* CRandomMersenne with CRandomMother and include mother.cpp.                 *
*                                                                            *
* To use the faster assembly language implementation, use class name         *
* CRandomMersenneA or CRandomMotherA, include asmlibran.h and link with      *
* the appropriate version of the asmlib library from asmlib.zip.             *
*                                                                            *
* © 2001-2007 Agner Fog. General Public License, see licence.htm             *
*****************************************************************************/

#include <time.h>                      // define time()
#include "randomc.h"                   // define classes for random number generators

#ifndef MULTIFILE_PROJECT
// If compiled as a single file then include these cpp files, 
// If compiled as a project then compile and link in these cpp files.
   #include "mersenne.cpp"             // code for random number generator
   #include "userintf.cpp"             // define system specific user interface
#endif


int main() {
   int32 seed = (int32)time(0);        // random seed

   // choose one of the random number generators:
   CRandomMersenne rg(seed);           // make instance of random number generator
   // or:
   // CRandomMother rg(seed);          // make instance of random number generator

   int i;                              // loop counter
   double r;                           // random number
   int32 ir;                           // random integer number


   // make random integers in interval from 0 to 99, inclusive:
   printf("\n\nRandom integers in interval from 0 to 99:\n");
   for (i=0; i<40; i++) {
      ir = rg.IRandom(0,99);
      printf ("%6li  ", ir);
   }

   // make random floating point numbers in interval from 0 to 1:
   printf("\n\nRandom floating point numbers in interval from 0 to 1:\n");
   for (i=0; i<32; i++) {
      r = rg.Random();
      printf ("%8.6f  ", r);
   }

   // make random bits (Not for TRandomMotherOfAll):
   printf("\n\nRandom bits (hexadecimal):\n");
   for (i=0; i<32; i++) {
      ir = rg.BRandom();
      printf ("%08lX  ", ir);
   }

   EndOfProgram();                     // system-specific exit code
   return 0;
}

