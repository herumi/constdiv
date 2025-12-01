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

testMod(123)
testMod(12345)
testMod(0xffff)
testMod(0xfffffff)
#testMod(2**30-1)
#testMod(3329,65537)
#testMod(7, 2**32-1)
#testMod(L, p-1)
