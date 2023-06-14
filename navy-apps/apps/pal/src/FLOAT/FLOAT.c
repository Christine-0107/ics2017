#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  //结果除以2^16
  FLOAT res = ((int64_t)a * (int64_t)b) >> 16;
  return res;
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  //1.先不管符号位，当作正数计算。
  assert(b!=0);
  FLOAT aa = Fabs(a);
  FLOAT bb = Fabs(b);
  //2.计算整数部分的商和余数。
  FLOAT res = aa / bb;
  FLOAT remain = aa % bb;
  //3.计算小数部分的商
  for(int i=0;i<16;i++){
    remain << 1;
    res << 1;
    if(remain >= bb){
      remain -= bb;
      res++;
    }
  }
  //4.添加符号位
  if(((a ^ b) & 0x80000000) == 0x80000000){
    res = -res;
  }
  return res;
}

//定义IEEE 754单精度浮点数的表示形式
union floatu {
  struct{
    uint32_t man : 23; //尾数部分
    uint32_t exp : 8; //指数部分
    uint32_t sign : 1; //符号
  };
  uint32_t value;
};

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */
  union floatu f;
  f.value = *((uint32_t*)(void*)&a);
  int exp = f.exp - 127; //IEEE 754单精度浮点数保存时有偏移值127
  FLOAT res = 0;
  assert(exp!=128);
  if(exp >= 0){
    int mov = 7 - exp;
    if(mov >= 0){
      res = (f.man | (1<<23)) >> mov;
    }
    else{
      res = (f.man | (1<<23)) << (-mov);
    }
    return f.sign == 0 ? res : -res;
  }
  return 0;
}

FLOAT Fabs(FLOAT a) {
  if((a & 0x80000000) == 0){
    return a;
  }
  else{
    return -a;
  }
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}
