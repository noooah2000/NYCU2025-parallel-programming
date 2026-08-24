#include "PPintrin.h"

// implementation of absSerial(), but it is vectorized using PP intrinsics
void absVector(float *values, float *output, int N)
{
  __pp_vec_float x;
  __pp_vec_float result;
  __pp_vec_float zero = _pp_vset_float(0.f);
  __pp_mask maskActive, maskIsNegative, maskIsNotNegative;

  //  Note: Take a careful look at this loop indexing.  This example
  //  code is not guaranteed to work when (N % VECTOR_WIDTH) != 0.
  //  Why is that the case?
  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    int remain = N - i;
    if (remain < VECTOR_WIDTH) {
      maskActive = _pp_init_ones(remain);
    } else {
      // All ones
      maskActive = _pp_init_ones();
    }

    // All zeros
    maskIsNegative = _pp_init_ones(0);

    // Load vector of values from contiguous memory addresses
    _pp_vload_float(x, values + i, maskActive); // x = values[i];

    // Set mask according to predicate
    _pp_vlt_float(maskIsNegative, x, zero, maskActive); // if (x < 0) {

    // Execute instruction using mask ("if" clause)
    _pp_vsub_float(result, zero, x, maskIsNegative); //   output[i] = -x;

    // Inverse maskIsNegative to generate "else" mask
    maskIsNotNegative = _pp_mask_not(maskIsNegative); // } else {

    // Execute instruction ("else" clause)
    _pp_vload_float(result, values + i, maskIsNotNegative); //   output[i] = x; }

    // Write results back to memory
    _pp_vstore_float(output + i, result, maskActive);
  }
}

void clampedExpVector(float *values, int *exponents, float *output, int N)
{
  //
  // PP STUDENTS TODO: Implement your vectorized version of
  // clampedExpSerial() here.
  //
  // Your solution should work for any value of
  // N and VECTOR_WIDTH, not just when VECTOR_WIDTH divides N
  //
  __pp_vec_float x;
  __pp_vec_int y;
  __pp_vec_float result;
  __pp_vec_float maxval = _pp_vset_float(9.999999f);
  __pp_vec_int ZERO = _pp_vset_int(0), 
                ONE = _pp_vset_int(1);
  __pp_mask maskActive, maskIsPositive, maskIsOverMax;

  for (int i = 0; i < N; i += VECTOR_WIDTH) 
  {
    int remain = N - i;
    if (remain < VECTOR_WIDTH) {
      maskActive = _pp_init_ones(remain);
    } else {
      // All ones
      maskActive = _pp_init_ones();
    }

    _pp_vload_float(x, values + i, maskActive);
    _pp_vload_int(y, exponents + i, maskActive);

    result = _pp_vset_float(1.f); // res 初始為 1
    _pp_vgt_int(maskIsPositive, y, ZERO, maskActive); // 有哪些lane還沒乘完

    while (_pp_cntbits(maskIsPositive)) {
      _pp_vmult_float(result, result, x, maskIsPositive); // res = res * x
      _pp_vsub_int(y, y, ONE, maskIsPositive); // exp = exp - 1
      _pp_vgt_int(maskIsPositive, y, ZERO, maskActive); // 有哪些lane還沒乘完
    } 

    maskIsOverMax = _pp_init_ones(0);
    _pp_vgt_float(maskIsOverMax, result, maxval, maskActive); // if res > 9.999999
    _pp_vset_float(result, 9.999999f, maskIsOverMax); // res = 9.999999

    _pp_vstore_float(output + i, result, maskActive);
  }
}

// returns the sum of all elements in values
// You can assume N is a multiple of VECTOR_WIDTH
// You can assume VECTOR_WIDTH is a power of 2
float arraySumVector(float *values, int N)
{
  __pp_vec_float x;
  __pp_vec_float result = _pp_vset_float(0.f);
  __pp_mask maskActive, maskFirstHalf;
  //
  // PP STUDENTS TODO: Implement your vectorized version of arraySumSerial here
  //

  for (int i = 0; i < N; i += VECTOR_WIDTH)
  {
    maskActive = _pp_init_ones();
    _pp_vload_float(x, values + i, maskActive); // x = values[i];
    _pp_vadd_float(result, result, x, maskActive);
  }

  maskFirstHalf = _pp_init_ones(VECTOR_WIDTH / 2);
  for (int i = VECTOR_WIDTH-1; i >= 1; i /= 2)
  {
    _pp_hadd_float(result, result);
    _pp_interleave_float(result, result);
    _pp_vset_float(result, 0.f, maskFirstHalf);
  }

  return result.value[VECTOR_WIDTH-1];
}