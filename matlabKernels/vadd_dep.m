vadd
in
  w : vector(column), x : vector(column), y : vector(column), z : vector(column)
out
  r : vector(column)
{
  t = x + y
  s = z + t
  u = w + s
  r = u + t
}

