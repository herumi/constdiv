# constdiv
This is a PoC of constant integer division faster than compiler-generated code.

# Algorithm
M: integer >= 1.
d in [1, M]
take A >= d.
  c := (A + d - 1) // d.
  e := d c - A.
  M_d := M - ((M+1)%d).

Lemma.
1. By definition,
  0 <= e <= d-1 < A.
  M_d%d = d-1.
  d-1 <= M_d.
2. if e M_d < A, then e(d-1) < A = d c - e, so e < c.

Theorem
if e M_d < A, then x//d = (x c)//A for x in [0, M].

Proof
(q, r) := divmod(x, d). x = d q + r.
Then x c = d q c + r c = (A + e) q + r c = A q + (q e + r c).
y := q e + r c.
If 0 <= y < A, then q = (x c) // A.
So we prove that y < A if e M_d < A.
f(x) := dy = d q e + d r c = d q e + (A + e) r = (d q + r) e + r A = x e + r A.
So if max f(x) < d A, then max(y) = max f(x) / d < A.

e and A are constant values and >= 0, then arg_max f(x) = M_d or M.
i.e., (x, r) = (M_d, d-1) or (M, r0) where r0 := M % d.

Claim max f(x) = f(M_d) < d A.
case 1. M_d = M. then r0 = d-1. max f(x) = f(M_d) = M_d e + (d-1) A < A + (d-1) A = d A.
case 2. M_d < M. then r0 < d-1. max f(x) = max(f(M_d), f(M)).
M = M_d + 1 + r0.
f(M_d) - f(M) = (M_d e + (d-1)A) - (M e + r0 A) = (d-1 - r0) A - (r0 + 1)e
>(d-1 - (d-2)) A - ((d-2)+1)e = A - d e + e = d (c - e) > 0.
Then max f(x) = f(M_d) < d A.

This condition is the assumption of Thereom 1 in
"Integer division by constants: optimal bounds", Daniel Lemire, Colin Bartlett, Owen Kaser. 2021

