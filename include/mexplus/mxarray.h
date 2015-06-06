/** MxArray data conversion library.
 *
 * The library provides mexplus::MxArray class for data conversion between
 * mxArray* and C++ types. The static API's are the core of the high-level
 * conversions.
 *
 *    int value = MxArray::to<int>(prhs[0]);
 *    string value = MxArray::to<string>(prhs[0]);
 *    vector<double> value = MxArray::to<vector<double> >(prhs[0]);
 *
 *    plhs[0] = MxArray::from(20);
 *    plhs[0] = MxArray::from("text value.");
 *    plhs[0] = MxArray::from(vector<double>(20, 0));
 *
 * Additionally, object API's are there to wrap around a complicated data
 * access.
 *
 * ### Read access
 *
 *    MxArray cell(prhs[0]);   // Assumes cell array in prhs[0].
 *    int x = cell.at<int>(0);
 *    vector<double> y = cell.at<vector<double> >(1);
 *
 *    MxArray numeric(prhs[0]);   // Assumes numeric array in prhs[0].
 *    double x = numeric.at<double>(0);
 *    int y = numeric.at<int>(1);
 *
 * ### Write access
 *
 *    MxArray cell(MxArray::Cell(1, 3));
 *    cell.set(0, 12);
 *    cell.set(1, "text value.");
 *    cell.set(2, vector<double>(4, 0));
 *    plhs[0] = cell.release();
 *
 *    MxArray numeric(MxArray::Numeric<double>(2, 2));
 *    numeric.set(0, 0, 1);
 *    numeric.set(0, 1, 2);
 *    numeric.set(1, 0, 3);
 *    numeric.set(1, 1, 4);
 *    plhs[0] = numeric.release();
 *
 * To add your own data conversion, define in namespace mexplus a template
 * specialization of MxArray::from() and MxArray::to().
 *
 * Kota Yamaguchi 2014 <kyamagu@cs.stonybrook.edu>
 */

#ifndef __MEXPLUS_MXARRAY_H__
#define __MEXPLUS_MXARRAY_H__

#include <mex.h>
#include <mexplus/mxtypes.h>
#include <cassert>
#include <stdint.h>
#include <string>
#include <typeinfo>
#include <vector>
#include <algorithm>

/** Macro definitions.
 */
#define MEXPLUS_CHECK_NOTNULL(pointer) \
    if (!(pointer)) \
      mexErrMsgIdAndTxt("mexplus:error", \
                        "Null pointer exception: %s:%d:%s `" #pointer "`.", \
                        __FILE__, \
                        __LINE__, \
                        __FUNCTION__)

#define MEXPLUS_ERROR(...) mexErrMsgIdAndTxt("mexplus:error", __VA_ARGS__)
#define MEXPLUS_WARNING(...) mexWarnMsgIdAndTxt("mexplus:warning", __VA_ARGS__)
#define MEXPLUS_ASSERT(condition, ...) \
    if (!(condition)) mexErrMsgIdAndTxt("mexplus:error", __VA_ARGS__)

namespace mexplus {

/** mxArray object wrapper for data conversion and manipulation.
 *
 * The class is similar to a combination of unique_ptr and wrapper around
 * Matlab's matrix API. An MxArray object created from a mutable mxArray*
 * pointer automatically frees its internal memory unless explicitly
 * released. When MxArray is created from a const mxArray*, the object does not
 * manage memory but still provides the same matrix API.
 */
class MxArray {
public:
  /** Empty MxArray constructor. Use reset() to set a pointer.
   */
  MxArray() : array_(NULL), owner_(false) {}
  /** NULL assignment.
   */
  MxArray& operator= (std::nullptr_t) {
    reset();
    return *this;
  }
  /** Move constructor.
   */
  MxArray(MxArray&& array) : array_(NULL), owner_(false) {
    *this = std::move(array);
  }
  /** Move assignment.
   */
  MxArray& operator= (MxArray&& rhs) {
    if (this != &rhs) {
      array_ = rhs.array_;
      owner_ = rhs.owner_;
      rhs.array_ = NULL;
      rhs.owner_ = false;
    }
    return *this;
  }
  /** MxArray constructor from mutable mxArray*. MxArray will manage memory.
   * @param array mxArray pointer.
   */
  explicit MxArray(const mxArray* array) :
      array_(const_cast<mxArray*>(array)),
      owner_(false) {}
  /** MxArray constructor from const mxArray*. MxArray will not manage memory.
   * @param array mxArray pointer given by mexFunction.
   */
  explicit MxArray(mxArray* array) : array_(array), owner_(array) {}
  /** MxArray constructor from scalar.
   */
  template <typename T>
  explicit MxArray(const T& value) : array_(from(value)), owner_(true) {}
  /** Destructor. Unreleased pointers will be destroyed.
   */
  virtual ~MxArray() {
    if (array_ && owner_)
      mxDestroyArray(array_);
  }
  /** Swap operation.
   */
  void swap(MxArray& rhs)  {
    if (this != &rhs) {
      mxArray* array = rhs.array_;
      bool owner = rhs.owner_;
      rhs.array_ = array_;
      rhs.owner_ = owner_;
      array_ = array;
      owner_ = owner;
    }
  }
  /** Reset an mxArray to a const mxArray*.
   *
   * Caller must be VERY careful with this, as the behavior is undefined when
   * the original mxArray* is destroyed. For example, the following will crash.
   * @code
   *     MxArray foo;
   *     {
   *       MxArray bar(1);
   *       foo.reset(bar.get());
   *     }
   *     foo.toInt(); // Error!
   * @endcode
   */
  void reset(const mxArray* array = NULL) {
    if (array_ && owner_)
      mxDestroyArray(array_);
    array_ = const_cast<mxArray*>(array);
    owner_ = false;
  }
  /** Reset an mxArray.
   */
  void reset(mxArray* array) {
    if (array_ && owner_)
      mxDestroyArray(array_);
    array_ = array;
    owner_ = array;
  }
  /** Release managed mxArray* pointer, or clone if not owner.
   * @return Unmanaged mxArray*. Always caller must destroy.
   */
  mxArray* release()  {
    MEXPLUS_CHECK_NOTNULL(array_);
    mxArray* array = (owner_) ? array_ : clone();
    array_ = NULL;
    owner_ = false;
    return array;
  }
  /** Clone mxArray. This always allocates new mxArray*.
   * @return Unmanaged mxArray*. Always caller must destroy.
   */
  mxArray* clone() const {
    MEXPLUS_CHECK_NOTNULL(array_);
    mxArray* array = mxDuplicateArray(array_);
    MEXPLUS_CHECK_NOTNULL(array);
    return array;
  }
  /** Conversion to const mxArray*.
   * @return const mxArray* pointer.
   */
  inline const mxArray* get() const { return array_; }
  /** Return true if the array is not NULL.
   */
  operator bool() const { return array_ != NULL; }
  /** Return true if owner.
   */
  inline bool isOwner() const { return owner_; }
  /** Create a new numeric (real or complex) matrix.
   * @param rows Number of rows.
   * @param columns Number of cols.
   */
  template <typename T>
  static mxArray* Numeric(int rows = 1, int columns = 1);
  /** Create a new logical matrix.
   * @param rows Number of rows.
   * @param columns Number of cols.
   */
  static mxArray* Logical(int rows = 1, int columns = 1) {
    mxArray* logical_array = mxCreateLogicalMatrix(rows, columns);
    MEXPLUS_CHECK_NOTNULL(logical_array);
    return logical_array;
  }
  /** Create a new cell matrix.
   * @param rows Number of rows.
   * @param columns Number of cols.
   *
   * Example:
   * @code
   *     MxArray cell_array = MxArray::Cell(1, 2);
   *     cell_array.set(0, 1);
   *     cell_array.set(1, "another value");
   *     plhs[0] = cell_array.release();
   * @endcode
   */
  static mxArray* Cell(int rows = 1, int columns = 1) {
    mxArray* cell_array = mxCreateCellMatrix(rows, columns);
    MEXPLUS_CHECK_NOTNULL(cell_array);
    return cell_array;
  }
  /** Generic constructor for a struct matrix.
   * @param fields field names.
   * @param nfields number of field names.
   * @param rows size of the first dimension.
   * @param columns size of the second dimension.
   *
   * Example:
   * @code
   *     const char* fields[] = {"field1", "field2"};
   *     MxArray struct_array(MxArray::Struct(2, fields));
   *     struct_array.set("field1", 1);
   *     struct_array.set("field2", "field2 value");
   *     plhs[0] = struct_array.release();
   * @endcode
   */
  static mxArray* Struct(int nfields = 0,
                         const char** fields = NULL,
                         int rows = 1,
                         int columns = 1) {
    mxArray* struct_array = mxCreateStructMatrix(rows,
                                                 columns,
                                                 nfields,
                                                 fields);
    MEXPLUS_CHECK_NOTNULL(struct_array);
    return struct_array;
  }
  /** mxArray* importer methods.
   */
  template <typename T>
  static mxArray* from(const T& value) { return fromInternal<T>(value); }
  static mxArray* from(const char* value) {
    mxArray* array = mxCreateString(value);
    MEXPLUS_CHECK_NOTNULL(array);
    return array;
  }
  static mxArray* from(int32_t value) {
    mxArray* array = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
    MEXPLUS_CHECK_NOTNULL(array);
    *reinterpret_cast<int32_t*>(mxGetData(array)) = value;
    return array;
  }
  /** mxArray* exporter methods.
   */
  template <typename T>
  static void to(const mxArray* array, T* value) {
    toInternal<T>(array, value);
  }
  template <typename T>
  static T to(const mxArray* array) {
    T value;
    toInternal<T>(array, &value);
    return value;
  }
  /** mxArray* element reader methods.
   */
  template <typename T>
  static T at(const mxArray* array, mwIndex index) {
    T value;
    atInternal<T>(array, index, &value);
    return value;
  }
  template <typename T>
  static void at(const mxArray* array, mwIndex index, T* value) {
    atInternal<T>(array, index, value);
  }
  static const mxArray* at(const mxArray* array, mwIndex index) {
    MEXPLUS_CHECK_NOTNULL(array);
    MEXPLUS_ASSERT(mxIsCell(array), "Expected a cell array.");
    return mxGetCell(array, index);
  }
  template <typename T>
  static void at(const mxArray* array,
                 const std::string& field,
                 T* value,
                 mwIndex index = 0) {
    atInternal<T>(array, field, index, value);
  }
  static const mxArray* at(const mxArray* array,
                           const std::string& field,
                           mwIndex index = 0)  {
    MEXPLUS_CHECK_NOTNULL(array);
    MEXPLUS_ASSERT(mxIsStruct(array), "Expected a struct array.");
    return mxGetField(array, index, field.c_str());
  }
  /** mxArray* element writer methods.
   */
  template <typename T>
  static void set(mxArray* array, mwIndex index, const T& value) {
    setInternal<T>(array, index, value);
  }
  static void set(mxArray* array, mwIndex index, mxArray* value) {
    MEXPLUS_CHECK_NOTNULL(array);
    MEXPLUS_CHECK_NOTNULL(value);
    MEXPLUS_ASSERT(mxIsCell(array), "Expected a cell array.");
    MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                   "Index out of range: %u.",
                   index);
    mxDestroyArray(mxGetCell(array, index));
    mxSetCell(array, index, value);
  }
  template <typename T>
  static void set(mxArray* array,
                  const std::string& field,
                  const T& value,
                  mwIndex index = 0) {
    setInternal<T>(array, field, index, value);
  }
  static void set(mxArray* array,
                  const std::string& field,
                  mxArray* value,
                  mwIndex index = 0) {
    MEXPLUS_CHECK_NOTNULL(array);
    MEXPLUS_CHECK_NOTNULL(value);
    MEXPLUS_ASSERT(mxIsStruct(array), "Expected a struct array.");
    MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                   "Index out of range: %u.",
                   index);
    int field_number = mxGetFieldNumber(array, field.c_str());
    if (field_number < 0) {
      field_number = mxAddField(array, field.c_str());
      MEXPLUS_ASSERT(field_number >= 0,
                     "Failed to create a field '%s'",
                     field.c_str());
    }
    mxDestroyArray(mxGetFieldByNumber(array, index, field_number));
    mxSetFieldByNumber(array, index, field_number, value);
  }

  /** Convert MxArray to a specified type.
   */
  template <typename T>
  T to() const {
    T value;
    toInternal<T>(array_, &value);
    return value;
  }
  template <typename T>
  void to(T* value) const { toInternal<T>(array_, value); }
  /** Template for element accessor.
   * @param index index of the array element.
   * @return value of the element at index.
   *
   *
   * Example:
   * @code
   *     MxArray array(prhs[0]);
   *     double value = array.at<double>(0);
   * @endcode
   */
  template <typename T>
  T at(mwIndex index) const {
    T value;
    atInternal<T>(array_, index, &value);
    return value;
  }
  template <typename T>
  void at(mwIndex index, T* value) const {
    atInternal<T>(array_, index, value);
  }
  const mxArray* at(mwIndex index) const {
    return at(array_, index);
  }
  /** Template for element accessor.
   * @param row index of the first dimension.
   * @param column index of the second dimension.
   * @return value of the element at (row, column).
   */
  template <typename T>
  T at(mwIndex row, mwIndex column) const;
  /** Template for element accessor.
   * @param si subscript index of the element.
   * @return value of the element at subscript index.
   */
  template <typename T>
  T at(const std::vector<mwIndex>& subscripts) const;
  /** Struct element accessor.
   * @param field field name of the struct array.
   * @param index index of the struct array.
   * @return value of the element at the specified field.
   */
  template <typename T>
  T at(const std::string& field, mwIndex index = 0) const {
    T value;
    atInternal<T>(array_, field, index, &value);
    return value;
  }
  template <typename T>
  void at(const std::string& field, T* value, mwIndex index = 0) const {
    atInternal<T>(array_, field, index, value);
  }
  const mxArray* at(const std::string& field, mwIndex index = 0) const {
    return at(array_, field, index);
  }

  /** Template for element write accessor.
   * @param index offset of the array element.
   * @param value value of the field.
   */
  template <typename T>
  void set(mwIndex index, const T& value) {
    setInternal<T>(array_, index, value);
  }
  /** Template for element write accessor.
   * @param row index of the first dimension of the array element.
   * @param column index of the first dimension of the array element.
   * @param value value of the field.
   */
  template <typename T>
  void set(mwIndex row, mwIndex column, const T& value);
  /** Template for element write accessor.
   * @param subscripts subscript index of the element.
   * @param value value of the field.
   */
  template <typename T>
  void set(const std::vector<mwIndex>& subscripts, const T& value);
  /** Cell element write accessor.
   * @param index index of the element.
   * @param value cell element to be inserted.
   */
  void set(mwIndex index, mxArray* value) {
  MEXPLUS_ASSERT(isOwner(), "Must be an owner to set.");
    set(array_, index, value);
  }
  /** Struct element write accessor.
   * @param field field name of the struct array.
   * @param value value of the field.
   * @param index linear index of the struct array element.
   */
  template <typename T>
  void set(const std::string& field, const T& value, mwIndex index = 0) {
    MEXPLUS_ASSERT(isOwner(), "Must be an owner to set.");
    setInternal<T>(array_, field, index, value);
  }
  /** Struct element write accessor.
   * @param field field name of the struct array.
   * @param value value of the field to be inserted.
   * @param index linear index of the struct array element.
   */
  void set(const std::string& field, mxArray* value, mwIndex index = 0) {
    MEXPLUS_ASSERT(isOwner(), "Must be an owner to set.");
    set(array_, field, value, index);
  }
  /** Get raw data pointer.
   * @return pointer T*. If MxArray is not compatible, return NULL.
   */
  template <typename T>
  T* getData() const;
  mxLogical* getLogicals() const {
    MEXPLUS_CHECK_NOTNULL(array_);
    MEXPLUS_ASSERT(isLogical(),
                   "Expected a logical array but %s.",
                   className().c_str());
    return mxGetLogicals(array_);
  }
  mxChar* getChars() const {
    MEXPLUS_CHECK_NOTNULL(array_);
    MEXPLUS_ASSERT(isChar(),
                   "Expected a char array but %s.",
                   className().c_str());
    return mxGetChars(array_);
  }
  /** Class ID of mxArray.
   */
  inline mxClassID classID() const { return mxGetClassID(array_); }
  /** Class name of mxArray.
   */
  inline const std::string className() const {
    return std::string(mxGetClassName(array_));
  }
  /** Number of elements in an array.
   */
  inline mwSize size() const { return mxGetNumberOfElements(array_); }
  /** Number of dimensions.
   */
  inline mwSize dimensionSize() const {
    return mxGetNumberOfDimensions(array_);
  }
  /** Array of each dimension.
   */
  inline std::vector<mwSize> dimensions() const {
    const mwSize* dimensions = mxGetDimensions(array_);
    return std::vector<mwSize>(dimensions, dimensions + dimensionSize());
  }
  /** Number of rows in an array.
   */
  inline mwSize rows() const { return mxGetM(array_); }
  /** Number of columns in an array.
   */
  inline mwSize cols() const { return mxGetN(array_); }
  /** Number of fields in a struct array.
   */
  inline int fieldSize() const { return mxGetNumberOfFields(array_); }
  /** Get field name of a struct array.
   * @param index index of the struct array.
   * @return std::string.
   */
  std::string fieldName(int index) const {
    const char* field = mxGetFieldNameByNumber(array_, index);
    MEXPLUS_ASSERT(field, "Failed to get field name at %d.", index);
    return std::string(field);
  }
  /** Get field names of a struct array.
   * @params field_nams std::vector<std::string> of struct field names.
   */
  std::vector<std::string> fieldNames() const {
    MEXPLUS_ASSERT(isStruct(), "Expected a struct array.");
    std::vector<std::string> fields(fieldSize());
    for (int i = 0; i < fields.size(); ++i)
      fields[i] = fieldName(i);
    return fields;
  }
  /** Number of elements in IR, PR, and PI arrays.
   */
  inline mwSize nonZeroMax() const { return mxGetNzmax(array_); }
  /** Offset from first element to desired element.
   * @param row index of the first dimension of the array.
   * @param column index of the second dimension of the array.
   * @return linear offset of the specified subscript index.
   */
  mwIndex subscriptIndex(mwIndex row, mwIndex column) const {
    MEXPLUS_ASSERT(row < rows() && column < cols(),
                   "Subscript is out of range.");
    mwIndex subscripts[] = {row, column};
    return mxCalcSingleSubscript(array_, 2, subscripts);
  }
  /** Offset from first element to desired element.
   * @param si subscript index of the array.
   * @return linear offset of the specified subscript index.
   */
  mwIndex subscriptIndex(const std::vector<mwIndex>& subscripts) const {
    return mxCalcSingleSubscript(array_, subscripts.size(), &subscripts[0]);
  }
  /** Determine whether input is cell array.
   */
  inline bool isCell() const { return mxIsCell(array_); }
  /** Determine whether input is string array.
   */
  inline bool isChar() const { return mxIsChar(array_); }
  /** Determine whether input is vector array.
   */
  inline bool isVector() const { 
    mwSize rows  = (mwSize)mxGetM(array_);
    mwSize cols  = (mwSize)mxGetN(array_);
    mwSize nDims = (mwSize)mxGetNumberOfDimensions(array_);
    
    return nDims==2 && (rows==1 || cols==1);
  }
  /** Determine whether array is member of specified class.
   */
  inline bool isClass(const char* name) const {
    return mxIsClass(array_, name);
  }
  /** Determine whether data is complex.
   */
  inline bool isComplex() const { return mxIsComplex(array_); }
  /** Determine whether mxArray represents data as double-precision,
   * floating-point numbers.
   */
  inline bool isDouble() const { return mxIsDouble(array_); }
  /** Determine whether array is empty.
   */
  inline bool isEmpty() const { return mxIsEmpty(array_); }
  /** Determine whether input is finite.
   */
  static inline bool IsFinite(double value) { return mxIsFinite(value); }
  /** Determine whether array was copied from MATLAB global workspace.
   */
  inline bool isFromGlobalWS() const { return mxIsFromGlobalWS(array_); };
  /** Determine whether input is infinite.
   */
  static inline bool IsInf(double value) { return mxIsInf(value); }
  /** Determine whether array represents data as signed 8-bit integers.
   */
  inline bool isInt8() const { return mxIsInt8(array_); }
  /** Determine whether array represents data as signed 16-bit integers.
   */
  inline bool isInt16() const { return mxIsInt16(array_); }
  /** Determine whether array represents data as signed 32-bit integers.
   */
  inline bool isInt32() const { return mxIsInt32(array_); }
  /** Determine whether array represents data as signed 64-bit integers.
   */
  inline bool isInt64() const { return mxIsInt64(array_); }
  /** Determine whether array is of type mxLogical.
   */
  inline bool isLogical() const { return mxIsLogical(array_); }
  /** Determine whether scalar array is of type mxLogical.
   */
  inline bool isLogicalScalar() const { return mxIsLogicalScalar(array_); }
  /** Determine whether scalar array of type mxLogical is true.
   */
  inline bool isLogicalScalarTrue() const {
    return mxIsLogicalScalarTrue(array_);
  }
  /** Determine whether array is numeric.
   */
  inline bool isNumeric() const { return mxIsNumeric(array_); }
  /** Determine whether array represents data as single-precision,
   * floating-point numbers.
   */
  inline bool isSingle() const { return mxIsSingle(array_); }
  /** Determine whether input is sparse array.
   */
  inline bool isSparse() const { return mxIsSparse(array_); }
  /** Determine whether input is structure array.
   */
  inline bool isStruct() const { return mxIsStruct(array_); }
  /** Determine whether array represents data as unsigned 8-bit integers.
   */
  inline bool isUint8() const { return mxIsUint8(array_); }
  /** Determine whether array represents data as unsigned 16-bit integers.
   */
  inline bool isUint16() const { return mxIsUint16(array_); }
  /** Determine whether array represents data as unsigned 32-bit integers.
   */
  inline bool isUint32() const { return mxIsUint32(array_); }
  /** Determine whether array represents data as unsigned 64-bit integers.
   */
  inline bool isUint64() const { return mxIsUint64(array_); }
  /** Determine whether a struct array has a specified field.
   */
  bool hasField(const std::string& field_name, mwIndex index = 0) const {
    return isStruct() &&
        mxGetField(array_, index, field_name.c_str()) != NULL;
  }
  /** Determine whether input is NaN (Not-a-Number).
   */
  static inline bool IsNaN(double value) { return mxIsNaN(value); }
  /** Value of infinity.
   */
  static inline double Inf() { return mxGetInf(); }
  /** Value of NaN (Not-a-Number).
   */
  static inline double NaN() { return mxGetNaN(); }
  /** Value of EPS.
   */
  static inline double Eps() { return mxGetEps(); }

private:
  /** Copy constructor is prohibited except internally.
   */
  MxArray(const MxArray& array);
  //MxArray(const MxArray& array) = delete;
  /** Copy assignment operator is prohibited.
   */
  MxArray& operator=(const MxArray& rhs);
  //MxArray& operator=(const MxArray& rhs) = delete;



  /*************************************************************/

  /**             Templated mxArray importers                 **/

  /*************************************************************/

  /* fundamental numerics */
  template <typename T>
  static mxArray* fromInternal(const typename std::enable_if<
      MxArithmeticType<T>::value, T>::type& value);
  /* complex types, complex<float> or complex<double> */
  template <typename T>
  static mxArray* fromInternal(const typename std::enable_if<
      MxComplexType<T>::value, T>::type& value );
  /* container with fundamental numerics, i.e. vector<double> */
  template <typename Container>
  static mxArray* fromInternal(const typename std::enable_if<
      MxArithmeticCompound<Container>::value, 
      Container>::type& value);
  /* container with complex numbers, i.e. vector<complex<double>> */
  template <typename Container>
  static mxArray* fromInternal(const typename std::enable_if<
      MxComplexCompound<Container>::value, Container>::type& value);
  /* char type */
  template <typename T>
  static mxArray* fromInternal(const typename std::enable_if<
      MxCharType<T>::value, T>::type& value);
  /* containter with signed char */
  template <typename Container>
  static mxArray* fromInternal(const typename std::enable_if<
      (MxCharCompound<Container>::value) &&
      (std::is_signed<typename Container::value_type>::value),
      Container>::type& value);
  /* container with unsigned char */
  template <typename Container>
  static mxArray* fromInternal(const typename std::enable_if<
      (MxCharCompound<Container>::value) &&
      !(std::is_signed<typename Container::value_type>::value),
      Container>::type& value);
  /* logicals */
  template <typename T>
  static mxArray* fromInternal(const typename std::enable_if<
      MxLogicalType<T>::value, T>::type& value);
  /* container with logicals */
  template <typename Container>
  static mxArray* fromInternal(const typename std::enable_if<
      MxLogicalCompound<Container>::value, Container>::type& value);
  /* container with cell type content */
  template <typename Container>
  static mxArray* fromInternal(const typename std::enable_if<
      MxCellCompound<Container>::value, Container>::type& value);



  /*************************************************************/
  
  /**             Templated mxArray exporters                 **/

  /*************************************************************/

  /* singleton types */
  template <typename T>
  static void toInternal(const mxArray* array, 
                         typename std::enable_if<
                           MxArithmeticType<T>::value || 
                           MxComplexType<T>::value ||
                           MxLogicalType<T>::value ||
                           MxCharType<T>::value, T>::type* value) {
    atInternal<T>(array, 0, value);
  }
  /* vector types */
  template <typename T>
  static void toInternal(const mxArray* array, 
                         typename std::enable_if<
                           MxComplexOrArithmeticCompound<T>::value || 
                           MxLogicalCompound<T>::value ||
                           MxCharCompound<T>::value, T>::type* value);
  /* nested types (leads into recursive deduction) */
  template <typename T>
  static void toInternal(const mxArray* array, typename std::enable_if<
                           MxCellType<T>::value &&
                           (!std::is_compound<T>::value || 
                            MxCellType<typename T::value_type>::value), T>::type* value);



  /*************************************************************/
  
  /**             Templated mxArray getters                   **/

  /*************************************************************/

  /* fundamental numeric types */
  template <typename T>
  static void atInternal(const mxArray* array, mwIndex index,
                         typename std::enable_if<
                         MxComplexOrArithmeticType<T>::value || 
                         MxLogicalType<T>::value ||
                         MxCharType<T>::value, T>::type* value); 
  /* converter for nested types */
  template <typename T>
  static void atInternal(const mxArray* array, mwIndex index,
                         typename std::enable_if<
                           std::is_compound<T>::value && !MxComplexType<T>::value, T>::type* value);
  /* structure acess */
  template <typename T>
  static void atInternal(const mxArray* array, const std::string& field,
                         mwIndex index, T* value);



  /*************************************************************/
  
  /**           Templated mxArray element setters             **/

  /*************************************************************/

  /* fundamental numeric and complex types */
  template <typename T>
  static void setInternal(mxArray* array, mwIndex index,
                          const typename std::enable_if<
						                !std::is_compound<T>::value || 
										MxComplexType<T>::value, T>::type& value);
  /* container types */
  template <typename T>
  static void setInternal(mxArray* array,  mwIndex index,
                          const typename std::enable_if<
					                  MxCellType<T>::value, T>::type& value);
  /* structure access */
  template <typename T>
  static void setInternal(mxArray* array, const std::string& field,
                          mwIndex index, const T& value);



  /*************************************************************/
  
  /**    Assignment helpers (for MxArray.to<type>(value) )    **/

  /*************************************************************/
						  
  /* Explicit integer element assignment */
  template <typename T, typename R>
  static void assignTo(const mxArray* array, 
                       mwIndex index, typename std::enable_if<
                       std::is_integral<R>::value, R>::type* value) {
    MEXPLUS_ASSERT(!mxIsComplex(array), "Non-complex array expected!");
    *value = (R)*(reinterpret_cast<T*>(mxGetData(array)) + index);
  }
  /* Explicit floating point element assignment */
  template <typename T, typename R>
  static void assignTo(const mxArray* array, 
                       mwIndex index, 
                       typename std::enable_if<
                         std::is_floating_point<R>::value, R>::type* value) {
    if(mxIsComplex(array))
    {
        T real_part = *(reinterpret_cast<T*>(mxGetPr(array)) + index);
        T imag_part = *(reinterpret_cast<T*>(mxGetPi(array)) + index);

        *value = std::abs( std::complex<R>( real_part, imag_part ) );  /* magnitude */
    } else {
        *value = (R)*(reinterpret_cast<T*>(mxGetData(array)) + index);
    }
  }
  /* Explicit complex element assignment */
  template <typename T, typename R>
  static void assignTo(const mxArray* array, mwIndex index, 
                       typename std::enable_if<
                         MxComplexType<R>::value, R>::type* value) {
    typedef typename R::value_type ValueType;
    ValueType real_part, imag_part;

    if(mxIsComplex(array))
    {
        real_part = (typename R::value_type)*(reinterpret_cast<T*>(mxGetPr(array)) + index);
        imag_part = (typename R::value_type)*(reinterpret_cast<T*>(mxGetPi(array)) + index);
    } else {
        real_part = (typename R::value_type)*(reinterpret_cast<T*>(mxGetData(array)) + index);
        imag_part = (typename R::value_type)0.0;
    }
    
    *value = std::complex<ValueType>(real_part, imag_part);
  }
  /* explicit char (signed) element assignment */
  template <typename R>
  static void assignCharTo(const mxArray* array, mwIndex index,
                           typename std::enable_if<
                             std::is_signed<R>::value, R>::type* value) {
    typedef typename std::make_signed<mxChar>::type SignedMxChar;
    *value = *(reinterpret_cast<SignedMxChar*>(mxGetChars(array)) + index);
  }
  /* explicit char (unsigned) element assignment */
  template <typename R>
  static void assignCharTo(const mxArray* array,
                           mwIndex index,
                           typename std::enable_if<
                             !std::is_signed<R>::value, R>::type* value) {
    *value = *(mxGetChars(array) + index);
  }
    /* explicit cell element assignment */
  template <typename T>
  static void assignCellTo(const mxArray* array, mwIndex index, T* value) {
    const mxArray* element = mxGetCell(array, index);
    MEXPLUS_CHECK_NOTNULL(element);
    toInternal<T>(element, value);  /* recursion for nested types */
  }



  /* explicit numeric array assignment */
  template <typename T, typename R>
  static void assignTo(const mxArray* array, typename std::enable_if<
                                               MxArithmeticCompound<R>::value || 
											   MxLogicalCompound<R>::value || 
											   MxCharCompound<R>::value, R>::type* value) {
    if(!mxIsComplex(array))
    {
      T* data_pointer = reinterpret_cast<T*>(mxGetData(array));
      value->assign(data_pointer, data_pointer + mxGetNumberOfElements(array));
    } else {
      T* real_part = reinterpret_cast<T*>(mxGetPr(array));
      T* imag_part = reinterpret_cast<T*>(mxGetPi(array));

      assert( value->empty() );
      for( mwSize i = 0; i < mxGetNumberOfElements(array); i++, real_part++, imag_part++ )
      {
        value->push_back( std::abs( std::complex<double>( *real_part, *imag_part ) ) ); /* magnitude */
      }
    }
  }
  /* explicit complex array assigment */
    template <typename T, typename R>
  static void assignTo(const mxArray* array, typename std::enable_if<
      MxComplexCompound<R>::value, R>::type* value) {
    if(!mxIsComplex(array))
    {
      T* data_pointer = reinterpret_cast<T*>(mxGetData(array));

      assert( value->empty() );
      for( mwSize i = 0; i < mxGetNumberOfElements(array); i++, data_pointer++ )
      {
        value->push_back( (R)( (typename R::value_type)*data_pointer, 0.0f ) );
      }
    } else {
      T* real_part = reinterpret_cast<T*>(mxGetPr(array));
      T* imag_part = reinterpret_cast<T*>(mxGetPi(array));

      for( mwSize i = 0; i < mxGetNumberOfElements(array); i++, real_part++, imag_part++ )
      {
		  value->push_back( R::value_type( *real_part, *imag_part ) );
      }
    }
  }
    /* explicit char (signed) array assignment */
  template <typename R>
  static void assignStringTo(const mxArray* array, typename std::enable_if<
      std::is_signed<typename R::value_type>::value, R>::type* value) {
    typedef typename std::make_signed<mxChar>::type SignedMxChar;
    SignedMxChar* data_pointer = reinterpret_cast<SignedMxChar*>(
        mxGetChars(array));
    value->assign(data_pointer, data_pointer + mxGetNumberOfElements(array));
  }
  /* explicit char (unsigned) array assignment */
  template <typename R>
  static void assignStringTo(const mxArray* array, typename std::enable_if<
      !std::is_signed<typename R::value_type>::value,
      R>::type* value) {
    mxChar* data_pointer = mxGetChars(array);
    value->assign(data_pointer, data_pointer + mxGetNumberOfElements(array));
  }
  /* explicit cell array assignment */
  template <typename T>
  static void assignCellTo(const mxArray* array, T* value) {
    assert( value->empty() );
    for (int i = 0; i < mxGetNumberOfElements(array); ++i) {
      const mxArray* element = mxGetCell(array, i);
      MEXPLUS_CHECK_NOTNULL(element);
      value->push_back(to<typename T::value_type>(element));
    }
  }



  /*************************************************************/
  
  /**  Assignment helpers (for MxArray.set<type>(i, value) )  **/

  /*************************************************************/

  /* explicit numeric element assignment */
  template <typename R, typename T>
  static void assignFrom(mxArray* array, 
                         mwIndex index, 
                         const typename std::enable_if<
                         MxArithmeticType<T>::value || 
						 MxCharType<T>::value, T>::type& value) {
    if(mxIsComplex(array))
    {
      *(reinterpret_cast<R*>(mxGetPr(array)) + index) = (R)value;
      *(reinterpret_cast<R*>(mxGetPi(array)) + index) = (R)0.0;
    } else {
      *(reinterpret_cast<R*>(mxGetData(array)) + index) = (R)value;
    }
  }
  /* explicit complex element assignment */
  template <typename R, typename T>
  static void assignFrom(mxArray* array, 
                         mwIndex index, 
                         const typename std::enable_if<
                         MxComplexType<T>::value, T>::type& value) {
    if(mxIsComplex(array))
    {
      *(reinterpret_cast<R*>(mxGetPr(array)) + index) = value.real();
      *(reinterpret_cast<R*>(mxGetPi(array)) + index) = value.imag();
    } else {
      *(reinterpret_cast<R*>(mxGetData(array)) + index) = (R)std::abs( value );  /* magnitude */
    }
  }
  template <typename T>
  static void assignCharFrom(mxArray* array,
                             mwIndex index,
                             const typename std::enable_if<
                               std::is_floating_point<T>::value, T>::type& value) {
    *(mxGetChars(array) + index) = (char)value;  // whoever needs this...
  }
  template <typename T>
  static void assignCharFrom(mxArray* array,
                             mwIndex index,
                             const typename std::enable_if<
                               std::is_integral<T>::value &&
                               std::is_signed<T>::value, T>::type& value) {
    *(mxGetChars(array) + index) = reinterpret_cast<const typename
        std::make_unsigned<T>::type&>(value);
  }
  template <typename T>
  static void assignCharFrom(mxArray* array,
                             mwIndex index,
                             const typename std::enable_if<
                               std::is_integral<T>::value &&
                               !std::is_signed<T>::value, T>::type& value) {
    *(mxGetChars(array) + index) = value;
  }

  template <typename T>
  static void assignCharFrom(mxArray* array,
                             mwIndex index,
                             const typename std::enable_if<
                               MxComplexType<T>::value, T>::type& value) {
    *(mxGetChars(array) + index) = std::abs(value);  // whoever needs it...
  }

  /** Pointer to the mxArray C object.
   */
  mxArray* array_;
  /** Flag to enable resource management.
   */
  bool owner_;
};





/*************************************************************/

/**             Templated mxArray importers                 **/

/*************************************************************/

/* Fundamental numeric type */
template <typename T>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    MxArithmeticType<T>::value, T>::type& value) {
  mxArray* array = mxCreateNumericMatrix(1,
                                         1,
                                         MxTypes<T>::class_id,
                                         MxTypes<T>::complexity);
  MEXPLUS_CHECK_NOTNULL(array);
  *reinterpret_cast<T*>(mxGetData(array)) = value;
  return array;
}
/* Complex type, complex<float> or complex<double> */
template <typename T>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    MxComplexType<T>::value, T>::type& value ) {
  typedef T ValueType;
  typedef typename T::value_type CplValueType;
  mxArray* array = mxCreateNumericMatrix(1, 1, 
                                         MxTypes<ValueType>::class_id,
                                         MxTypes<ValueType>::complexity);
  MEXPLUS_CHECK_NOTNULL(array);
  *reinterpret_cast<CplValueType*>(mxGetPr(array)) = value.real();
  *reinterpret_cast<CplValueType*>(mxGetPi(array)) = value.imag();

  return array;
}
/* container with fundamental numerics, i.e. vector<double> */
template <typename Container>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    MxArithmeticCompound<Container>::value, Container>::type& value) {
  typedef typename Container::value_type ValueType;
  mxArray* array = mxCreateNumericMatrix(1,
                                         value.size(),
                                         MxTypes<ValueType>::class_id,
                                         MxTypes<ValueType>::complexity);
  MEXPLUS_CHECK_NOTNULL(array);
  std::copy(value.begin(),
            value.end(),
            reinterpret_cast<ValueType*>(mxGetData(array)));
  return array;
}
/* container with complex numbers, i.e. vector<complex<double>> */
template <typename Container>
mxArray* MxArray::fromInternal(const typename std::enable_if<
      MxComplexCompound<Container>::value, Container>::type& value) {
  typedef typename Container::value_type CtValueType;
  typedef typename CtValueType::value_type ValueType;
  mxArray* array = mxCreateNumericMatrix(1,
                                         value.size(),
                                         MxTypes<CtValueType>::class_id,
                                         mxCOMPLEX);
  MEXPLUS_CHECK_NOTNULL(array);
  ValueType* real = reinterpret_cast<ValueType*>(mxGetPr(array));
  ValueType* imag = reinterpret_cast<ValueType*>(mxGetPi(array));

  typename Container::const_iterator it;

  for( it = value.begin(); it != value.end(); it++ )
  {
      *real++ = (*it).real();
      *imag++ = (*it).imag();
  }

  return array;
}
template <typename T>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    MxCharType<T>::value, T>::type& value) {
  const char char_array[] = {static_cast<char>(value), 0};
  mxArray* array = mxCreateString(char_array);
  MEXPLUS_CHECK_NOTNULL(array);
  return array;
}
template <typename Container>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    (MxCharCompound<Container>::value) &&
    (std::is_signed<typename Container::value_type>::value),
    Container>::type& value) {
  typedef typename std::make_unsigned<typename Container::value_type>::type
                   UnsignedValue;
  const mwSize dimensions[] = {1, static_cast<mwSize>(value.size())};
  mxArray* array = mxCreateCharArray(2, dimensions);
  MEXPLUS_CHECK_NOTNULL(array);
  mxChar* array_data = mxGetChars(array);
  for (typename Container::const_iterator it = value.begin();
       it != value.end();
       ++it) {
    *(array_data++) = reinterpret_cast<const UnsignedValue&>(*it);
  }
  return array;
}
template <typename Container>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    (MxCharCompound<Container>::value) &&
    !(std::is_signed<typename Container::value_type>::value),
    Container>::type& value) {
  const mwSize dimensions[] = {1, static_cast<mwSize>(value.size())};
  mxArray* array = mxCreateCharArray(2, dimensions);
  MEXPLUS_CHECK_NOTNULL(array);
  std::copy(value.begin(), value.end(), mxGetChars(array));
  return array;
}
template <typename T>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    MxLogicalType<T>::value, T>::type& value) {
  mxArray* array = mxCreateLogicalScalar(value);
  MEXPLUS_CHECK_NOTNULL(array);
  return array;
}
template <typename Container>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    MxLogicalCompound<Container>::value, Container>::type& value) {
  mxArray* array = mxCreateLogicalMatrix(1, value.size());
  MEXPLUS_CHECK_NOTNULL(array);
  std::copy(value.begin(), value.end(), mxGetLogicals(array));
  return array;
}
template <typename Container>
mxArray* MxArray::fromInternal(const typename std::enable_if<
    MxCellCompound<Container>::value, Container>::type& value) {
  mxArray* array = mxCreateCellMatrix(1, value.size());
  MEXPLUS_CHECK_NOTNULL(array);
  mwIndex index = 0;
  for (typename Container::const_iterator it = value.begin();
       it != value.end();
       ++it) {
    mxArray* new_item = from(*it);  /* safe in case if from() fails */
    mxDestroyArray(mxGetCell(array, index));
    mxSetCell(array, index++, new_item);
  }
  return array;
}




/*************************************************************/

/**             Templated mxArray exporters                 **/

/*************************************************************/

/* converter from numeric matrix to container */
template <typename T>
void MxArray::toInternal(const mxArray* array, typename std::enable_if<
                           MxComplexOrArithmeticCompound<T>::value || 
                           MxLogicalCompound<T>::value ||
                           MxCharCompound<T>::value, T>::type* value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_CHECK_NOTNULL(value);
  switch (mxGetClassID(array)) {
    case mxINT8_CLASS:    assignTo<int8_t, T>(array, value); break;
    case mxUINT8_CLASS:   assignTo<uint8_t, T>(array, value); break;
    case mxINT16_CLASS:   assignTo<int16_t, T>(array, value); break;
    case mxUINT16_CLASS:  assignTo<uint16_t, T>(array, value); break;
    case mxINT32_CLASS:   assignTo<int32_t, T>(array, value); break;
    case mxUINT32_CLASS:  assignTo<uint32_t, T>(array, value); break;
    case mxINT64_CLASS:   assignTo<int64_t, T>(array, value); break;
    case mxUINT64_CLASS:  assignTo<uint64_t, T>(array, value); break;
    case mxSINGLE_CLASS:  assignTo<float, T>(array, value); break;
    case mxDOUBLE_CLASS:  assignTo<double, T>(array, value); break;
    case mxLOGICAL_CLASS: assignTo<mxLogical, T>(array, value); break;
    case mxCHAR_CLASS:    assignStringTo<T>(array, value); break;
    case mxCELL_CLASS:    assignCellTo<T>(array, value); break;
    // case mxSPARSE_CLASS:
    default:
      MEXPLUS_ERROR("Cannot convert %s.", mxGetClassName(array));
  }
}
/* converter from nested types to container */
template <typename T>
void MxArray::toInternal(const mxArray* array, typename std::enable_if<
                           MxCellType<T>::value &&
                           (!std::is_compound<T>::value || 
                            MxCellType<typename T::value_type>::value), T>::type* value) {
  MEXPLUS_CHECK_NOTNULL(value);
  MEXPLUS_ASSERT(mxIsCell(array), "Expected a cell array.");
  assert( value->empty() );
  for (int i = 0; i < mxGetNumberOfElements(array); ++i) {
    const mxArray* element = mxGetCell(array, i);
    value->push_back(to<typename T::value_type>(element));
  }
}




/*************************************************************/

/**             Templated mxArray getters                   **/

/*************************************************************/
/* converter from fundamental numeric or complex types */
template <typename T>
void MxArray::atInternal(const mxArray* array, mwIndex index,
                         typename std::enable_if<
                         MxComplexOrArithmeticType<T>::value || 
                         MxLogicalType<T>::value ||
                         MxCharType<T>::value, T>::type* value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_CHECK_NOTNULL(value);
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  switch (mxGetClassID(array)) {
    case mxINT8_CLASS:    assignTo<int8_t, T>(array, index, value); break;
    case mxUINT8_CLASS:   assignTo<uint8_t, T>(array, index, value); break;
    case mxINT16_CLASS:   assignTo<int16_t, T>(array, index, value); break;
    case mxUINT16_CLASS:  assignTo<uint16_t, T>(array, index, value); break;
    case mxINT32_CLASS:   assignTo<int32_t, T>(array, index, value); break;
    case mxUINT32_CLASS:  assignTo<uint32_t, T>(array, index, value); break;
    case mxINT64_CLASS:   assignTo<int64_t, T>(array, index, value); break;
    case mxUINT64_CLASS:  assignTo<uint64_t, T>(array, index, value); break;
    case mxSINGLE_CLASS:  assignTo<float, T>(array, index, value); break;
    case mxDOUBLE_CLASS:  assignTo<double, T>(array, index, value); break;
    case mxLOGICAL_CLASS: assignTo<mxLogical, T>(array, index, value); break;
    case mxCHAR_CLASS:    assignCharTo<T>(array, index, value); break;
    case mxCELL_CLASS:    assignCellTo<T>(array, index, value); break;
    // case mxSPARSE_CLASS:
    default:
      MEXPLUS_ASSERT(true, "Cannot convert %s", mxGetClassName(array));
  }
}
template <typename T>
void MxArray::atInternal(const mxArray* array, mwIndex index,
                         typename std::enable_if<
                           std::is_compound<T>::value && 
                           !MxComplexType<T>::value, T>::type* value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_CHECK_NOTNULL(value);
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  MEXPLUS_ASSERT(mxIsCell(array), "Expected a cell array.");
  const mxArray* element = mxGetCell(array, index);
  toInternal<T>(element, value);
}
template <typename T>
void MxArray::atInternal(const mxArray* array,
                         const std::string& field,
                         mwIndex index,
                         T* value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_CHECK_NOTNULL(value);
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  MEXPLUS_ASSERT(mxIsStruct(array), "Expected a struct array.");
  const mxArray* element = mxGetField(array, index, field.c_str());
  MEXPLUS_ASSERT(element, "Invalid field name %s.", field.c_str());
  toInternal<T>(element, value);
}




/*************************************************************/

/**             Templated mxArray setters                   **/

/*************************************************************/
/* converter from fundamental numeric or complex types */
template <typename T>
void MxArray::setInternal(mxArray* array,
                          mwIndex index,
                          const typename std::enable_if<
                            !std::is_compound<T>::value || 
							MxComplexType<T>::value, T>::type& value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  switch (mxGetClassID(array)) {
    case mxINT8_CLASS:    assignFrom<int8_t, T>(array, index, value); break;
    case mxUINT8_CLASS:   assignFrom<uint8_t, T>(array, index, value); break;
    case mxINT16_CLASS:   assignFrom<int16_t, T>(array, index, value); break;
    case mxUINT16_CLASS:  assignFrom<uint16_t, T>(array, index, value); break;
    case mxINT32_CLASS:   assignFrom<int32_t, T>(array, index, value); break;
    case mxUINT32_CLASS:  assignFrom<uint32_t, T>(array, index, value); break;
    case mxINT64_CLASS:   assignFrom<int64_t, T>(array, index, value); break;
    case mxUINT64_CLASS:  assignFrom<uint64_t, T>(array, index, value); break;
    case mxSINGLE_CLASS:  assignFrom<float, T>(array, index, value); break;
    case mxDOUBLE_CLASS:  assignFrom<double, T>(array, index, value); break;
    case mxCHAR_CLASS:    assignCharFrom<T>(array, index, value); break;
    case mxLOGICAL_CLASS: assignFrom<mxLogical, T>(array, index, value); break;
    case mxCELL_CLASS: 
    {
      mxArray* new_item = from(value);  /* safe in case if from() fails */

      mxDestroyArray(mxGetCell(array, index));
      mxSetCell(array, index, new_item); 
      break;
    }
    default:
      MEXPLUS_ERROR("Cannot assign to %s array.", mxGetClassName(array));
  }
}
/* converter from fundamental numeric or complex types */
template <typename T>
void MxArray::setInternal(mxArray* array,
                          mwIndex index,
                          const typename std::enable_if<
                            MxCellType<T>::value, T>::type& value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  MEXPLUS_ASSERT(mxIsCell(array), "Expected a cell array.");
  mxArray* new_item = from(value);  /* safe in case if from() fails */
  mxDestroyArray(mxGetCell(array, index));
  mxSetCell(array, index, new_item);
}
template <typename T>
void MxArray::setInternal(mxArray* array,
                          const std::string& field,
                          mwIndex index,
                          const T& value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  MEXPLUS_ASSERT(mxIsStruct(array), "Expected a struct array.");
  int field_number = mxGetFieldNumber(array, field.c_str());
  if( field_number < 0 ) {
    field_number = mxAddField(array, field.c_str());
    MEXPLUS_ASSERT(field_number >= 0,
                   "Failed to create a field '%s'",
                   field.c_str());
  }
  mxArray* new_item = from(value);
  mxDestroyArray(mxGetFieldByNumber(array, index, field_number));
  mxSetFieldByNumber(array, index, field_number, new_item);
}





template <typename T>
mxArray* MxArray::Numeric(int rows, int columns) {
  typedef typename std::enable_if<
      MxComplexOrArithmeticType<T>::value, T>::type Scalar;
  mxArray* numeric = mxCreateNumericMatrix(rows,
                                           columns,
                                           MxTypes<Scalar>::class_id,
                                           MxTypes<Scalar>::complexity);
  MEXPLUS_CHECK_NOTNULL(numeric);
  return numeric;
}

template <typename T>
T* MxArray::getData() const {
  MEXPLUS_CHECK_NOTNULL(array_);
  MEXPLUS_ASSERT(MxTypes<T>::class_id == classID(),
                 "Expected a %s array.",
                 typeid(T).name());
  return reinterpret_cast<T*>(mxGetData(array_));
}

template <typename T>
T MxArray::at(mwIndex row, mwIndex column) const {
  return at<T>(subscriptIndex(row, column));
}

template <typename T>
T MxArray::at(const std::vector<mwIndex>& subscripts) const {
  return at<T>(subscriptIndex(subscripts));
}

template <typename T>
void MxArray::set(mwIndex row, mwIndex column, const T& value) {
  set<T>(subscriptIndex(row, column), value);
}

template <typename T>
void MxArray::set(const std::vector<mwIndex>& subscripts, const T& value) {
  set<T>(subscriptIndex(subscripts), value);
}

} // namespace mexplus

#endif // __MEXPLUS_MXARRAY_H__
