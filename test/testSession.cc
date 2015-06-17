/** MEX function helper library.
 *
 * Copyright 2013 Kota Yamaguchi.
 */

#include <cstdint>
#include "mexplus/dispatch.h"

using namespace std;

/** Hypothetical dummy class for testing purpose.
 */
class HypotheticalClass {
};

template class mexplus::Session<HypotheticalClass>;

namespace {

template <typename T>
inline T ConvertToNumeric(const mxArray* pointer) {
  return *reinterpret_cast<T*>(mxGetData(pointer));
}

template <typename T>
mxArray* ConvertFromNumeric(T value) {
  mxArray* pointer = mxCreateNumericMatrix(1, 1, mxINT64_CLASS, mxREAL);
  if (!pointer)
    mexErrMsgTxt("Null pointer exception.");
  *reinterpret_cast<int64_t*>(mxGetData(pointer)) = value;
  return pointer;
}

typedef mexplus::Session<HypotheticalClass> HypotheticalObjects;

MEX_DEFINE(create) (int nlhs,
                    mxArray* plhs[],
                    int nrhs,
                    const mxArray* prhs[]) {
  plhs[0] = ConvertFromNumeric<int64_t>(
      HypotheticalObjects::create(new HypotheticalClass()));
}

MEX_DEFINE(get) (int nlhs,
                 mxArray* plhs[],
                 int nrhs,
                 const mxArray* prhs[]) {
  if (nrhs < 1)
    mexErrMsgTxt("Expected an object id input.");
  HypotheticalClass* object = HypotheticalObjects::get(
      ConvertToNumeric<int64_t>(prhs[0]));
  if (!object)
    mexErrMsgTxt("Session::get returned a NULL pointer.");
}

MEX_DEFINE(exist) (int nlhs,
                   mxArray* plhs[],
                   int nrhs,
                   const mxArray* prhs[]) {
  if (nrhs < 1)
    mexErrMsgTxt("Expected an object id input.");
  plhs[0] = mxCreateLogicalScalar(
      HypotheticalObjects::exist(ConvertToNumeric<int64_t>(prhs[0])));
}

MEX_DEFINE(destroy) (int nlhs,
                     mxArray* plhs[],
                     int nrhs,
                     const mxArray* prhs[]) {
  if (nrhs < 1)
    mexErrMsgTxt("Expected an object id input.");
  HypotheticalObjects::destroy(ConvertToNumeric<int64_t>(prhs[0]));
}

MEX_DEFINE(clear) (int nlhs,
                     mxArray* plhs[],
                     int nrhs,
                     const mxArray* prhs[]) {
  HypotheticalObjects::clear();
}

}  // namespace

MEX_DISPATCH
