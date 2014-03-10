function runDatabase
%RUNDATABASE Demonstrates how the example Database API can be used.

  % Using a database object.
  database = Database('myDatabase.db');
  database.query('some-key');
  clear database;

  % Using static methods.
  environment = Database.getEnvironment();
  environment.code = 2;
  Database.setEnvironment(environment);

end