MEXPLUS
=======

C++ Matlab MEX development kit.

The kit contains a couple of C++ wrappers to make MEX development easy in
Matlab. There are 3 major components in the development kit.

 * `mexplus/mxarray.h` MxArray data conversion and access class.
 * `mexplus/arguments.h` MEX function argument wrappers.
 * `mexplus/dispatch.h` Helper to make a stateful MEX binary.

All classes are located in `mexplus` namespace.

Example
-------

Suppose we have a Database class in C++ and will design a MEX interface to it.
Create the following files.

`Database.m`

Matlab class interface file. Provides a wrapper to `Database_`
MEX function. For simplicity, just a constructor and a destructor is shown.

```matlab
classdef Database < handle
%DATABASE Hypothetical Matlab database API.
properties (Access = private)
  id_ % ID of the session.
end

methods
  function this = Database(filename)
  %DATABASE Create a new database.
    assert(ischar(filename));
    this.id_ = Database_('open', filename);
  end

  function delete(this)
  %DELETE Destructor.
    Database_('close', this.id_);
  end
end
```

`Database.cc`

C++ Implementation of MEX function. Provide MEX entry points by `MEX_DEFINE`
macro, and insert `MEX_MAIN` at the end. Notice how inputs and outputs are
wrapped by mexplus.

```c++
// Demonstration of Hypothetical MEX database API.
#include <mexplus/arguments.h>
#include <mexplus/dispatch.h>

using namespace std;
using namespace mexplus;

// Hypothetical database class.
class Database {
public:
  Database(const std::string& filename);
  virtual ~Database();
  // Other methods...
};

// Instantiate a Session manager.
template class mexplus::Session<Database>;

MEX_DEFINE(open) (int nlhs, mxArray* plhs[],
                  int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs, 1);
  OutputArguments output(nlhs, &plhs, 1);
  output.set(0, Session<Database>::create(
      new Database(input.get<std::string>(0))));
}

MEX_DEFINE(close) (int nlhs, mxArray* plhs[],
                   int nrhs, const mxArray* prhs[]) {
  InputArguments input(nrhs, prhs, 1);
  OutputArguments output(nlhs, &plhs, 0);
  Session<Database>::destroy(input.get(0));
}

// Other entry points...

MEX_MAIN
```

_Build_

The above files can be compiled by mex command. The development kit also
contains `make.m` build function to make a build process easier. The kit
depends on some of the C++11 features. In Linux, you might need to add `CFLAGS="$CFLAGS -std=c++01x"` to `mex` command to build.

```matlab
mex -Iinclude Database.cc -output Database_
```

See `example` directory for a complete demonstration.

Parsing arguments
-----------------

Include `mexplus/arguments.h` to use these wrappers.

### InputArguments

The class provides a wrapper around input arguments to validate and convert
Matlab variables. You can define a single or multiple input formats to accept.

Example: parse 1 mandatory argument.

```c++
// C++
InputArguments input(nrhs, prhs, 1);
myFunction(input.get<double>(0));
```

```matlab
% Matlab
myFunction(1.0);
```

Example: parse 2 mandatory and 2 optional arguments.

```c++
// C++
InputArguments input(nrhs, prhs, 2, 2, "option1", "option2");
myFunction(input.get<double>(0),
           input.get<int>(1),
           input.get<string>("option1", "foo"),
           input.get<int>("option2", 10));
```

```matlab
% Matlab
myFunction(1.0, 'option1', 'foo', 'option1', 'foo', ...
                                  'option2', 10);

```

Example: parse 1 + 2 argument format or 2 + 2 argument format.

```c++
// C++
InputArguments input;
input.define("format1", 1, 2, "option1", "option2");
input.define("format2", 2, 2, "option1", "option2");
input.parse(nrhs, prhs);
if (input.is("format1"))
    myFunction(input.get<int>(0),
               input.get<string>("option1", "foo"),
               input.get<int>("option2", 10));
else if (input.is("format2"))
    myFunction(input.get<int>(0),
               input.get<vector<double> >(1),
               input.get<string>("option1", "foo"),
               input.get<int>("option2", 10));
```

```matlab
% Matlab
myFunction(1.0, 'secondArg', 'option1', 'foo', ...
                             'option2', 10);
myFunction(1.0, [1,2,3,4], 'foo', 'option1', 'foo', ...
                                  'option2', 10);
```

### OutputArguments

```c++
OutputArguments output(nlhs, &plhs, 3);
output.set(0, 1);
output.set(1, "foo");
MxArray cell = MxArray::Cell(1, 3);
cell.set(0, 0);
cell.set(1, 1);
cell.set(2, "value");
output.set(2, cell.release());
```


Data conversion
---------------

Include `mexplus/mxarray.h` to use `MxArray` class, which is automatically
included in `mexplus/arguments.h`.

### MxArray

The MxArray class provides common data conversion methods between mxArray*
and C++ types, as well as serving itself as a unique_ptr to manage memory.
The static API's are the core of the high-level conversions.

```c++
int value = MxArray::to<int>(prhs[0]);
string value = MxArray::to<string>(prhs[0]);
vector<double> value = MxArray::to<vector<double> >(prhs[0]);

plhs[0] = MxArray::from(20);
plhs[0] = MxArray::from("text value.");
plhs[0] = MxArray::from(vector<double>(20, 0));
```

Additionally, object API's are there to wrap around a complicated data
construction.

```c++
// Read access.
MxArray cell(prhs[0]);   // Assumes a cell array in prhs[0].
int x = cell.at<int>(0);
vector<double> y = cell.at<vector<double> >(1);

MxArray struct_array(prhs[0]);   // Assumes a struct array in prhs[0].
int x = struct_array.at<int>("field1");
vector<double> y = struct_array.at<vector<double> >("field2");

MxArray numeric(prhs[0]);   // Assumes a numeric array in prhs[0].
double x = numeric.at<double>(0);
int y = numeric.at<int>(1);

// Write access.
MxArray cell(MxArray::Cell(1, 3));
cell.set(0, 12);
cell.set(1, "text value.");
cell.set(2, vector<double>(4, 0));
plhs[0] = cell.release();

MxArray struct_array(MxArray::Struct());
struct_array.set("field1", 12);
struct_array.set("field2", "text value.");
struct_array.set("field3", vector<double>(4, 0));
plhs[0] = struct_array.release();

MxArray numeric(MxArray::Numeric<double>(2, 2));
numeric.set(0, 1);
numeric.set(1, 2);
numeric.set(2, 3);
numeric.set(3, 4);
plhs[0] = numeric.release();
```

To add your own data conversion, define in namespace mexplus a template
specialization of MxArray::from() and MxArray::to().

```c++
class MyObject; // This is your custom data class.

namespace mexplus {
// Define two template specializations.
template <>
mxArray* MxArray::from(const MyObject& value) {
  // Write your conversion code.
}

template <>
void MxArray::to(const mxArray* array, MyObject* value) {
  // Write your conversion code.
}
} // namespace mexplus

// Then you can use any of the following.
MyObject object;
vector<MyObject> object_vector;
MxArray::to<vector<MyObject> >(prhs[0], &object_vector);
MxArray::to<vector<MyObject> >(prhs[1], &object);
plhs[0] = MxArray::from(object_vector);
plhs[1] = MxArray::from(object);
```

Dispatching calls
-----------------

Macros in `mexplus/dispatch.h` help to create a single MEX binary with multiple
function entries. Create a C++ file that looks like this:

```c++
//mylibrary.cc
#include <mexplus/dispatch.h>

MEX_DEFINE(myfunc) (int nlhs, mxArray* plhs[],
                    int nrhs, const mxArray* prhs[]) {
  // Do something.
}

MEX_DEFINE(myfunc2) (int nlhs, mxArray* plhs[],
                     int nrhs, const mxArray* prhs[]) {
  // Do another thing.
}

MEX_MAIN
```

Notice how `MEX_DEFINE` and `MEX_MAIN` macros are used. Then build this file
in Matlab.

```matlab
mex -Iinclude mylibrary.cc
```

The build MEX binaries can now calls two entry points by the first command
argument.

```matlab
mylibrary('myfunc', varargin{:})  % myfunc is called.
mylibrary('myfunc2', varargin{:}) % myfunc2 is called.
```

To prevent from unexpected use, it is a good practice to wrap these MEX calls
in a Matlab class or namescoped functions and place the MEX binary in a private
directory:

    @MyClass/MyClass.m
    @MyClass/myfunc.m
    @MyClass/myfunc2.m
    @MyClass/private/mylibrary.mex*

or,

    +mylibrary/myfunc.m
    +mylibrary/myfunc2.m
    +mylibrary/private/mylibrary.mex*

Inside of `myfunc.m` and `myfunc2.m`, call the `mylibrary` MEX function. This
pattern is very useful to design an API for C++ class objects.


Test
----

Run the following to run a test of mexplus.

```matlab
make test
```