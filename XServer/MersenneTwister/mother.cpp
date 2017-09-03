/************************** MOTHER.CPP ****************** AgF 2007-08-01 *
*  'Mother-of-All' random number generator                               *
*                                                                        *
*  This is a multiply-with-carry type of random number generator         *
*  invented by George Marsaglia.  The algorithm is:                      *
*  S = 2111111111*X[n-4] + 1492*X[n-3] + 1776*X[n-2] + 5115*X[n-1] + C   *
*  X[n] = S modulo 2^32                                                  *
*  C = floor(S / 2^32)                                                   *
*                                                                        *
*  Note:                                                                 *
*  This implementation uses 64-bit integers for intermediate             *
*  calculations. Works only on compilers that support 64-bit integers.   *
*                                                                        *
* © 1999 - 2007 A. Fog.                                                  *
* GNU General Public License www.gnu.org/copyleft/gpl.html               *
*************************************************************************/
#include "Shared.h"
#include "randomc.h"


// Output random bits
uint32 CRandomMother::BRandom() {
  uint64 sum;
  sum = (uint64)2111111111ul * (uint64)x[3] +
     (uint64)1492 * (uint64)(x[2]) +
     (uint64)1776 * (uint64)(x[1]) +
     (uint64)5115 * (uint64)(x[0]) +
     (uint64)x[4];
  x[3] = x[2];  x[2] = x[1];  x[1] = x[0];
  x[4] = uint32(sum >> 32);            // Carry
  x[0] = uint32(sum);                  // Low 32 bits of sum
  return x[0];
} 


// returns a random number between 0 and 1:
double CRandomMother::Random() {
   return (double)BRandom() * (1./(65536.*65536.));
}


// returns integer random number in desired interval:
int CRandomMother::IRandom(int min, int max) {
   // Output random integer in the interval min <= x <= max
   // Relative error on frequencies < 2^-32
   if (max <= min) {
      if (max == min) return min; else return 0x80000000;
   }
   // Multiply interval with random and truncate
   int r = int((max - min + 1) * Random()) + min; 
   if (r > max) r = max;
   return r;
}


// this function initializes the random number generator:
void CRandomMother::RandomInit (uint32 seed) {
  int i;
  uint32 s = seed;
  // make random numbers and put them into the buffer
  for (i = 0; i < 5; i++) {
    s = s * 29943829 - 1;
    x[i] = s;
  }
  // randomize some more
  for (i=0; i<19; i++) BRandom();
}
