program test(input, output);
var x, y : integer;
begin
  read(x, y);
  if x > y then
    write(x)
  else
    write(y);
  for x := 1 to 10 do
    y := y + x
end.
