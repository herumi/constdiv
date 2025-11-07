import random

def gcm(d, S):
  while S != 0:
    (d, S) = (S, d % S)
  return d

def findMod(d, M=2**32-1):
#  if d & 1 == 0:
#    raise Exception('d must be odd', d)
  len_d = d.bit_length()
  len_M = M.bit_length()
  (q_M, r_M) = divmod(M, d)
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
      # compute gcm(d, S)
      g = gcm(d, S)
      d_red = d // g
      if d_red == 1:
        break
      S_red = S // g
      u_red = pow(d_red, -1, S_red)
      v_red = pow(S_red, -1, d_red)
      assert((u_red * d_red + v_red * S_red) % (dS//g**2) == 1)
      u = u_red * g
      v = v_red * g
      # max(r - L) is r = d-g, L = 0
      # min(r - L) is r = 0, L = S-g
      xbp = (-v * S) % dS
      xbm = (-u * d) % dS 
      xp = xbp + (M//dS)*dS
      if xp >= M:
        xp -= dS
      xm = xbm
      def get_y(x):
        (q, r) = divmod(x, d)
        (H, L) = divmod(x, S)
        return (q * e + (r - L) * c, r, L)
      (yp, r, L) = get_y(xp)
      assert(r == d-g and L == 0)
      (ym, r, L) = get_y(xm)
      assert(r == 0 and L == S-g)

      (q, r) = divmod(xm, d)
      (H, L) = divmod(xm, S)

      if (ym < 0 and yp - ym < maxV) or (ym >= 0 and yp < maxV):
        print(f'{a=} {s=} {c=} {e=} {xp=} {xm=} {xbp=} {xbm=}')
        candi.append((a, s, c))
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

def testMod(d, M=2**32-1):
  asc = findMod(d, M)
  (a, s, c) = asc
  print(f'd={hex(d)} len(d)={d.bit_length()} M={hex(M)}')
  e = d * c - (1 << a)
  print(f'{a=} {s=} c={hex(c)} {e=} {e<c=}')
  for i in range(100):
    x = random.randint(0, M)
    checkMod(x, d, asc)
  print('OK')

testMod(123)
testMod(2**30-1)
#testMod(7, 2**32-1)
#testMod(L, p-1)
