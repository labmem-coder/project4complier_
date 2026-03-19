program example(input, output);
const MAX = 100;
var arr : array[1..10] of integer;
    i, sum : integer;
procedure init(n : integer);
var j : integer;
begin
  for j := 1 to n do
    arr[j] := j * 2
end;
begin
  init(10);
  sum := 0;
  for i := 1 to 10 do
    sum := sum + arr[i];
  write(sum)
end.
