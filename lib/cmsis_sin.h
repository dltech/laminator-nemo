#ifndef H_SIN
#define H_SIN

#include <inttypes.h>

#define FAST_MATH_TABLE_SIZE  512
#define FAST_MATH_Q31_SHIFT   (32 - 10)
#define BASEQ31 2147483648

/* This is ripped out sin function from CMSIS library input and output
   are in q31 format */

uint32_t arm_sin_q31(uint32_t x);

#endif
