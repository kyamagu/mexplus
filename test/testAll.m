function testAll
%TESTALL Test mexplus library.
  addpath(fileparts(mfilename('fullpath')));
  tests = { ...
    @testMxTypes, ...
    @testMxArray, ...
    @testArguments, ...
    @testDispatch, ...
    @testSession, ...
    @testString};
  passed = 0;
  for i = 1:numel(tests)
    try
      feval(tests{i});
      fprintf('=> PASS: %s\n', func2str(tests{i}));
      passed = passed + 1;
    catch exception
      disp(exception.message);
      fprintf('=> FAIL: %s\n', func2str(tests{i}));
    end
  end
  fprintf('%d of %d passed\n', passed, numel(tests));
  if passed ~= numel(tests)
    exit(1);
  end
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
%TESTDISPATCH
  testDispatch_('foo');
  expectError('mexplus:dispatch:argumentError', @()testDispatch_());
  expectError('mexplus:dispatch:argumentError', @()testDispatch_('baz'));
  fprintf('PASS: %s\n', 'testDispatch');
end

function testSession
%TESTSESSION
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

function testString
%TESTSTRING
  fixtures = {char([0, 127, 128, 255]), uint8([0, 127, 128, 255])};
  for i = 1:numel(fixtures)
    value = fixtures{i};
    value_type = class(value);
    %fprintf(' %d', value(:)); fprintf('\n');
    returned_value = cast(testString_(fixtures{i}), value_type);
    %fprintf(' %d', returned_value(:)); fprintf('\n');
    assert(numel(returned_value) == numel(value) && ...
           all(returned_value(:) == value(:)));
  end
  fprintf('PASS: %s\n', 'testString');
end
