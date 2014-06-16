GEMVER
in
  v0 : vector(column), v1 : vector(column), u0 : vector(column), u1 : vector(column)
inout
  A : matrix(row)
{
  A = A + v0 * u0' + v1 * u1'
}
