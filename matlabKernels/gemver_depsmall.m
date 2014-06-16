GEMVER
in
  F : matrix(column), G : matrix(column),
  y : vector(column)
out
  B : matrix(column), x : vector(column), w : vector(column)
{
  B = F + G
  x = (B' * y)
  w = (B * x)
}
