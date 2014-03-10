/** MEX function arguments helper library.
 *
 * Kota Yamaguchi 2013 <kyamagu@cs.stonybrook.edu>
 */

#include <mex.h>
#include <mexplus/arguments.h>
#include <sstream>
#include <stdarg.h>

namespace mexplus {

InputArguments::InputArguments(int nrhs,
                               const mxArray* prhs[],
                               int mandatory_size,
                               int option_size,
                               ...) {
  Definition* definition = &definitions_["default"];
  definition->mandatories.resize(mandatory_size, NULL);
  va_list variable_list;
  va_start(variable_list, option_size);
  fillOptionalDefinition(option_size, &definition->optionals, variable_list);
  va_end(variable_list);
  parse(nrhs, prhs);
}

void InputArguments::define(const std::string name,
                            int mandatory_size,
                            int option_size,
                            ...) {
  Definition* definition = &definitions_[name];
  definition->mandatories.resize(mandatory_size);
  va_list variable_list;
  va_start(variable_list, option_size);
  fillOptionalDefinition(option_size, &definition->optionals, variable_list);
  va_end(variable_list);
}

void InputArguments::parse(int nrhs, const mxArray* prhs[]) {
  if (definitions_.empty())
    mexErrMsgIdAndTxt("mexplus:arguments:error", "No format defined.");
  std::map<std::string, Definition>::iterator entry;
  std::vector<std::map<std::string, Definition>::iterator> delete_positions;
  for (entry = definitions_.begin(); entry != definitions_.end(); ++entry)
    if (!parseDefinition(nrhs, prhs, &entry->second))
      delete_positions.push_back(entry);
  for (int i = 0; i < delete_positions.size(); ++i)
    definitions_.erase(delete_positions[i]);
  if (definitions_.empty())
    mexErrMsgIdAndTxt("mexplus:arguments:error",
                      (error_message_.empty()) ?
                      "Invalid arguments." : error_message_.c_str());
  if (definitions_.size() > 1)
    mexWarnMsgIdAndTxt("mexplus:arguments:warning",
                       "Input arguments match more than one signature.");
}

bool InputArguments::is(const std::string& name) const {
  std::map<std::string, Definition>::const_iterator entry =
      definitions_.find(name);
  return (entry != definitions_.end());
}

const mxArray* InputArguments::get(size_t index) const {
  if (definitions_.empty())
    mexErrMsgIdAndTxt("mexplus:arguments:error", "No format defined.");
  const Definition& definition = definitions_.begin()->second;
  if (index >= definition.mandatories.size())
    mexErrMsgIdAndTxt("mexplus:arguments:error", "Index out of range.");
  return definition.mandatories[index];
}

const mxArray* InputArguments::get(const std::string& option_name) const {
  if (definitions_.empty())
    mexErrMsgIdAndTxt("mexplus:arguments:error", "No format defined.");
  const Definition& definition = definitions_.begin()->second;
  std::map<std::string, const mxArray*>::const_iterator entry =
      definition.optionals.find(option_name);
  if (entry == definition.optionals.end())
    mexErrMsgIdAndTxt("mexplus:arguments:error",
                      "Unknown option %s.",
                      option_name.c_str());
  return entry->second;
}

void InputArguments::fillOptionalDefinition(
    int option_size,
    std::map<std::string, const mxArray*>* optionals,
    va_list variable_list) {
  for (int i = 0; i < option_size; ++i) {
    const char* option_name = va_arg(variable_list, const char*);
    (*optionals)[std::string(option_name)] = NULL;
  }
}

bool InputArguments::parseDefinition(int nrhs,
                                     const mxArray* prhs[],
                                     Definition* definition) {
  std::stringstream message;
  if (nrhs < definition->mandatories.size()) {
    message << "Too few arguments: " << nrhs << " for at least "
            << definition->mandatories.size() << ".";
    error_message_.assign(message.str());
    return false;
  }
  int index = 0;
  for (; index < definition->mandatories.size(); ++index)
    definition->mandatories[index] = prhs[index];
  for (; index < nrhs; ++index) {
    // Check if option name is valid.
    const mxArray* option_name_array = prhs[index];
    if (!mxIsChar(option_name_array)) {
      message << "Option name must be char but is given "
              << mxGetClassName(option_name_array) << ".";
      error_message_.assign(message.str());
      return false;
    }
    char option_name[64];
    if (mxGetString(option_name_array, option_name, sizeof(option_name))) {
      message << "Option name too long.";
      error_message_.assign(message.str());
      return false;
    }
    std::map<std::string, const mxArray*>::iterator entry =
        definition->optionals.find(option_name);
    if (entry == definition->optionals.end()) {
      message << "Invalid option name: '" << option_name << "'.";
      error_message_.assign(message.str());
      return false;
    }
    // Check if options are even.
    if (++index >= nrhs) {
      message << "Missing option value for option '" << option_name << "'.";
      error_message_.assign(message.str());
      return false;
    }
    if (entry->second)
      mexErrMsgIdAndTxt("mexplus:arguments:warning",
                        "Option '%s' appeared more than once.",
                        option_name);
    entry->second = prhs[index];
  }
  return true;
}

OutputArguments::OutputArguments(int nlhs,
                                 mxArray*** plhs,
                                 int maximum_size,
                                 int mandatory_size) :
    nlhs_(nlhs),
    plhs_(plhs) {
  if (mandatory_size > nlhs)
    mexErrMsgIdAndTxt("mexplus:arguments:error",
                      "Too few output: %d for %d.",
                      nlhs,
                      mandatory_size);
  if (maximum_size < nlhs)
    mexErrMsgIdAndTxt("mexplus:arguments:error",
                      "Too many output: %d for %d.",
                      nlhs,
                      maximum_size);
}

void OutputArguments::set(size_t index, mxArray* value) {
  if (index < nlhs_)
    (*plhs_)[index] = value;
}

mxArray* const& OutputArguments::operator[] (size_t index) const {
  if (index < nlhs_)
    mexErrMsgIdAndTxt("mexplus:arguments:error",
                      "Output index out of range: %d.",
                      index);
  return (*plhs_)[index];
}

mxArray*& OutputArguments::operator[] (size_t index) {
  if (index < nlhs_)
    mexErrMsgIdAndTxt("mexplus:arguments:error",
                      "Output index out of range: %d.",
                      index);
  return (*plhs_)[index];
}

} // namespace mexplus
