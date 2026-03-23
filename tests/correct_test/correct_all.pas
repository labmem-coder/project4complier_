program ExtendedPascalTest;

var
  str1, str2: string;
  ch: char;
  i: integer;

function GetNumber: integer;
begin
  GetNumber := 42;
end;

begin
  { 字符串操作测试 }
  str1 := 'Hello';
  ch := '!';
  write(str1);
  
  { switch-case语句测试 }
  i := 2;
  case i of
    1: write('One');
    2: begin
         write('Two');
         break;  { break语句测试 }
       end;
    3: write('Three');
  end;
  
  { continue语句测试 }
  for i := 1 to 5 do
  begin
    if i = 3 then continue;
    write(i);
  end;
end.