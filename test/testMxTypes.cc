/** MxArray unit test.
 *
 * Copyright 2013 Kota Yamaguchi.
 */

#include <mex.h>
#include <cstdint>
#include <complex>
#include <string>
#include <vector>
#include "mexplus/mxtypes.h"

using namespace std;
using mexplus::MxTypes;
using mexplus::MxCharCompound;
using mexplus::MxArithmeticType;
using mexplus::MxArithmeticCompound;
using mexplus::MxComplexType;
using mexplus::MxComplexCompound;
using mexplus::MxComplexOrArithmeticType;
using mexplus::MxComplexOrArithmeticCompound;
using mexplus::mxNumeric;
using mexplus::mxCell;
using mexplus::mxComplex;

#define EXPECT(...) if (!(__VA_ARGS__)) \
    mexErrMsgTxt(#__VA_ARGS__ " not true.")
#define RUN_TEST(function) function(); \
    mexPrintf("PASS: %s\n", #function); \
    mexCallMATLAB(0, NULL, 0, NULL, "drawnow")

namespace {

typedef struct FakeStruct_tag {} FakeStruct;

/** Test type mapping.
 */
void testArrayType() {
  // gcc 4.4.7 doesn't accept local types as template argument.
  EXPECT(is_same<MxTypes<int8_t>::array_type, mxNumeric>::value);
  // Not true with gcc 4.8.4.
  // EXPECT(is_same<MxTypes<uint8_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<int16_t>::array_type, mxNumeric>::value);
  // Not true with gcc 4.4.7.
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
  // EXPECT(is_same<MxTypes<char16_t>::array_type, mxChar>::value);
  // EXPECT(is_same<MxTypes<char32_t>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<mxChar>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<bool>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<mxLogical>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<mxLogical>::array_type, mxLogical>::value);
  // Visual Studio doesn't allow this.
  // EXPECT(is_same<MxTypes<void>::array_type, mxCell>::value);
  EXPECT(is_same<MxTypes<void*>::array_type, mxCell>::value);
  EXPECT(is_same<MxTypes<FakeStruct>::array_type, mxCell>::value);
  EXPECT(is_same<MxTypes<std::complex<float>>::array_type, mxComplex>::value);
  EXPECT(is_same<MxTypes<std::complex<double>>::array_type, mxComplex>::value);
  EXPECT(!is_arithmetic<std::complex<float>>::value);
  EXPECT(!is_arithmetic<std::complex<double>>::value);
  EXPECT(is_compound<std::complex<double>>::value);
  EXPECT(MxArithmeticType<int8_t>::value);
  EXPECT(MxArithmeticCompound<std::vector<float>>::value);
  EXPECT(MxCharCompound<string>::value);
  EXPECT(!MxCharCompound<char*>::value);
  EXPECT(MxArithmeticType<double>::value);
  EXPECT(MxComplexCompound<vector<complex<double>>>::value);
  EXPECT(MxArithmeticCompound<vector<double>>::value);
  EXPECT(MxArithmeticCompound<vector<int>>::value);
  EXPECT(!MxArithmeticCompound<vector<char>>::value);
  EXPECT(MxComplexType<std::complex<double>>::value);
  EXPECT(!MxComplexType<double>::value);
  EXPECT(MxComplexOrArithmeticType<double>::value);
  EXPECT(!MxComplexOrArithmeticType<char>::value);
  EXPECT(MxComplexOrArithmeticType<std::complex<double>>::value);
  EXPECT(!MxComplexOrArithmeticType<vector<double>>::value);
  EXPECT(!MxComplexOrArithmeticType<vector<std::complex<double>>>::value);
  EXPECT(!MxComplexOrArithmeticType<string>::value);
  EXPECT(!MxComplexOrArithmeticType<vector<string>>::value);
  EXPECT(MxArithmeticCompound<vector<double>>::value);
  EXPECT(MxComplexOrArithmeticCompound<vector<std::complex<float>>>::value);
  EXPECT(!MxComplexOrArithmeticCompound<string>::value);
  EXPECT(!MxComplexOrArithmeticCompound<char>::value);
  EXPECT(!MxComplexOrArithmeticCompound<std::complex<double>>::value);
  EXPECT(!MxComplexOrArithmeticCompound<vector<string>>::value);

  EXPECT(is_same<MxTypes<const int8_t>::array_type, mxNumeric>::value);
  // Not true with gcc 4.8.4.
  // EXPECT(is_same<MxTypes<const uint8_t>::array_type, mxNumeric>::value);
  EXPECT(is_same<MxTypes<const int16_t>::array_type, mxNumeric>::value);
  // Not true with gcc 4.4.7.
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
  // EXPECT(is_same<MxTypes<const char16_t>::array_type, mxChar>::value);
  // EXPECT(is_same<MxTypes<const char32_t>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<const mxChar>::array_type, mxChar>::value);
  EXPECT(is_same<MxTypes<const bool>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<const mxLogical>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<const mxLogical>::array_type, mxLogical>::value);
  EXPECT(is_same<MxTypes<const FakeStruct>::array_type, mxCell>::value);
  EXPECT(is_same<MxTypes<const std::complex<float> >::array_type,
                 mxComplex>::value);
  EXPECT(is_same<MxTypes<const std::complex<double>>::array_type,
                 mxComplex>::value);
  EXPECT(!is_arithmetic<const std::complex<float> >::value);
  EXPECT(!is_arithmetic<const std::complex<double> >::value);
}

}  // namespace

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  RUN_TEST(testArrayType);
}
