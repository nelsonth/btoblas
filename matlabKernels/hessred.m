HESSRED
in
	a : vector(column), b : vector(column), c : vector(column), d : vector(column),
	x : vector(column)
inout
	A : matrix(column)
out
	v : vector(column), w : vector(column)
{
	A = A - a * b' - c * d'
	v = A' * x
	w = A * x
}
