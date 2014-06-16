HESSTEST
in
	x : vector(column), A : matrix(column)
out
	v : vector(column), w : vector(column)
{
	v = A' * x
	w = A * x
}
