/** Testing string conversion.
 *
 * Kota Yamaguchi 2013 <kyamagu@cs.stonybrook.edu>
 */

#include <mexplus/mxarray.h>

using namespace std;

void mexFunction(int nlhs,
                 mxArray* plhs[],
                 int nrhs,
                 const mxArray* prhs[]) {
  string value;
  mexplus::MxArray::to<string>(prhs[0], &value);
  plhs[0] = mexplus::MxArray::from(value);
}
