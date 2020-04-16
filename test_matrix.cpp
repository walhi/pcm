#include <cstdint>
#include <stdio.h>

#include "q_correction.h"

#define SIZE 14
#define L0_POS 0
#define R0_POS 1
#define L1_POS 2
#define R1_POS 3
#define L2_POS 4
#define R2_POS 5
#define P_POS  6
#define Q_POS  7
uint8_t PCMFrame[8][17];

int main(void)
{

  uint16_t L0_ = 0;
  uint16_t R0_ = 0b00000000000001;
  uint16_t L1_ = 0b00000000000010;
  uint16_t R1_ = 0b00000000000100;
  uint16_t L2_ = 0b00000000001000;
  uint16_t R2_ = 0b00000000010000;
  uint16_t P_  = 0b00000000011111;
  uint16_t Q   = 0b00000000100000;

  for (uint16_t ggg = 0; ggg < 255; ggg++){
    uint16_t L0 = 0b00000000000000;
    uint16_t R0 = 0b00000000000001;
    uint16_t L1 = 0b00000000000010;
    uint16_t R1 = 0b00000000000100;
    uint16_t L2 = 0b00000000001000;
    uint16_t R2 = 0b00000000010000;
    uint16_t P  = 0b00000000011111;
    //printf("%03d\n", ggg);
    if (ggg & 0x01) L0 = 0xfff;
    if (ggg & 0x02) R0 = 0xfff;
    if (ggg & 0x04) L1 = 0xfff;
    if (ggg & 0x08) R1 = 0xfff;
    if (ggg & 0x10) L2 = 0xfff;
    if (ggg & 0x20) R2 = 0xfff;
    if (ggg & 0x40) P = 0xfff;
    uint8_t j = 0;
    PCMFrame[0][16] = L0 != L0_;
    PCMFrame[1][16] = R0 != R0_;
    PCMFrame[2][16] = L1 != L1_;
    PCMFrame[3][16] = R1 != R1_;
    PCMFrame[4][16] = L2 != L2_;
    PCMFrame[5][16] = R2 != R2_;
    PCMFrame[6][16] = P != P_;

    uint8_t count = 0;
    for(int i = 0; i < 8; i++) count += PCMFrame[i][16];

    if (count != 2) continue;

    uint16_t Sp = L0 ^ R0 ^ L1 ^ R1 ^ L2 ^ R2 ^ P;
    uint16_t Sq = qCorTS(6, L0) ^ qCorTS(5, R0) ^ qCorTS(4, L1) ^ qCorTS(3, R1) ^ qCorTS(2, L2) ^ qCorTS(1, R2) ^ Q;

    if (PCMFrame[j + L0_POS][16] & PCMFrame[j + R0_POS][16]){ // L0 R0
      fprintf(stderr, "L0 R0");
      uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m5, Sq));
      uint16_t E2 = E1 ^ Sp;
      L0 ^= E1;
      R0 ^= E2;
    } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + L1_POS][16]){ // R0 L1
      fprintf(stderr, "R0 L1");
      uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m4, Sq));
      uint16_t E2 = E1 ^ Sp;
      R0 ^= E1;
      L1 ^= E2;
    } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + R1_POS][16]){ // L1 R1
      fprintf(stderr, "L1 R1");
      uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m3, Sq));
      uint16_t E2 = E1 ^ Sp;
      L1 ^= E1;
      R1 ^= E2;
    } else if (PCMFrame[j + R1_POS][16] & PCMFrame[j + L2_POS][16]){ // R1 L2
      fprintf(stderr, "R1 L2");
      uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m2, Sq));
      uint16_t E2 = E1 ^ Sp;
      R1 ^= E1;
      L2 ^= E2;
    } else if (PCMFrame[j + L2_POS][16] & PCMFrame[j + R2_POS][16]){ // L2 R2
      fprintf(stderr, "L2 R2");
      uint16_t E1 = qCorMul(T_I_m1, Sp ^ qCorMul(T_m1, Sq));
      uint16_t E2 = E1 ^ Sp;
      L2 ^= E1;
      R2 ^= E2;
    } else if (PCMFrame[j + R2_POS][16] & PCMFrame[j + P_POS][16]){ // R2 P
      fprintf(stderr, "R2 P");
      uint16_t E1 = qCorMul(T_m1, Sq);
      R2 ^= E1;
    } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + L1_POS][16]){ // L0 L1
      fprintf(stderr, "L0 L1");
      uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m4, Sq));
      uint16_t E2 = E1 ^ Sp;
      L0 ^= E1;
      L1 ^= E2;
    } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + R1_POS][16]){ // R0 R1
      fprintf(stderr, "R0 R1");
      uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m3, Sq));
      uint16_t E2 = E1 ^ Sp;
      R0 ^= E1;
      R1 ^= E2;
    } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + L2_POS][16]){ // L1 L2
      fprintf(stderr, "L1 L2");
      uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m2, Sq));
      uint16_t E2 = E1 ^ Sp;
      L1 ^= E1;
      L2 ^= E2;
    } else if (PCMFrame[j + R1_POS][16] & PCMFrame[j + R2_POS][16]){ // R1 R2
      fprintf(stderr, "R1 R2");
      uint16_t E1 = qCorMul(T2_I_m1, Sp ^ qCorMul(T_m1, Sq));
      uint16_t E2 = E1 ^ Sp;
      R1 ^= E1;
      R2 ^= E2;
    } else if (PCMFrame[j + L2_POS][16] & PCMFrame[j + P_POS][16]){ // L2 P
      fprintf(stderr, "L2 P");
      uint16_t E1 = qCorMul(T_m2, Sq);
      L2 ^= E1;
    } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + R1_POS][16]){ // L0 R1
      fprintf(stderr, "L0 R1");
      uint16_t E1 = qCorMul(T3_I_m1, Sp ^ qCorMul(T_m3, Sq));
      uint16_t E2 = E1 ^ Sp;
      L0 ^= E1;
      R1 ^= E2;
    } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + L2_POS][16]){ // R0 L2
      fprintf(stderr, "R0 L2");
      uint16_t E1 = qCorMul(T3_I_m1, Sp ^ qCorMul(T_m2, Sq));
      uint16_t E2 = E1 ^ Sp;
      R0 ^= E1;
      L2 ^= E2;
    } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + R2_POS][16]){ // L1 R2
      fprintf(stderr, "L1 R2");
      uint16_t E1 = qCorMul(T3_I_m1, Sp ^ qCorMul(T_m1, Sq));
      uint16_t E2 = E1 ^ Sp;
      L1 ^= E1;
      R2 ^= E2;
    } else if (PCMFrame[j + R1_POS][16] & PCMFrame[j + P_POS][16]){ // R1 P
      fprintf(stderr, "R1 P");
      uint16_t E1 = qCorMul(T_m3, Sq);
      R1 ^= E1;
    } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + L2_POS][16]){ // L0 L2
      fprintf(stderr, "L0 L2");
      uint16_t E1 = qCorMul(T4_I_m1, Sp ^ qCorMul(T_m2, Sq));
      uint16_t E2 = E1 ^ Sp;
      L0 ^= E1;
      L2 ^= E2;
    } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + R2_POS][16]){ // R0 R2
      fprintf(stderr, "R0 R2");
      uint16_t E1 = qCorMul(T4_I_m1, Sp ^ qCorMul(T_m1, Sq));
      uint16_t E2 = E1 ^ Sp;
      R0 ^= E1;
      R2 ^= E2;
    } else if (PCMFrame[j + L1_POS][16] & PCMFrame[j + P_POS][16]){ // L1 P
      fprintf(stderr, "L1 P");
      uint16_t E1 = qCorMul(T_m4, Sq);
      L1 ^= E1;
    } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + R2_POS][16]){ // L0 R2
      fprintf(stderr, "L0 R2");
      uint16_t E1 = qCorMul(T5_I_m1, Sp ^ qCorMul(T_m1, Sq));
      uint16_t E2 = E1 ^ Sp;
      L0 ^= E1;
      R2 ^= E2;
    } else if (PCMFrame[j + R0_POS][16] & PCMFrame[j + P_POS][16]){ // R0 P
      fprintf(stderr, "R0 P");
      uint16_t E1 = qCorMul(T_m5, Sq);
      R0 ^= E1;
    } else if (PCMFrame[j + L0_POS][16] & PCMFrame[j + P_POS][16]){ // L0 P
      fprintf(stderr, "L0 P");
      uint16_t E1 = qCorMul(T_m6, Sq);
      L0 ^= E1;
    }

    PCMFrame[0][16] = L0 != L0_;
    PCMFrame[1][16] = R0 != R0_;
    PCMFrame[2][16] = L1 != L1_;
    PCMFrame[3][16] = R1 != R1_;
    PCMFrame[4][16] = L2 != L2_;
    PCMFrame[5][16] = R2 != R2_;
    PCMFrame[6][16] = P != P_;

    count = 0;
    for(int i = 0; i < 8; i++) count += PCMFrame[i][16];

    if (count){
      fprintf(stderr, " - Repair error\n");
      /*
        printf("L0 %04x %04x\n", L0, L0_);
        printf("R0 %04x %04x\n", R0, R0_);
        printf("L1 %04x %04x\n", L1, L1_);
        printf("R1 %04x %04x\n", R1, R1_);
        printf("L2 %04x %04x\n", L2, L2_);
        printf("R2 %04x %04x\n", R2, R2_);
      */
    } else {
      fprintf(stderr, " - Repair complete\n");
    }
  }
  return 0;
}
