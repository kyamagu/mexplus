/** MEX dispatch library.
 *
 * Copyright 2014 Kota Yamaguchi.
 *
 * This helper contains MEX_DEFINE() macro to help create a dispatchable MEX
 * file. Two files are required to create a new mex function. Suppose you are
 * creating two MEX functions `myfunc` and `myfunc2`. Then, make the following
 * files.
 *
 * myfunc.m
 *
 *     function output_args = myfunc(varargin)
 *     %MYFUNC Description of the function.
 *     %
 *     %   Details go here.
 *     %
 *       output_args = mylibrary(mfilename, varargin{:})
 *     end
 *
 * myfunc2.m
 *
 *     function output_args = myfunc(varargin)
 *     %MYFUNC Description of the function.
 *     %
 *     %   Details go here.
 *     %
 *       output_args = mylibrary(mfilename, varargin{:})
 *     end
 *
 * These files contains help documentation and a line to invoke the mex
 * function.
 *
 * mylibrary.cc
 *
 *     #include <mexplus/dispatch.h>
 *
 *     MEX_DEFINE(myfunc) (int nlhs, mxArray* plhs[],
 *                         int nrhs, const mxArray* prhs[]) {
 *       ...
 *     }
 *
 *     MEX_DEFINE(myfunc2) (int nlhs, mxArray* plhs[],
 *                          int nrhs, const mxArray* prhs[]) {
 *       ...
 *     }
 *
 *     MEX_DISPATCH
 *
 * This file is the implementation of the mex function. The MEX_DEFINE macro
 * defines an entry point of the function. MEX_DISPATCH macro at the end
 * inserts necessary codes to dispatch function calls to an appropriate
 * function.
 *
 * Similarly, you can write another pair of .m (and C++) file to add to your
 * library. You may split MEX_DEFINE macros in multiple C++ files. In that
 * case, have MEX_DISPATCH macro in one of the files.
 *
 */

#ifndef INCLUDE_MEXPLUS_DISPATCH_H_
#define INCLUDE_MEXPLUS_DISPATCH_H_

#include <mex.h>
#include <map>
#include <memory>
#include <string>

#ifndef MEXPLUS_AT_EXIT
#define MEXPLUS_AT_EXIT
#endif

#ifndef MEXPLUS_AT_INIT
#define MEXPLUS_AT_INIT
#endif

#ifndef MEXPLUS_AT_ERROR
#define MEXPLUS_AT_ERROR(name)
#endif

namespace mexplus {

typedef bool OperationNameAdmitter(const std::string& name);

class OperationCreator;
inline void CreateOperation(OperationNameAdmitter* admitter,
                            OperationCreator* creator);

/** Abstract operation class. Child class must implement operator().
 */
class Operation {
 public:
  /** Destructor.
   */
  virtual ~Operation() {}
  /** Execute the operation.
   */
  virtual void operator()(int nlhs,
                          mxArray *plhs[],
                          int nrhs,
                          const mxArray *prhs[]) = 0;
};

/** Base class for operation creators.
 */
class OperationCreator {
 public:
  /** Register an operation in the constructor.
   */
  explicit OperationCreator(OperationNameAdmitter* admitter) {
    CreateOperation(admitter, this);
  }
  /** Destructor.
   */
  virtual ~OperationCreator() {}
  /** Implementation must return a new instance of the operation.
   */
  virtual Operation* create() = 0;
};

/** Implementation of the operation creator to be used as composition in an
 * Operator class.
 */
template <class OperationClass>
class OperationCreatorImpl : public OperationCreator {
 public:
  explicit OperationCreatorImpl(OperationNameAdmitter* admitter,
                                const char* tag) :
      OperationCreator(admitter) {
    if (tag)
      mexPrintf("Tag: %s\n", tag);
  }
  virtual Operation* create() { return new OperationClass; }
};

/** Factory class for operations.
 */
class OperationFactory {
 public:
  typedef std::map<OperationNameAdmitter*, OperationCreator*> RegistryMap;

  /** Register a new creator.
   */
  friend void CreateOperation(OperationNameAdmitter* admitter,
                              OperationCreator* creator);
  /** Create a new instance of the registered operation.
   */
  static Operation* create(const std::string& name) {
    RegistryMap::const_iterator it = find(name);
    return (it == registry()->end()) ?
      static_cast<Operation*>(NULL) : it->second->create();
  }
  /** Obtain a pointer to the registration table.
   */
  static RegistryMap* registry() {
    static RegistryMap registry_table;
    return &registry_table;
  }

 private:
  static RegistryMap::const_iterator find(const std::string& name) {
    RegistryMap::const_iterator it;
    for (it = registry()->begin(); it != registry()->end(); it++) {
      if ((*it->first)(name))
        return it;
    }
    return it;
  }
};

/** Register a new creator in OperationFactory.
 */
inline void CreateOperation(OperationNameAdmitter* admitter,
                            OperationCreator* creator) {
  OperationFactory::registry()->insert(std::make_pair(admitter, creator));
}

/** Key-value storage to make a stateful MEX function.
 *  \code
 *    #include <mexplus/dispatch.h>
 *
 *    using namespace std;
 *    using namespace mexplus;
 *
 *    class Database;
 *
 *    template class Session<Database>;
 *
 *    MEX_DEFINE(open) (int nlhs, mxArray* plhs[],
 *                      int nrhs, const mxArray* prhs[]) {
 *      unique_ptr<Database> database(new Database(...));
 *      database->open(...);
 *      intptr_t session_id = Session<Database>::create(database.release());
 *      plhs[0] = mxCreateDoubleScalar(session_id);
 *    }
 *
 *    MEX_DEFINE(query) (int nlhs, mxArray* plhs[],
 *                       int nrhs, const mxArray* prhs[]) {
 *      intptr_t session_id = mxGetScalar(prhs[0]);
 *      Database* database = Session<Database>::get(session_id);
 *      database->query(...);
 *    }
 *
 *    MEX_DEFINE(close) (int nlhs, mxArray* plhs[],
 *                       int nrhs, const mxArray* prhs[]) {
 *      intptr_t session_id = mxGetScalar(prhs[0]);
 *      Session<Database>::destroy(session_id);
 *    }
 * \endcode
 */
template<class T>
class Session {
 public:
  typedef std::map<intptr_t, std::shared_ptr<T> > InstanceMap;

  /** Create an instance.
   */
  static intptr_t create(T* instance) {
    InstanceMap* instances = getInstances();
    intptr_t id = reinterpret_cast<intptr_t>(instance);
    instances->insert(std::make_pair(id, std::shared_ptr<T>(instance)));
    mexLock();
    return id;
  }
  /** Destroy an instance.
   */
  static void destroy(intptr_t id) {
    getInstances()->erase(id);
    mexUnlock();
  }
  static void destroy(const mxArray* pointer) {
    destroy(getIntPointer(pointer));
  }
  /** Retrieve an instance or throw if no instance is found.
   */
  static T* get(intptr_t id) {
    InstanceMap* instances = getInstances();
    typename InstanceMap::iterator instance = instances->find(id);
    if (instance == instances->end())
      mexErrMsgIdAndTxt("mexplus:session:notFound",
                        "Invalid id %d. Did you create?",
                        id);
    return instance->second.get();
  }
  static T* get(const mxArray* pointer) {
    return get(getIntPointer(pointer));
  }
  /** Retrieve a const instance or throw if no instance is found.
   */
  static const T& getConst(intptr_t id) {
    return *get(id);
  }
  static const T& getConst(const mxArray* pointer) {
    return getConst(getIntPointer(pointer));
  }
  /** Check if the given id exists.
   */
  static bool exist(intptr_t id) {
    InstanceMap* instances = getInstances();
    typename InstanceMap::iterator instance = instances->find(id);
    return instance != instances->end();
  }
  static bool exist(const mxArray* pointer) {
    return exist(getIntPointer(pointer));
  }
  /** Clear all session instances.
   */
  static void clear() {
    for (int i = 0; i < getInstances()->size(); ++i)
      mexUnlock();
    getInstances()->clear();
  }
  /** Get instance map.
   */
  static const InstanceMap& getInstanceMap() { return *getInstances(); }

 private:
  /** Constructor prohibited.
   */
  Session() {}
  ~Session() {}
  /** Convert mxArray to intptr_t.
   */
  static intptr_t getIntPointer(const mxArray* pointer) {
    if (mxIsEmpty(pointer))
      mexErrMsgIdAndTxt("mexplus:session:invalidType", "Id is empty.");
    if (sizeof(intptr_t) == 8 && !mxIsInt64(pointer) && !mxIsUint64(pointer))
      mexErrMsgIdAndTxt("mexplus:session:invalidType",
                        "Invalid id type %s.",
                        mxGetClassName(pointer));
    if (sizeof(intptr_t) == 4 && !mxIsInt32(pointer) && !mxIsUint32(pointer))
      mexErrMsgIdAndTxt("mexplus:session:invalidType",
                        "Invalid id type %s.",
                        mxGetClassName(pointer));
    return *reinterpret_cast<intptr_t*>(mxGetData(pointer));
  }
  /** Get static instance storage.
   */
  static InstanceMap* getInstances() {
    static InstanceMap instances;
    return &instances;
  }
};

}  // namespace mexplus

/** Define a MEX API function. Example:
 *
 * MEX_DEFINE(myfunc) (int nlhs, mxArray *plhs[],
 *                     int nrhs, const mxArray *prhs[]) {
 *   if (nrhs != 1 || nlhs > 1)
 *     mexErrMsgTxt("Wrong number of arguments.");
 *   ...
 * }
 */
#define MEX_DEFINE(name) \
class Operation_##name : public mexplus::Operation { \
 public: \
  virtual void operator()(int nlhs, \
                          mxArray *plhs[], \
                          int nrhs, \
                          const mxArray *prhs[]); \
 private: \
  static bool Operation_Admitter(const std::string& func) { \
    return func == #name;\
  } \
  static const mexplus::OperationCreatorImpl<Operation_##name> creator_; \
}; \
const mexplus::OperationCreatorImpl<Operation_##name> \
    Operation_##name::creator_(Operation_##name::Operation_Admitter, NULL); \
void Operation_##name::operator()

/** Define a MEX API function using a private admitter. Example:
 *
 * static bool myfunc_admitter(std::string& name) {
 *   return name == "myfunc";
 * }
 *
 * MEX_DEFINE2(myfunc, myfunc_admitter) (int nlhs, mxArray *plhs[],
 *                                       int nrhs, const mxArray *prhs[]) {
 *   if (nrhs != 1 || nlhs > 1)
 *     mexErrMsgTxt("Wrong number of arguments.");
 *   ...
 * }
 */
#define MEX_DEFINE2(name, admitter) \
static const char* tag = NULL /*#name*/; \
class Operation_##name : public mexplus::Operation { \
 public: \
  virtual void operator()(int nlhs, \
                          mxArray *plhs[], \
                          int nrhs, \
                          const mxArray *prhs[]); \
 private: \
  static const mexplus::OperationCreatorImpl<Operation_##name> creator_; \
}; \
const mexplus::OperationCreatorImpl<Operation_##name> \
    Operation_##name::creator_(admitter, tag); \
void Operation_##name::operator()

/** Insert a function dispatching code. Use once per MEX binary.
 */
#define MEX_DISPATCH \
void mexFunction(int nlhs, mxArray *plhs[], \
                 int nrhs, const mxArray *prhs[]) { \
  MEXPLUS_AT_INIT;\
  if (nrhs < 1 || !mxIsChar(prhs[0])) \
    mexErrMsgIdAndTxt("mexplus:dispatch:argumentError", \
                      "Invalid argument: missing operation."); \
  std::string operation_name(\
      mxGetChars(prhs[0]), \
      mxGetChars(prhs[0]) + mxGetNumberOfElements(prhs[0])); \
  std::unique_ptr<mexplus::Operation> operation(\
      mexplus::OperationFactory::create(operation_name)); \
  if (operation.get() == NULL) { \
    MEXPLUS_AT_ERROR(operation_name); \
    mexErrMsgIdAndTxt("mexplus:dispatch:argumentError", \
        "Invalid operation: %s", operation_name.c_str()); \
  } \
  (*operation)(nlhs, plhs, nrhs - 1, prhs + 1); \
  MEXPLUS_AT_EXIT; \
}

#endif  // INCLUDE_MEXPLUS_DISPATCH_H_
