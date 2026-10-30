-- bench.lua: a small program for the source-level debugger tests
local t = 0
local function add(a, b)
  local s = a + b
  return s
end
for i = 1, 3 do
  t = add(t, i)
end
local r = fact(4)
print(t)
return t
