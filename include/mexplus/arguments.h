/** MEX function arguments helper library.
 *
 * Example: writing a MEX function that takes 2 input arguments and 1 optional
 *          flag, and returns one output.
 *
 * % myFunction.m
 * function result = myFunction(arg1, arg2, varargin)
 * 
 * // myFunction.cc
 * #include "mexplus/arguments.h"
 * using namespace std;
 * using namespace mexplus;
 * void mexFunction(int nlhs, mxArray* plhs[],
 *                  int nrhs, const mxArray* prhs) {
 *   InputArguments input(nrhs, prhs, 2, 1, "Flag");
 *   OutputArguments output(nlhs, &plhs, 1);
 *   vector<double> result = myFunction(input.get<vector<double> >(0),
 *                                      input.get<string>(1),
 *                                      input.get<int>("Flag", 0));
 *   output.set(0, result);
 * }
 *
 * % Build
 * >> mex myFunction.cc -lmex_arguments
 *
 * Kota Yamaguchi 2014 <kyamagu@cs.stonybrook.edu>
 */

#ifndef __MEXPLUS_ARGUMENTS_H__
#define __MEXPLUS_ARGUMENTS_H__

#include <map>
#include <mexplus/mxarray.h>

namespace mexplus {

/** Utility to parse input arguments.
 *
 * Example: parse 2 mandatory and 2 optional arguments.
 *
 *     InputArguments input(nrhs, prhs, 2, 2, "option1", "option2");
 *     myFunction2(input.get<double>(0),
 *                 input.get<int>(1),
 *                 input.get<string>("option1", "foo"),
 *                 input.get<int>("option2", 10));
 *
 * Example: parse 1 + 2 argument format or 2 + 2 argument format.
 *
 *     InputArguments input;
 *     input.define("format1", 1, 2, "option1", "option2");
 *     input.define("format2", 2, 2, "option1", "option2");
 *     input.parse(nrhs, prhs);
 *     if (input.is("format1"))
 *         myFunction(input.get<int>(0),
 *                    input.get<string>("option1", "foo"),
 *                    input.get<int>("option2", 10));
 *     else if (input.is("format2"))
 *         myFunction2(input.get<int>(0),
 *                     input.get<string>(1),
 *                     input.get<string>("option1", "foo"),
 *                     input.get<int>("option2", 10));
 *
 */
class InputArguments {
public:
  /** Definition of arguments.
   */
  typedef struct Definition_tag {
    std::vector<const mxArray*> mandatories;
    std::map<std::string, const mxArray*> optionals;
  } Definition;

  /** Empty constructor.
   */
  InputArguments() {}
  /** Shorthand constructor for a single argument definition.
   */
  InputArguments(int nrhs,
                 const mxArray* prhs[],
                 int mandatory_size = 1,
                 int option_size = 0,
                 ...);
  virtual ~InputArguments() {}
  /** Define a new argument format.
   */
  void define(const std::string name,
              int mandatory_size,
              int option_size = 0,
              ...);
  /** Parse arguments from mexFunction input.
   */
  void parse(int nrhs, const mxArray* prhs[]);
  /** Return which format is chosen.
   */
  bool is(const std::string& name) const;
  /** Get a parsed mandatory argument.
   */
  const mxArray* get(size_t index) const;
  /** Get a parsed mandatory argument.
   */
  template <typename T>
  T get(size_t index) const;
  template <typename T>
  void get(size_t index, T* value) const;
  /** Get a parsed optional argument.
   */
  const mxArray* get(const std::string& option_name) const;
  /** Get a parsed optional argument.
   */
  template <typename T>
  T get(const std::string& option_name, const T& default_value) const;
  template <typename T>
  void get(const std::string& option_name,
           const T& default_value,
           T* value) const;
  /** Access raw mxArray* pointer.
   */
  const mxArray* operator[] (size_t index) const { return get(index); }
  /** Access raw mxArray* pointer.
   */
  const mxArray* operator[] (const std::string& option_name) const {
    return get(option_name);
  }

private:
  /** Fill in optional arguments definition.
   */
  void fillOptionalDefinition(int option_size,
                              std::map<std::string, const mxArray*>* optionals,
                              va_list variable_list);
  /** Try to parse one definition or return false on failure.
   */
  bool parseDefinition(int nrhs,
                       const mxArray* prhs[],
                       Definition* definition);
  /** Format definitions.
   */
  std::map<std::string, Definition> definitions_;
  /** Last error message.
   */
  std::string error_message_;
};

template <typename T>
T InputArguments::get(size_t index) const {
  T value;
  get<T>(index, &value);
  return value;
}

template <typename T>
void InputArguments::get(size_t index, T* value) const {
  MxArray::to<T>(get(index), value);
}

template <typename T>
T InputArguments::get(const std::string& option_name,
                      const T& default_value) const {
  T value;
  get<T>(option_name, default_value, &value);
  return value;
}

template <typename T>
void InputArguments::get(const std::string& option_name,
                         const T& default_value,
                         T* value) const {
  MxArray option(get(option_name));
  if (option)
    option.to<T>(value);
  else
    *value = default_value;
}

/** Output arguments wrapper.
 *
 * Example:
 *
 *     OutputArguments output(nlhs, &plhs, 3);
 *     MxArray cell = MxArray::Cell(1, 3);
 *     cell.set(0, 0);
 *     cell.set(1, 1);
 *     cell.set(2, "value");
 *     output.set(0, 1);
 *     output.set(1, "foo");
 *     output.set(2, cell.release());
 */
class OutputArguments {
public:
  /** Construct output argument wrapper.
   */
  OutputArguments(int nlhs,
                  mxArray*** plhs,
                  int maximum_size = 1,
                  int mandatory_size = 0);
  /** Safely assign mxArray to the output.
   */
  void set(size_t index, mxArray* value);
  /** Safely assign T to the output.
   */
  template <typename T>
  void set(size_t index, const T& value) {
    set(index, MxArray::from(value));
  }
  /** Size of the output.
   */
  size_t size() const { return nlhs_; }
  /** Const square bracket operator.
   */
  mxArray* const& operator[] (size_t index) const;
  /** Mutable square bracket operator.
   */
  mxArray*& operator[] (size_t index);

private:
  /** Number of output arguments.
   */
  int nlhs_;
  /** Output argument array.
   */
  mxArray*** plhs_;
};

} // namespace mexplus

#endif // __MEXPLUS_ARGUMENTS_H__
