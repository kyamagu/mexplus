function runDatabase
%RUNDATABASE Demonstrates how the example Database API can be used.

  % Using a database object.
  database = Database('myDatabase.db');
  value = database.query('some-key');
  disp(value);
  database.put('another-key', 'foo');
  value = database.query('another-key');
  disp(value);
  clear database;

  % Using static methods.
  environment = Database.getEnvironment();
  environment.code = 2;
  Database.setEnvironment(environment);

end
