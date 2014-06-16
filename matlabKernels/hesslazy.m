HESSLAZY
in
	Y : matrix(orientation=column), U : matrix(orientation=column),
	Z : matrix(orientation=column), u : vector(orientation=column)
inout
	y : vector(orientation=column), z : vector(orientation=column)
{
	t = U' * u
	y = y - Y * t - U * (Z' * u)
	z = z - U * (Y' * u) - Z * t
}
