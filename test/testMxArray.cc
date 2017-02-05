/** MxArray unit test.
 *
 * Copyright 2013 Kota Yamaguchi.
 */

#include <typeinfo>
#include "mexplus/mxarray.h"

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
  // EXPECT(mexplus::MxArithmeticType<T>::value);
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

template <typename S>
void testComplex() {
  typedef std::complex<S> T;
  T value(+1.1f, -3.4f), value2(0.0f, 0.0f), value3(+2.2f, -5.6f);
  typedef std::vector<T> ComplexVector;
  MxArray array(value);
  EXPECT(array);
  EXPECT(array.size() == 1);
  EXPECT(array.to<T>() == value);
  EXPECT(array.at<T>(0) == value);
  value2 = T(0.0f, 0.0f);
  array.to<T>(&value2);
  EXPECT(value2 == value);
  array.at<T>(0, &value2);
  EXPECT(value2 == value);
  array.set(0, value3);
  EXPECT(array.to<T>() == value3);
  EXPECT(array.at<T>(0) == value3);
  mxArray* plhs = MxArray::from(value);
  EXPECT(MxArray::at<T>(plhs, 0) == value);
  EXPECT(MxArray::to<T>(plhs) == value);
  value2 = T(0.0f, 0.0f);
  MxArray::to<T>(plhs, &value2);
  EXPECT(value2 == value);
  value2 = T(0.0f, 0.0f);
  MxArray::at<T>(plhs, 0, &value2);
  EXPECT(value2 == value);

  ComplexVector a, b(2);
  b[0] = T(1.1f, 2.2f);
  b[1] = T(3.3f, 4.4f);

  array = MxArray(b);
  a = b;
  EXPECT(a == b);
  EXPECT(mexplus::MxComplexOrArithmeticCompound<std::vector<S> >::value);
  std::vector<S> magnitude = array.to<std::vector<S> >();
  EXPECT(magnitude.size() == b.size());
  for (int i = 0; i < magnitude.size(); ++i) {
    EXPECT(abs(magnitude[i] - sqrt(
        a[i].real() * a[i].real() + a[i].imag() * a[i].imag())) < 1e-09);
  }
  mxDestroyArray(plhs);
}

void testAllComplex(){
  testComplex<float>();
  testComplex<double>();
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
  cell_array = MxArray::Cell(1, 2);
  EXPECT(!cell_array.at(0));
  EXPECT(!cell_array.at(1));
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
  struct_array = MxArray::Struct(sizeof(fields) / sizeof(char*), fields);
  EXPECT(!struct_array.at("field1"));
}

}  // namespace

/** Custom cell object for conversion test.
 */
struct MyCellObject {
  string name;
  vector<float> value;
};

/** Custom struct object for conversion test.
 */
struct MyStructObject {
  string name;
  vector<float> value;
};

namespace mexplus {

template <>
mxArray* MxArray::from(const MyCellObject& object) {
  MxArray cell_array(MxArray::Cell(1, 2));
  cell_array.set(0, object.name);
  cell_array.set(1, object.value);
  return cell_array.release();
}

template <>
void MxArray::to(const mxArray* array, MyCellObject* object) {
  MxArray cell_array(array);
  object->name = cell_array.at<string>(0);
  object->value = cell_array.at<vector<float> >(1);
}

template <>
mxArray* MxArray::from(const MyStructObject& object) {
  MxArray struct_array(MxArray::Struct());
  struct_array.set("name", object.name);
  struct_array.set("value", object.value);
  return struct_array.release();
}

template <>
void MxArray::to(const mxArray* array, MyStructObject* object) {
  MxArray struct_array(array);
  object->name = struct_array.at<string>("name");
  object->value = struct_array.at<vector<float> >("value");
}

}  // namespace mexplus

namespace {

void testCustomCell() {
  MyCellObject object, object2;
  object.name = "foo";
  object.value = vector<float>(10, 1);
  MxArray array(MxArray::from(object));
  EXPECT(array);
  EXPECT(array.isCell());
  EXPECT(array.at<string>(0) == "foo");
  EXPECT(array.at<vector<float> >(1).size() == 10);
  MxArray::to(array.get(), &object2);
  EXPECT(object.name == object2.name);
  EXPECT(object.value.size() == object2.value.size());
}

void testCustomStruct() {
  MyStructObject object, object2;
  object.name = "foo";
  object.value = vector<float>(10, 1);
  MxArray array(MxArray::from(object));
  EXPECT(array);
  EXPECT(array.isStruct());
  EXPECT(array.at<string>("name") == "foo");
  EXPECT(array.at<vector<float> >("value").size() == 10);
  MxArray::to(array.get(), &object2);
  EXPECT(object.name == object2.name);
  EXPECT(object.value.size() == object2.value.size());
}

}  // namespace

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[]) {
  RUN_TEST(testAllFundamentalScalar);
  RUN_TEST(testAllFundamentalVector);
  RUN_TEST(testAllComplex);
  RUN_TEST(testMxArrayMemory);
  RUN_TEST(testMxArrayString);
  RUN_TEST(testMxArrayCell);
  RUN_TEST(testMxArrayStruct);
  RUN_TEST(testCustomStruct);
  RUN_TEST(testCustomCell);
}
