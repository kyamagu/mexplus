/** MEX dispatch helper library.
 *
 * Copyright 2013 Kota Yamaguchi.
 */

#include "mexplus/dispatch.h"

#define EXPECT(condition) if (!(condition)) \
    mexErrMsgTxt(#condition " not true.")
#define PRINTF(...) mexPrintf(__VA_ARGS__); \
    mexCallMATLAB(0, NULL, 0, NULL, "drawnow")

namespace {

MEX_DEFINE(foo) (int nlhs,
                 mxArray* plhs[],
                 int nrhs,
                 const mxArray* prhs[]) {
}

MEX_DEFINE(bar) (int nlhs,
                 mxArray* plhs[],
                 int nrhs,
                 const mxArray* prhs[]) {
}

}  // namespace

MEX_DISPATCH
