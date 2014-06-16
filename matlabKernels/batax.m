BATAX
in
  A : matrix(orientation=row), x : vector(orientation=column), beta : scalar
out
  y : vector(orientation=column)
{
  y = beta * (A' * (A * x))
}
