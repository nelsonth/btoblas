DGEMV
in
  A : matrix(row), B : matrix(row), x : vector(column)
out
  C : matrix(column)
{
	t = A*x
	f = B*t
	C = t*f'
}

