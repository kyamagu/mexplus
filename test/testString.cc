/** Testing string conversion.
 *
 * Copyright 2013 Kota Yamaguchi.
 */

#include "mexplus/mxarray.h"

using namespace std;

void mexFunction(int nlhs,
                 mxArray* plhs[],
                 int nrhs,
                 const mxArray* prhs[]) {
  string value;
  mexplus::MxArray::to<string>(prhs[0], &value);
  plhs[0] = mexplus::MxArray::from(value);
}
