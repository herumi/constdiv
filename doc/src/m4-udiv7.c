; input: x = w0
; output: w0
; d = 7
; a = 35
udiv:
  ; cL = w8 = 0x24924925
  mov w8, #18725  ; =0x4925
  movk    w8, #9362, lsl #16
  ; x8 = cL * x
  umull   x8, w0, w8
  ; y = x8 = (cL * x) >> 32
  lsr x8, x8, #32
  ; w9 = x - y
  sub w9, w0, w8
  ; t = ((x - y) >> 1) + y
  add w8, w8, w9, lsr #1
  ; t >> (a - 33)
  lsr w0, w8, #2
  ret