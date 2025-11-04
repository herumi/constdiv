def findMod(d, M=2**32-1):
  if d & 1 == 0:
    raise Exception('d must be odd', d)
  len_d = d.bit_length()
  len_M = M.bit_length()
  (q_M, r_M) = divmod(M, d)
  for a in range(len_d, len_d + len_M + 1):
    A = 1 << a
    c = (A + d - 1) // d
    e = d * c - A
    for s in reversed(range(1, len_d)):
      S = 1 << s
      dS = d * S
      u = pow(d, -1, S)
      v = pow(S, -1, d)
      assert((u * d + v * S) % dS == 1)
      xp = (-v * S) % dS + (M//dS)*dS
      xm = (u * d) % dS + dS
      def y(x):
        (q, r) = divmod(x, d)
        (H, L) = divmod(x, S)
        return q * e + (r - L) * c
      yp = y(xp)
      ym = y(xm)
      if yp - ym < 2 * A:
        print(f'd={d} a={a} c={hex(c)} e={e}')
        print(f's={s} xp={xp} yp={yp} xm={xm} ym={ym} yp-ym={yp-ym}')
        return


findMod(7)
