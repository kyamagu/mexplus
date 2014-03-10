/** MxArray data conversion library.
 *
 * Kota Yamaguchi 2013 <kyamagu@cs.stonybrook.edu>
 */

#include <mexplus/mxarray.h>

namespace mexplus {

MxArray::MxArray() : array_(NULL), owner_(false) {}

MxArray& MxArray::operator= (std::nullptr_t) {
  reset();
  return *this;
}

MxArray::MxArray(MxArray&& array) : array_(NULL), owner_(false) {
  *this = std::move(array);
}

MxArray& MxArray::operator= (MxArray&& rhs) {
  if (this != &rhs) {
    array_ = rhs.array_;
    owner_ = rhs.owner_;
    rhs.array_ = NULL;
    rhs.owner_ = false;
  }
  return *this;
}

MxArray::MxArray(mxArray* array) : array_(array), owner_(array) {}

MxArray::MxArray(const mxArray* array) :
    array_(const_cast<mxArray*>(array)),
    owner_(false) {}

mxArray* MxArray::from(const char* value) {
  mxArray* array = mxCreateString(value);
  MEXPLUS_CHECK_NOTNULL(array);
  return array;
}

mxArray* MxArray::from(int32_t value) {
  mxArray* array = mxCreateNumericMatrix(1, 1, mxINT32_CLASS, mxREAL);
  MEXPLUS_CHECK_NOTNULL(array);
  *reinterpret_cast<int32_t*>(mxGetData(array)) = value;
  return array;
}

MxArray::~MxArray() {
  if (array_ && owner_)
    mxDestroyArray(array_);
}

void MxArray::swap(MxArray& rhs) {
  if (this != &rhs) {
    mxArray* array = rhs.array_;
    bool owner = rhs.owner_;
    rhs.array_ = array_;
    rhs.owner_ = owner_;
    array_ = array;
    owner_ = owner;
  }
}

void MxArray::reset(const mxArray* array) {
  if (array_ && owner_)
    mxDestroyArray(array_);
  array_ = const_cast<mxArray*>(array);
  owner_ = false;
}

void MxArray::reset(mxArray* array) {
  if (array_ && owner_)
    mxDestroyArray(array_);
  array_ = array;
  owner_ = array;
}

mxArray* MxArray::release() {
  MEXPLUS_CHECK_NOTNULL(array_);
  mxArray* array = (owner_) ? array_ : clone();
  array_ = NULL;
  owner_ = false;
  return array;
}

mxArray* MxArray::clone() const {
  MEXPLUS_CHECK_NOTNULL(array_);
  mxArray* array = mxDuplicateArray(array_);
  MEXPLUS_CHECK_NOTNULL(array);
  return array;
}

mxArray* MxArray::Logical(int rows, int columns) {
  mxArray* logical_array = mxCreateLogicalMatrix(rows, columns);
  MEXPLUS_CHECK_NOTNULL(logical_array);
  return logical_array;
}

mxArray* MxArray::Cell(int rows, int columns) {
  mxArray* cell_array = mxCreateCellMatrix(rows, columns);
  MEXPLUS_CHECK_NOTNULL(cell_array);
  return cell_array;
}

mxArray* MxArray::Struct(int nfields,
                         const char** fields,
                         int rows,
                         int columns) {
  mxArray* struct_array = mxCreateStructMatrix(rows, columns, nfields, fields);
  MEXPLUS_CHECK_NOTNULL(struct_array);
  return struct_array;
}

template <>
mxLogical* MxArray::getData() const {
  MEXPLUS_CHECK_NOTNULL(array_);
  MEXPLUS_ASSERT(isLogical(),
                 "Expected a logical array but %s.",
                 className().c_str());
  return mxGetLogicals(array_);
}

template <>
mxChar* MxArray::getData() const {
  MEXPLUS_CHECK_NOTNULL(array_);
  MEXPLUS_ASSERT(isChar(), 
                 "Expected a char array but %s.",
                 className().c_str());
  return mxGetChars(array_);
}

std::string MxArray::fieldName(int index) const {
  const char* field = mxGetFieldNameByNumber(array_, index);
  MEXPLUS_ASSERT(field, "Failed to get field name at %d.", index);
  return std::string(field);
}

std::vector<std::string> MxArray::fieldNames() const {
  MEXPLUS_ASSERT(isStruct(), "Expected a struct array.");
  std::vector<std::string> fields(fieldSize());
  for (int i = 0; i < fields.size(); ++i)
    fields[i] = fieldName(i);
  return fields;
}

mwIndex MxArray::subscriptIndex(mwIndex row, mwIndex column) const {
  MEXPLUS_ASSERT(row >= rows() || column >= cols(),
                 "Subscript is out of range.");
  mwIndex subscripts[] = {row, column};
  return mxCalcSingleSubscript(array_, 2, subscripts);
}

mwIndex MxArray::subscriptIndex(const std::vector<mwIndex>& subscripts) const {
  return mxCalcSingleSubscript(array_, subscripts.size(), &subscripts[0]);
}

void MxArray::set(mxArray* array, mwIndex index, mxArray* value) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_CHECK_NOTNULL(value);
  MEXPLUS_ASSERT(mxIsCell(array), "Expected a cell array.");
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  mxSetCell(array, index, value);
}

void MxArray::set(mwIndex index, mxArray* value) {
  MEXPLUS_ASSERT(isOwner(), "Must be an owner to set.");
  set(array_, index, value);
}

void MxArray::set(mxArray* array,
                  const std::string& field,
                  mxArray* value,
                  mwIndex index) {
  MEXPLUS_CHECK_NOTNULL(array);
  MEXPLUS_CHECK_NOTNULL(value);
  MEXPLUS_ASSERT(mxIsStruct(array), "Expected a struct array.");
  MEXPLUS_ASSERT(index < mxGetNumberOfElements(array),
                 "Index out of range: %u.",
                 index);
  if (!mxGetField(array, index, field.c_str()))
    MEXPLUS_ASSERT(mxAddField(array, field.c_str()) >= 0,
                   "Failed to create a field '%s'",
                   field.c_str());
  mxSetField(array, index, field.c_str(), value);
}

void MxArray::set(const std::string& field,
                  mxArray* value,
                  mwIndex index) {
  MEXPLUS_ASSERT(isOwner(), "Must be an owner to set.");
  set(array_, field, value, index);
}

} // namespace mexplus
