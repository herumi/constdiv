import sys
sys.path.append('../ext/s_xbyak')
from s_xbyak import *

d = 12345

N = 3

def gen_moddorg():
  c = 0x53c1df1d
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

def gen_moddnew():
  c = 0xa9e1
  a = 29
  def raw(x, x32):
    imul(rdx, x, c)
    shr(rdx, a)
    imul(rdx, rdx, d)
    mov(eax, x32)
    sub(rax, rdx)
    lea(ecx, ptr(eax+d))
    cmovc(eax, ecx)

  align(16)
  with FuncProc('moddnew'):
    with StackFrame(1, 0, useRDX=True) as sf:
      x = sf.p[0]
      for i in range(N):
        raw(x, x.changeBit(32))



def main():
  parser = getDefaultParser()
  param = parser.parse_args()

  init(param)

  segment('text')

  gen_moddorg()
  gen_moddnew()

  term()

if __name__ == '__main__':
  main()
