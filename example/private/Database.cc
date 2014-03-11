/** Demonstration of mexplus library.
 *
 * In this example, we create 3 MEX API for the hypothetical Database class for
 * Matlab.
 *
 */

#include <mexplus/arguments.h>
#include <mexplus/dispatch.h>

using namespace std;
using namespace mexplus;

/** Hypothetical database class that has 3 methods.
 */
class Database {
public:
  /** Database constructor.
   */
  Database(const string& filename) {
    mexPrintf("Opened '%s'.\n", filename.c_str());
  }
  /** Database destructor.
   */
  virtual ~Database() {}
  /** Database query.
   */
  string query(const string& key) {
    mexPrintf("Querying '%s'.\n", key.c_str());
    return key;
  }
};

// Explicitly instantiate a Session manager.
template class mexplus::Session<Database>;

namespace {

// Defines MEX API for open.
MEX_DEFINE(open) (int nlhs, mxArray* plhs[],
                  int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs);
  OutputArguments output(nlhs, &plhs);
  output.set(0, Session<Database>::create(new Database(input.get<string>(0))));
}

// Defines MEX API for close.
MEX_DEFINE(close) (int nlhs, mxArray* plhs[],
                   int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs);
  OutputArguments output(nlhs, &plhs, 0);
  Session<Database>::destroy(input.get(0));
}

// Defines MEX API for query.
MEX_DEFINE(query) (int nlhs, mxArray* plhs[],
                   int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs, 2);
  OutputArguments output(nlhs, &plhs);
  Database* database = Session<Database>::get(input.get(0));
  output.set(0, database->query(input.get<string>(1)));
}

} // namespace

MEX_MAIN
