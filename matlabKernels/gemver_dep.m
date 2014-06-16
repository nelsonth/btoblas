GEMVER
in
  F : matrix(column), G : matrix(column),
  a : scalar, b : scalar,
  y : vector(column), z : vector(column)
out
  B : matrix(column), x : vector(column), w : vector(column)
{
  B = F + G
  x = b * (B' * y) + z
  w = a * (B * x)
}
