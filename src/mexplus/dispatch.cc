/** MEX dispatch library.
 *
 * Kota Yamaguchi 2013 <kyamagu@cs.stonybrook.edu>
 */

#include <mexplus/dispatch.h>
#include <memory>

namespace mexplus {

void OperationFactory::define(const std::string& name,
                              OperationCreator* creator) {
  registry()->insert(make_pair(name, creator));
}

Operation* OperationFactory::create(const std::string& name) {
  std::map<std::string, OperationCreator*>::const_iterator it =
      registry()->find(name);
  if (it == registry()->end())
    return static_cast<Operation*>(NULL);
  else
    return it->second->create();
}

std::map<std::string, OperationCreator*>* OperationFactory::registry() {
  static std::map<std::string, OperationCreator*> registry_table;
  return &registry_table;
}

OperationCreator::OperationCreator(const std::string& name) {
  OperationFactory::define(name, this);
}

OperationCreator::~OperationCreator() {}

} // namespace mexplus

/** Main entry of the mex function.
 */
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
  if (nrhs < 1 || !mxIsChar(prhs[0]))
    mexErrMsgIdAndTxt("mexplus:dispatch:argumentError",
        "Invalid argument: missing operation.");
  std::string operation_name(
      mxGetChars(prhs[0]),
      mxGetChars(prhs[0]) + mxGetNumberOfElements(prhs[0]));
  std::auto_ptr<mexplus::Operation> operation(
      mexplus::OperationFactory::create(operation_name));
  if (operation.get() == NULL)
    mexErrMsgIdAndTxt("mexplus:dispatch:argumentError",
        "Invalid operation: %s", operation_name.c_str());
  (*operation)(nlhs, plhs, nrhs - 1, prhs + 1);
}
