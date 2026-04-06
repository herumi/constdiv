// input: x
// output: rax = x // d
// destroy: t
mov(t, c << (64 - a));
mulx(rax, t, x); // [rax:t] = x * t
