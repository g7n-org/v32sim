function fact(n)
  if n <= 1 then
    return 1
  end
  local m = fact(n - 1)
  return n * m
end
