program test(input, output);
var a, b : integer;
function max(x, y : integer) : integer;
begin
  if x > y then max := x
  else max := y
end;
begin
  read(a, b);
  write(max(a, b))
end.
