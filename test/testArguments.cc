/** MEX function helper library.
 *
 * Copyright 2013 Kota Yamaguchi.
 */

#include <memory>
#include "mexplus/arguments.h"

#define EXPECT(condition) if (!(condition)) \
    mexErrMsgTxt(#condition " not true.")
#define RUN_TEST(function) function(); \
    mexPrintf("PASS: %s\n", #function); \
    mexCallMATLAB(0, NULL, 0, NULL, "drawnow")

using namespace std;
using mexplus::MxArray;
using mexplus::InputArguments;
using mexplus::OutputArguments;

namespace {

// Declare a memory-safe mxArray*. Note that Matlab automatically frees up
// mxArray* when MEX exits unless flagged persistent.
#define MAKE_VALUE(pointer) \
  shared_ptr<mxArray>(pointer, mxDestroyArray)
// Initialize vector<const mxArray*> rhs.
#define MAKE_RHS(rhs, ...) \
  const shared_ptr<mxArray> kFixture[] = { __VA_ARGS__ }; \
  const size_t kFixtureSize = sizeof(kFixture) / sizeof(shared_ptr<mxArray>); \
  vector<const mxArray*> rhs(kFixtureSize); \
  for (int i = 0; i < kFixtureSize; ++i) \
    rhs[i] = kFixture[i].get();
// Initialize vector<mxArray*> lhs.
#define MAKE_LHS(lhs, ...) \
  const shared_ptr<mxArray> kFixture[] = { __VA_ARGS__ }; \
  const size_t kFixtureSize = sizeof(kFixture) / sizeof(shared_ptr<mxArray>); \
  vector<mxArray*> lhs(kFixtureSize); \
  for (int i = 0; i < kFixtureSize; ++i) \
    lhs[i] = kFixture[i].get();

/** Test a single format input with default options.
 */
void testInputsSingleFormatOptionsDefault() {
  MAKE_RHS(
    rhs,
    MAKE_VALUE(mxCreateDoubleScalar(3.2)),
    MAKE_VALUE(mxCreateString("Text input."))
  );
  InputArguments input(rhs.size(), &rhs[0], 2, 2, "Option1", "Option2");
  EXPECT(input[0]);
  EXPECT(input[1]);
  EXPECT(input.get<double>(0) == 3.2);
  EXPECT(input.get<string>(1) == "Text input.");
  EXPECT(input.get<double>("Option1", -1) == -1);
  EXPECT(input.get<string>("Option2", "Option2 value.") == "Option2 value.");
}

/** Test a single format input with updated options.
 */
void testInputsSingleFormatOptionsUpdate() {
  MAKE_RHS(
    rhs,
    MAKE_VALUE(mxCreateDoubleScalar(3.2)),
    MAKE_VALUE(mxCreateString("Text input.")),
    MAKE_VALUE(mxCreateString("Option2")),
    MAKE_VALUE(mxCreateString("Option2 value.")),
    MAKE_VALUE(mxCreateString("Option1")),
    MAKE_VALUE(mxCreateDoubleScalar(10))
  );
  InputArguments input(rhs.size(), &rhs[0], 2, 2, "Option1", "Option2");
  EXPECT(input.is("default"));
  EXPECT(input.get<double>(0) == 3.2);
  EXPECT(input.get<string>(1) == "Text input.");
  EXPECT(input.get<double>("Option1", -1) == 10);
  EXPECT(input.get<string>("Option2", "Some value.") == "Option2 value.");
}

/** Test a single format input with struct options.
 */
void testInputsSingleFormatStructOptions() {
  MxArray options(MxArray::Struct());
  options.set("Option2", "Option2 value.");
  options.set("Option1", static_cast<double>(10));
  MAKE_RHS(
    rhs,
    MAKE_VALUE(mxCreateDoubleScalar(3.2)),
    MAKE_VALUE(mxCreateString("Text input.")),
    MAKE_VALUE(options.release())
  );
  InputArguments input(rhs.size(), &rhs[0], 2, 2, "Option1", "Option2");
  EXPECT(input.is("default"));
  EXPECT(input.get<double>(0) == 3.2);
  EXPECT(input.get<string>(1) == "Text input.");
  EXPECT(input.get<double>("Option1", -1) == 10);
  EXPECT(input.get<string>("Option2", "Some value.") == "Option2 value.");
}

/** Test a single format input with default options.
 */
void testInputsMultipleFormats() {
  MAKE_RHS(
    rhs,
    MAKE_VALUE(mxCreateDoubleScalar(3.2)),
    MAKE_VALUE(mxCreateString("Text input."))
  );
  InputArguments input;
  input.define("format1", 1, 2, "Option1", "Option2");
  input.define("format2", 2, 2, "Option1", "Option2");
  input.define("format3", 3);
  input.parse(rhs.size(), &rhs[0]);
  EXPECT(!input.is("format1"));
  EXPECT(input.is("format2"));
  EXPECT(!input.is("format3"));
  EXPECT(input.get<double>(0) == 3.2);
  EXPECT(input.get<string>(1) == "Text input.");
  EXPECT(input.get<double>("Option1", -1) == -1);
  EXPECT(input.get<string>("Option2", "Option2 value.") == "Option2 value.");
}

/** Test output arguments.
 */
void testOutputArguments() {
  MAKE_LHS(
    lhs,
    MAKE_VALUE(mxCreateDoubleScalar(3.2)),
    MAKE_VALUE(mxCreateString("Text input."))
  );
  OutputArguments output0(lhs.size() - 1, &lhs[0], 1);
  OutputArguments output1(lhs.size(), &lhs[0], 2, 1);
  OutputArguments output2(lhs.size() - 1, &lhs[0], 2, 1);
  output0.set(0, lhs[0]);
  output1.set(0, lhs[0]);
  output1.set(1, lhs[1]);
  output2.set(0, lhs[0]);
  output2.set(1, lhs[1]);
  EXPECT(output0[0]);
  EXPECT(output1[0]);
  EXPECT(output1[1]);
  EXPECT(output2[0]);
  output0[0] = lhs[0];
}

}  // namespace

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  RUN_TEST(testInputsSingleFormatOptionsDefault);
  RUN_TEST(testInputsSingleFormatOptionsUpdate);
  RUN_TEST(testInputsSingleFormatStructOptions);
  RUN_TEST(testInputsMultipleFormats);
  RUN_TEST(testOutputArguments);
}
