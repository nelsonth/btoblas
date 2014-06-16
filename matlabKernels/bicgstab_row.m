BICGSTAB
in
  A : matrix(row), r0 : vector(column),
  w : scalar
inout
  x : vector(column), p : vector(column), r : vector(column)
out
  s : vector(column), Ap : vector(column), r1 : vector(column), As : vector(column),
  alpha : scalar, beta : scalar, omega : scalar
{
  Ap = A * p
  alpha = (r'* r0) / ( r0' * Ap)
  s = r - alpha * Ap
  As = A * s
  omega = (As' * s) / (As' * As)
  x = x + alpha * p + omega * s
  r1 = s - w * As
  beta = ((r1' * r0) / (r' * r0)) * (alpha / omega)
  p = r1 + beta * (p - omega * Ap)
  r = r1
}
