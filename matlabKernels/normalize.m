normalize
in
	x : vector(column)
out
	a : vector(column)
{
	a = x / squareroot(x' * x)
}
