/** MEX dispatch helper library.
 *
 * Kota Yamaguchi 2013 <kyamagu@cs.stonybrook.edu>
 */

#include <mexplus/dispatch.h>

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

} // namespace

MEX_DISPATCH
