import sys
sys.path.append('../ext/s_xbyak')
from s_xbyak import *

d = 7

"""
mov edx, edi
mov eax, edi
imul    rdx, rdx, 613566757
shr rdx, 32
sub eax, edx
shr eax
add eax, edx
shr eax, 2
lea edx, 0[0+rax*8]
sub edx, eax
mov eax, edi
sub eax, edx
"""

def gen_mod7org():
  c = 0x124924925
  a = 35
  def mod7org_raw(x32):
    mov(edx, x32)
    mov(eax, c & 0xFFFFFFFF)
    imul(rax, rdx)
    shr(eax, 32)
    sub(edx, eax)
    shr(edx, 1)
    add(eax, edx)
    shr(eax, a - 33)
    lea(edx, ptr(rax*8))
    sub(edx, eax)
    mov(eax, x32)
    sub(eax, edx)

  align(16)
  with FuncProc('mod7org'):
    with StackFrame(1, 0, useRDX=True) as sf:
      x = sf.p[0]
      x32 = x.changeBit(32)
      mod7org_raw(x32)



def main():
  parser = getDefaultParser()
  param = parser.parse_args()

  init(param)

  segment('text')

  gen_mod7org()
#  gen_mod7new()

  term()

if __name__ == '__main__':
  main()
