#include "q_correction.h"
/*
void printBin(uint16_t in)
{
  in <<= 2;
  for (int i = 0; i < 14; i++){
    printf("%c", (in & 0x8000)?'1':'0');
    in <<= 1;
  }
}
*/
uint16_t qCorMul(const uint16_t *m, uint16_t v){
  uint16_t result = 0;
  for (int i = 0; i < 14; i++){
    uint16_t tmp = m[i] & v;
    tmp ^= tmp >> 8;
    tmp ^= tmp >> 4;
    tmp &= 0xf;
    if (0x6996 >> tmp & 1){
      result |= 0x4000;
    }
    //if (i != 13)
      result >>= 1;
  }
  return result;
}

// T^{x} * Sample
uint16_t qCorTS(uint8_t x, uint16_t s){
  for (int i = 0; i < x; i++){
    s <<= 1;
    if (s & 0x2000){
      s ^= 0x11;
    }
  }
  s &= ~0xc000;
  return s;
}
