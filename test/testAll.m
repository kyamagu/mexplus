function testAll
%TESTALL Test mexplus library.
  addpath(fileparts(mfilename('fullpath')));
  testMxTypes;
  testMxArray;
  testArguments;
  testDispatch;
  testSession;
end

function expectError(identifier, function_handle)
%EXPECTERROR Expect to throw the specified error identifier.
  try
    feval(function_handle);
    error('test:fail', 'Unexpected execution.');
  catch exception
    assert(strcmp(exception.identifier, identifier));
  end
end

function testDispatch
%DOTESTDISPATCH
  testDispatch_('foo');
  expectError('mexplus:dispatch:argumentError', @()testDispatch_());
  expectError('mexplus:dispatch:argumentError', @()testDispatch_('baz'));
  fprintf('PASS: %s\n', 'testDispatch');
end

function testSession
%DOTESTSESSION
  id = testSession_('create');
  testSession_('get', id);
  expectError('mexplus:session:notFound', @()testSession_('get', id + 1));
  assert(testSession_('exist', id));
  assert(~testSession_('exist', id + 1));
  testSession_('destroy', id);
  expectError('mexplus:session:notFound', @()testSession_('get', id));
  assert(~testSession_('exist', id));
  testSession_('clear');
  fprintf('PASS: %s\n', 'testSession');
end
