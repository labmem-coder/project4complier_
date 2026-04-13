program t;
var person: record name: string; age: integer; end;
begin
  person.name := 'Ada';
  person.age := 20;
  write(person.name, person.age)
end.
