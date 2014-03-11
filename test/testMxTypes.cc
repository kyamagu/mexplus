/** MxArray unit test.
 *
 * Kota Yamaguchi 2013 <kyamagu@cs.stonybrook.edu>
 */

#include <mex.h>
#include <mexplus/mxtypes.h>
#include <stdint.h>

using namespace std;
using mexplus::MxTypes;
using mexplus::mxNumeric;
using mexplus::mxCell;

#define EXPECT(...) if (!(__VA_ARGS__)) \
    mexErrMsgTxt(#__VA_ARGS__ " not true.")
#define RUN_TEST(function) function(); \
    mexPrintf("PASS: %s\n", #function); \
    mexCallMATLAB(0, NULL, 0, NULL, "drawnow")

namespace {

/** Test type mapping.
 */
void testArrayType() {
  typedef struct FakeStruct_tag {} FakeStruct;
  EXPECT(is_same<MxTypes<int8_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<uint8_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<int16_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<uint16_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<int32_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<uint32_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<int64_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<uint64_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<float>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<double>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<short>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<long>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<long long>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<char>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<wchar_t>::array_type, mxChar>::value);
  // Visual Studio cannot distinguish char16_t and char32_t from uint.
  //EXPECT(is_same<MxTypes<char16_t>::array_type, mxChar>::value);
  //EXPECT(is_same<MxTypes<char32_t>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<mxChar>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<bool>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<mxLogical>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<mxLogical>::array_type, mxLogical>::value);
  // Visual Studio doesn't allow this.
  //EXPECT(is_same<MxTypes<void>::array_type, mxCell>::value);
  EXPECT(is_same<MxTypes<void*>::array_type, mxCell>::value);
  EXPECT(is_same<MxTypes<FakeStruct>::array_type, mxCell>::value);
  EXPECT(is_same<MxTypes<const int8_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const uint8_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const int16_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const uint16_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const int32_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const uint32_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const int64_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const uint64_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const float>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const double>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const short>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const long>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const long long>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const char>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<const wchar_t>::array_type, mxChar>::value);
  //EXPECT(is_same<MxTypes<const char16_t>::array_type, mxChar>::value);
  //EXPECT(is_same<MxTypes<const char32_t>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<const mxChar>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<const bool>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<const mxLogical>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<const mxLogical>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<const FakeStruct>::array_type, mxCell>::value);
}

} // namespace

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  RUN_TEST(testArrayType);
}
