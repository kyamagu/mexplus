MEXPLUS
=======

C++ Matlab MEX development kit.

The kit contains a couple of C++ wrappers to make MEX development easy in
Matlab. There are 3 major components of the development kit.

 * `mexplus/mxarray.h` MxArray data conversion and access class.
 * `mexplus/arguments.h` MEX function argument wrappers.
 * `mexplus/dispatch.h` Helper to make a stateful MEX binary.

Example
-------

Suppose we have a Database class in C++ and will design a MEX interface to it.
Create the following files.

`Database.m`: Matlab class interface file. Provides a wrapper to `Database_`
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

`Database.cc`: C++ Implementation of MEX function. Provide MEX entry points by
`MEX_DEFINE` macro. Notice how inputs and outputs are wrapped by mexplus.

```c++
// Demonstration of Hypothetical MEX database API.
#include <mexplus/arguments.h>
#include <mexplus/dispatch.h>

// Hypothetical database class that has 3 methods.
class Database {
public:
  Database(const std::string& filename);
  virtual ~Database();
};

// Explicitly instantiate a Session manager.
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
```

Build: The above files can be compiled by mex command. The development kit also
contains `make.m` build function to make a build process easier. The kit
depends on some of the C++11 features. In Linux, you might need to add `CFLAGS="$CFLAGS -std=c++01x"` to `mex` command to build.

```matlab
mex -Iinclude Database.cc src/mexplus/arguments.cc src/mexplus/dispatch.cc \
    src/mexplus/mxarray.cc -output Database_
```

See `example` directory for a complete demonstration.


Test
----

Run the following to run a test of mexplus.

```matlab
make test
```