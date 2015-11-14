/** Demonstration of mexplus library.
 *
 * In this example, we create MEX APIs for the hypothetical Database class in
 * Matlab.
 *
 */
#include <mexplus.h>

using namespace std;
using namespace mexplus;

// Hypothetical database class to be MEXed. This example is a proxy to C++ map.
class Database {
public:
  // Database constructor. This is a stub.
  Database(const string& filename) {
    mexPrintf("Opened '%s'.\n", filename.c_str());
  }
  // Database destructor.
  virtual ~Database() {}
  // Query a record.
  string query(const string& key) const {
    mexPrintf("Querying '%s'.\n", key.c_str());
    map<string, string>::const_iterator record  = records_.find(key);
    return (record != records_.end()) ? record->second : "Not Found";
  }
  // Put a record.
  void put(const string& key, const string& value) {
    mexPrintf("Putting '%s':'%s'.\n", key.c_str(), value.c_str());
    records_.insert(pair<string, string>(key, value));
  }

private:
  // Database implementation.
  map<string, string> records_;
};

// Instance manager for Database.
template class mexplus::Session<Database>;

namespace {

// Defines MEX API for new.
MEX_DEFINE(new) (int nlhs, mxArray* plhs[],
                 int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs, 1);
  OutputArguments output(nlhs, plhs, 1);
  output.set(0, Session<Database>::create(new Database(input.get<string>(0))));
}

// Defines MEX API for delete.
MEX_DEFINE(delete) (int nlhs, mxArray* plhs[],
                    int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs, 1);
  OutputArguments output(nlhs, plhs, 0);
  Session<Database>::destroy(input.get(0));
}

// Defines MEX API for query (const method).
MEX_DEFINE(query) (int nlhs, mxArray* plhs[],
                   int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs, 2);
  OutputArguments output(nlhs, plhs, 1);
  const Database& database = Session<Database>::getConst(input.get(0));
  output.set(0, database.query(input.get<string>(1)));
}

// Defines MEX API for set (non const method).
MEX_DEFINE(put) (int nlhs, mxArray* plhs[],
                 int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs, 3);
  OutputArguments output(nlhs, plhs, 0);
  Database* database = Session<Database>::get(input.get(0));
  database->put(input.get<string>(1), input.get<string>(2));
}

} // namespace

MEX_DISPATCH // Don't forget to add this if MEX_DEFINE() is used.
