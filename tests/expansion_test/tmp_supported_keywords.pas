program kwtest;
const a = 1;
var arr: array[1..2] of integer; i: integer;
function f(x: integer): integer;
begin
  f := x
end;
procedure p;
begin
  writeln('ok')
end;
begin
  i := 1;
  if not (i = 2) then
    case i of
      1: begin write(arr[1]); break end;
      2: write('two')
    end;
  for i := 1 to 2 do
  begin
    continue;
    read(arr[i])
  end;
  readln(i);
  p
end.
