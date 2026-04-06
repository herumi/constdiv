// input: x
// output: x0 = x // d
// destroy: t
mov_imm(t, c << (64 - a));
umulh(x0, t, x); // x0 = (t * x) >> 64
