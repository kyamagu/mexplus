/** MxArray unit test.
 *
 * Kota Yamaguchi 2013 <kyamagu@cs.stonybrook.edu>
 */

#include <mexplus/mxarray.h>
#include <typeinfo>

using namespace std;
using mexplus::MxArray;

#define EXPECT(...) if (!(__VA_ARGS__)) \
    mexErrMsgIdAndTxt("test:MxArray", \
                      #__VA_ARGS__ " not true: %s:%d", \
                      __FILE__, \
                      __LINE__)
#define RUN_TEST(function) function(); \
    mexPrintf("PASS: %s\n", #function); \
    mexCallMATLAB(0, NULL, 0, NULL, "drawnow")

namespace {

template <typename T>
void testFundamentalScalar() {
  T value = 2, value2 = 0;
  MxArray array(value);
  EXPECT(array);
  EXPECT(array.size() == 1);
  EXPECT(array.to<T>() == value);
  EXPECT(array.at<T>(0) == value);
  value2 = 0;
  array.to<T>(&value2);
  EXPECT(value2 == value);
  value2 = 0;
  array.at<T>(0, &value2);
  EXPECT(value2 == value);
  array.set(0, 1);
  EXPECT(array.to<T>() == 1);
  EXPECT(array.at<T>(0) == 1);
  mxArray* plhs = MxArray::from(value);
  EXPECT(MxArray::at<T>(plhs, 0) == value);
  EXPECT(MxArray::to<T>(plhs) == value);
  value2 = 0;
  MxArray::to<T>(plhs, &value2);
  EXPECT(value2 == value);
  value2 = 0;
  MxArray::at<T>(plhs, 0, &value2);
  EXPECT(value2 == value);
  mxDestroyArray(plhs);
}

/** Check all fundamental type conversions.
 */
void testAllFundamentalScalar() {
  testFundamentalScalar<int8_t>();
  testFundamentalScalar<uint8_t>();
  testFundamentalScalar<int16_t>();
  testFundamentalScalar<uint16_t>();
  testFundamentalScalar<int32_t>();
  testFundamentalScalar<uint32_t>();
  testFundamentalScalar<int64_t>();
  testFundamentalScalar<uint64_t>();
  testFundamentalScalar<float>();
  testFundamentalScalar<double>();
  testFundamentalScalar<bool>();
  testFundamentalScalar<char>();
  testFundamentalScalar<short>();
  testFundamentalScalar<long>();
  testFundamentalScalar<size_t>();
}

template <typename T>
void testFundamentalVector() {
  vector<T> value(10);
  for (int i = 0; i < value.size(); ++i)
    value[i] = i;
  MxArray array(value);
  EXPECT(array);
  EXPECT(array.size() == value.size());
  vector<T> return_value;
  array.to<vector<T> >(&return_value);
  EXPECT(value.size() == return_value.size());
  for (int i = 0; i < value.size(); ++i) {
    EXPECT(value[i] == return_value[i]);
    EXPECT(value[i] == array.at<T>(i));
    array.set(i, i);
  }
  for (int i = 0; i < value.size(); ++i) {
    EXPECT(array.at<T>(i) == static_cast<T>(i));
  }
}

/** Check all fundamental vector conversions.
 */
void testAllFundamentalVector() {
  testFundamentalVector<int8_t>();
  testFundamentalVector<uint8_t>();
  testFundamentalVector<int16_t>();
  testFundamentalVector<uint16_t>();
  testFundamentalVector<int32_t>();
  testFundamentalVector<uint32_t>();
  testFundamentalVector<int64_t>();
  testFundamentalVector<uint64_t>();
  testFundamentalVector<float>();
  testFundamentalVector<double>();
  testFundamentalVector<bool>();
  testFundamentalVector<char>();
  testFundamentalVector<short>();
  testFundamentalVector<long>();
  testFundamentalVector<size_t>();
}

/** Check memory and reference management.
 */
void testMxArrayMemory() {
  MxArray empty;
  EXPECT(!empty);
  MxArray one(mxCreateDoubleScalar(1.0));
  EXPECT(one);
  EXPECT(one.to<double>() == 1.0);
  EXPECT(one.isOwner());
  MxArray moved_one(std::move(one));
  EXPECT(moved_one);
  EXPECT(moved_one.to<double>() == 1.0);
  EXPECT(moved_one.isOwner());
  EXPECT(!one.isOwner());
  EXPECT(!one);
  moved_one.swap(empty);
  EXPECT(!moved_one.isOwner());
  EXPECT(empty.isOwner());
  moved_one = std::move(empty);
  EXPECT(moved_one.isOwner());
  EXPECT(!empty.isOwner());
  one.reset(moved_one.release());
  EXPECT(one);
  EXPECT(one.isOwner());
  EXPECT(!moved_one);
  EXPECT(!moved_one.isOwner());
  MxArray another_one(one.get());
  EXPECT(another_one);
  EXPECT(another_one.to<double>() == 1.0);
  EXPECT(!another_one.isOwner());
  another_one.reset();
  EXPECT(!another_one);
  EXPECT(!another_one.isOwner());
  another_one.reset(one.clone());
  EXPECT(another_one);
  EXPECT(another_one.to<double>() == 1.0);
  EXPECT(another_one.isOwner());
  another_one.reset();
  EXPECT(!another_one);
  EXPECT(!another_one.isOwner());
}

/** Check string conversions.
 */
void testMxArrayString() {
  MxArray value("string value.");
  MxArray value2(string("another string."));
  vector<string> string_vector;
  string_vector.push_back("element1");
  string_vector.push_back("element2");
  string_vector.push_back("element3");
  vector<vector<string> > string_vector_vector;
  string_vector_vector.push_back(string_vector);
  string_vector_vector.push_back(string_vector);
  vector<vector<vector<string> > > nested_string;
  nested_string.push_back(string_vector_vector);
  nested_string.push_back(string_vector_vector);
  MxArray value3(string_vector);
  MxArray value4(nested_string);
  EXPECT(value);
  EXPECT(value2);
  EXPECT(value3);
  EXPECT(value4);
  EXPECT(value.isChar());
  EXPECT(value2.isChar());
  EXPECT(value3.isCell());
  EXPECT(value4.isCell());
  EXPECT(value.to<string>() == "string value.");
  EXPECT(value2.to<string>() == "another string.");
  vector<string> returned_value3 = value3.to<vector<string> >();
  EXPECT(returned_value3.size() == string_vector.size());
  for (int i = 0; i < string_vector.size(); ++i)
    EXPECT(returned_value3[i] == string_vector[i]);
  EXPECT(value4.to<vector<vector<vector<string> > > >().size() == 2);
  value.set(0, 'S');
  EXPECT(value.to<string>() == "String value.");
  value3.set(0, "Element1");
  EXPECT(value3.at<string>(0) == "Element1");
  value4.set(0, nested_string[0]);
}

/** Check cell array.
 */
void testMxArrayCell() {
  MxArray cell_array(MxArray::Cell(1, 2));
  EXPECT(!cell_array.at(0));
  EXPECT(!cell_array.at(1));
  cell_array.set(0, 10.1);
  cell_array.set(1, "text.");
  EXPECT(cell_array.at(0));
  EXPECT(cell_array.at(1));
}

/** Check struct array.
 */
void testMxArrayStruct() {
  const char* fields[] = {"field1", "field2", "field3"};
  MxArray struct_array(MxArray::Struct(
      sizeof(fields) / sizeof(char*), fields));
  EXPECT(!struct_array.at("field1"));
  struct_array.set("field1", 10.1);
  struct_array.set("field2", "text.");
  struct_array.set("field3", vector<double>(10, 2));
  struct_array.set("field4", "additional value.");
  EXPECT(struct_array.at<double>("field1") == 10.1);
  EXPECT(struct_array.at<string>("field2") == "text.");
  vector<double> vector_of_2 = struct_array.at<vector<double> >("field3");
  EXPECT(struct_array.at<string>("field4") == "additional value.");
  EXPECT(vector_of_2.size() == 10);
  vector<double>::const_iterator it;
  for (it = vector_of_2.begin(); it != vector_of_2.end(); ++it)
    EXPECT(*it == 2);
}

} // namespace

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  RUN_TEST(testAllFundamentalScalar);
  RUN_TEST(testAllFundamentalVector);
  RUN_TEST(testMxArrayMemory);
  RUN_TEST(testMxArrayString);
  RUN_TEST(testMxArrayCell);
  RUN_TEST(testMxArrayStruct);
}
