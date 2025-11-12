import sys
sys.path.append('../ext/s_xbyak')
from s_xbyak import *

d = 12345

N = 3

'''
msec
N       2     |    3     |   4
org  2603.503 | 4110.692 | 5931.546
new1 2105.524 | 3133.144 | 4387.774
new2 2052.601 | 3073.320 | 4322.624

diff | <3> - <2> | <4> - <3>
org  | 1507.189  | 1820.854
new1 | 1027.620  | 1254.630
new2 | 1020.719  | 1249.304
'''

def gen_moddorg():
  c = 0x153c1df1d
  a = 46
  def raw(x32):
    mov(eax, x32)
    mov(ecx, x32)
    imul(rcx, rcx, c & 0xFFFFFFFF)
    shr(rcx, 32)
    mov(edx, x32)
    sub(edx, ecx)
    shr(edx, 1)
    add(edx, ecx)
    shr(edx, a - 33)
    imul(ecx, edx, d)
    sub(eax, ecx)


  align(16)
  with FuncProc('moddorg'):
    with StackFrame(1, 0, useRDX=True) as sf:
      x = sf.p[0]
      x32 = x.changeBit(32)
      for i in range(N):
        raw(x32)
        add(x32, eax)

def gen_moddnew(mode):
  def raw(x, x32):
    if mode == 1:
      # fast div and standard mod
      c = 0x153c1df1d
      a = 46
      mov(edx, c & 0xffffffff)
      imul(rdx, x)
      shr(rdx, 32)
      add(rdx, x)
      shr(rdx, a - 32)
      imul(rdx, rdx, d)
      mov(eax, x32)
      sub(eax, edx)
    elif mode == 0:
      # slow
      c = 0xa9e1
      a = 29
      imul(rdx, x, c)
      shr(rdx, a)
      imul(rdx, rdx, d)
      mov(eax, x32)
      sub(rax, rdx)
      sbb(ecx, ecx)
      mov(edx, d)
      and_(edx, ecx)
      add(eax, edx)
    elif mode == 2:
      # fast
      c = 0xa9e1
      a = 29
      imul(rdx, x, c)
      shr(rdx, a)
      imul(rdx, rdx, d)
      mov(eax, x32)
      sub(rax, rdx)
      lea(ecx, ptr(eax+d))
      cmovc(eax, ecx)
    else:
      raise Exception('invalid mode', mode)

  align(16)
  with FuncProc(f'moddnew{mode}'):
    with StackFrame(1, 0, useRCX=True, useRDX=True) as sf:
      x = sf.p[0]
      x32 = x.changeBit(32)
      for i in range(N):
        raw(x, x32)
        add(x32, eax)



def main():
  parser = getDefaultParser()
  param = parser.parse_args()

  init(param)

  segment('text')

  gen_moddorg()
  gen_moddnew(1)
  gen_moddnew(2)

  term()

if __name__ == '__main__':
  main()
