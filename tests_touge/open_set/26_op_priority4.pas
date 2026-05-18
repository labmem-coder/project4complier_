program main;
var a,b,c,d,e: integer;
flag:boolean;
begin
  read(a);
  read(b);
  read(c);
  read(d);
  read(e);
  flag := false;
  if ((a - b * c <> d - a / c) or (a * b / c = e + d) or (a + b + c = d + e)) then
  begin
    flag := true;
  end;
  if flag then
    write(1);
end.

File does not exist.

*** PARSE SUCCESSFUL ***

*** ERRORS: SEMANTIC (4) ***
  [Semantic][Line 10, Col 11] Undeclared identifier: 'false'
  [Semantic][Line 10, Col 3] Type mismatch in assignment to 'flag': cannot assign void to boolean
  [Semantic][Line 13, Col 13] Undeclared identifier: 'true'
  [Semantic][Line 13, Col 5] Type mismatch in assignment to 'flag': cannot assign void to boolean

Semantic analysis failed. Code generation skipped.