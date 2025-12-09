import random

def gcd(d, S):
  while S != 0:
    (d, S) = (S, d % S)
  return d

def findMod(d, M=2**32-1, slist=[]):
#  if d & 1 == 0:
#    raise Exception('d must be odd', d)
  len_d = d.bit_length()
  len_M = M.bit_length()
  (q_M, r_M) = divmod(M, d)
  if slist == []:
    slist = reversed(range(0, len_d))
  print(f'{list(slist)=}')
  candi = []
  for a in range(len_d, len_d + len_M + 1):
    A = 1 << a
    maxV = A * 2
    c = (A + d - 1) // d
    e = d * c - A
    if e >= c:
      continue

    for s in reversed(range(0, len_d)):
      S = 1 << s
      dS = d * S
      g = gcd(d, S)
      if d == g:
        break
      d_red = d // g
      S_red = S // g
      u = pow(d_red, -1, S_red)
      v = pow(S_red, -1, d_red)
      assert((u * d + v * S) % dS == g)

      # max(r - L) is r = d-g, L = 0
      # min(r - L) is r = 0, L = S-g
      # x = a v S + b u d if x % d = a g, x % S = b g
      xmax0 = ((d_red - 1) * v * S) % dS
      xmin0 = ((S_red - 1) * u * d) % dS
      xmax = xmax0 + (M//dS)*dS
      if xmax >= M:
        xmax -= dS
      xmin = xmin0
      def get_y(x):
        (q, r) = divmod(x, d)
        (H, L) = divmod(x, S)
        return (q * e + (r - L) * c, r, L)
      (ymax, r, L) = get_y(xmax)
      assert(r == d-g and L == 0)
      (ymin, r, L) = get_y(xmin)
      assert(r == 0 and L == S-g)

      (q, r) = divmod(xmin, d)
      (H, L) = divmod(xmin, S)

      if (ymin < 0 and ymax - ymin < maxV) or (ymin >= 0 and ymax < maxV):
        print(f'{a=} {s=} {c=} {e=}\nmax={ymax} (x={xmax})\nmin={ymin} (y={xmin})\n{xmax0=} {xmin0=} {dS=}')
        candi.append((a, s, c))
#        return (a, s, c)
        break
  if not candi:
    raise Exception('no candidate found', d, M)
  # min (a, s, c) according to c
  asc = min(candi, key=lambda x: x[2]) if candi else None
  return asc

L=0xac45a4010001a40200000000ffffffff
#L=0x452217cc900000010a11800000000000
p=L*L+L+1

def mod(x, d, asc):
  (a, s, c) = asc
  q = ((x >> s) * c) >> (a-s)
  r = x - q * d
  if r < 0:
    r += d
  if r >= d:
    r -= d
  return r

def checkMod(x, d, asc):
  (a, s, c) = asc
  r1 = x % d
  r2 = mod(x, d, asc)
  if r1 != r2:
    raise Exception(f'ERR {x=} {d=} {r1=} {r2=}')

def testMod(d, M=2**32-1, slist=[]):
  asc = findMod(d, M, slist)
  (a, s, c) = asc
  print(f'd={hex(d)} len(d)={d.bit_length()} M={hex(M)}')
  e = d * c - (1 << a)
  print(f'{a=} {s=} c={hex(c)} {e=} {e<c=}')
  for i in range(100):
    x = random.randint(0, M)
    checkMod(x, d, asc)
  print('OK')

def findMod2(d, M):
  (q0, r0) = divmod(M, d)
  M_d = r0 if r0 == d-1 else M - r0 - 1
  Mbit = M.bit_length()
  for a in range(128):
    A = 1 << a
    c = (A + d - 1) // d
    e = c * d - A
    if e * M_d >= A:
      continue
    for a2 in range(128):
      A2 = 1 << a2
      c2 = (A2 + d - 1) // d
      e2 = c2 * d - A2
      if e2 * M_d < (d+1) * A2 and e2 * M < (2 * d - r0) * A2:
        if c2.bit_length() >= Mbit:
          continue
        v = ((M*c2)>>a2)*d
        if v > M:
          continue
        print(f'found {d=} {a=} {hex(c)=} {a2=} {hex(c2)=}')
        return (a, c, a2, c2)

def mod2(x, d, a, c):
  q = (x * c) >> a
  return x - q * d

def mod3(x, d, a2, c2):
  q = (x * c2) >> a2
  x -= q * d
  if x < 0:
    x += d
  return x

def smod(a):
  a -= 32768
  t = a >> 13
  u = a & (2**13-1)
  u = u - t
  t = t * 2**9
  r = u + t
  return r

def test2(d, M, a, c, a2, c2):
  for x in range(M+1):
    r = x % d
    r2 = mod2(x, d, a, c)
    r3 = mod3(x, d, a2, c2)
    if r != r2:
      raise Exception(f'ERR2 {x=} {r=} {r2=}')
    if r != r3:
      print(f'ERR3 {x=} {r=} {r3=}')
#      raise Exception(f'ERR3 {x=} {r=} {r3=}')
#    if d == 8380417:
#      r4 = mod8380417(x)
#      if r != r4:
#        raise Exception(f'ERR5 {x=} {r=} {r4=}')
    if d == 3329:
      r4 = mod3329(x)
      if r != r4:
        raise Exception(f'ERR4 {x=} {r=} {r4=}')
    if d == 7681:
      r4 = smod(x)
      if r-r4 not in [2044, -5637]:
        print(f'{x=} {r=} {r4=} {(r-r4)=}')
  print('ok')

def testall(d, M):
  (a, c, a2, c2) = findMod2(d, M)
  test2(d, M, a, c, a2, c2)

def mod3329(x):
  assert(0 <= x <= 65535)
  q = 3329
  t = (x * 5) >> 14
  t = x - t * q
  if t < 0:
    t += q
  return t

def mod8380417(x):
  assert(0 <= x <= 2**24)
  q = 8380417
  c = 5
  a = 25
  t = (x * c) >> a
  t = x - t * q
  if t < 0:
    t += q
  return t


d = 2**13-2**9+1
M=65535
testall(d, M)
d = 3329
testall(d, M)
testall(8380417, 2**24)
#testall(8380417, 0xffffffff)

#testMod(12345)
#testMod(123)
#testMod(2**30-1)
