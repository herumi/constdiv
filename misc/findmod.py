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
    for s in [0]: #reversed(range(0, len_d)):
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
        print(f'''{hex(d)=} {d=}
{hex(M)=} {M=}
{a=} {s=}
c={hex(c)} {c.bit_length()=}
{e=} {yp=} {ym=}''')
        return (a, s, c)

L=0xac45a4010001a40200000000ffffffff
p=L*L+L+1

def mod(x, d, asc):
  (a, s, c) = asc
  q = ((x >> s) * c) >> a
  r = x - q * d
  if r < 0:
    r += d
  return r

def checkMod(x, d, asc):
  (a, s, c) = asc
  r1 = x % d
  r2 = mod(x, d, asc)
  if r1 != r2:
    raise Exception(f'ERR {x=} {d=} {r1=} {r2=}')

def testMod(d, M):
  asc = findMod(d, M)
  for x in range(100):
    checkMod(x, d, asc)
  print('OK')

testMod(7, 2**32-1)
