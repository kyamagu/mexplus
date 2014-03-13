classdef Database < handle
%DATABASE Example usage of the mexplus development kit.
%
% This class definition gives an interface to the underlying MEX functions
% built in the private directory. It is a good practice to wrap MEX functions
% with Matlab script so that the API is well documented and separated from
% its C++ implementation. Also such a wrapper is a good place to validate
% input arguments.
%
% Build
% -----
%
%    make
%
% See `make.m` for details.
%

properties (Access = private)
  id_ % ID of the session.
end

methods
  function this = Database(filename)
  %DATABASE Create a new database.
    assert(ischar(filename));
    this.id_ = Database_('new', filename);
  end

  function delete(this)
  %DELETE Destructor.
    Database_('delete', this.id_);
  end

  function result = query(this, key)
  %QUERY Query something to the database.
    assert(isscalar(this));
    result = Database_('query', this.id_, key);
  end

  function put(this, key, value)
  %PUT Save something to the database.
    assert(isscalar(this));
    Database_('put', this.id_, key, value);
  end
end

methods (Static)
  function environment = getEnvironment()
  %GETENVIRONMENT Get environment info.
    environment = Database_('getEnvironment');
  end

  function setEnvironment(environment)
  %SETENVIRONMENT Set environment info.
    Database_('setEnvironment', environment);
  end
end

end
