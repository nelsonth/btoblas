GEMVER
in
  u1 : vector(column), u2 : vector(column), 
  v1 : vector(column), v2 : vector(column),
  a : scalar, b : scalar,
  y : vector(column), z : vector(column)
inout
  A : matrix(column), z : vector(column)
out
  x : vector(column), w : vector(column)
{
  A = A + u1 * v1' + u2 * v2'
  x = A' * y
  z = b * x + z
  w = a * (A * z)
}
