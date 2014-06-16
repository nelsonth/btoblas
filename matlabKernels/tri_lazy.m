HESSTRI
in
	Y : matrix(column), W : matrix(column), u : vector(column)
out
	y : vector(column)
{
	t = Y' * u
	s = W * t
	q = W' * u
	r = Y * q
	y = r-s
}
