#ifndef __FLOAT_H__
#define __FLOAT_H__

#include "assert.h"

typedef int FLOAT;

static inline int F2int(FLOAT a) {
  //取整数位即可
  int res = a & 0xffff0000;
  res = res >> 16;
  return res;
}

static inline FLOAT int2F(int a) {
  //整数放在高16位即可
  return a << 16;
}

static inline FLOAT F_mul_int(FLOAT a, int b) {
  assert(0);
  return 0;
}

static inline FLOAT F_div_int(FLOAT a, int b) {
  assert(0);
  return 0;
}

FLOAT f2F(float);
FLOAT F_mul_F(FLOAT, FLOAT);
FLOAT F_div_F(FLOAT, FLOAT);
FLOAT Fabs(FLOAT);
FLOAT Fsqrt(FLOAT);
FLOAT Fpow(FLOAT, FLOAT);

#endif
