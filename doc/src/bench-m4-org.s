.lp:
    eor w13, w8, w0
    # begin x/7 sequence
    umull   x14, w13, w9
    lsr x14, x14, #32
    sub w13, w13, w14
    add w13, w14, w13, lsr #1
    # end x/7 sequence
     eor w13, w0, w13, lsr #2
    eor w14, w13, w8
    # begin x/19 sequence
    umull   x15, w14, w10
    lsr x15, x15, #32
    sub w14, w14, w15
    add w14, w15, w14, lsr #1
    # end x/19 sequence
    eor w13, w13, w14, lsr #4
    eor w14, w13, w8
    add w8, w8, #1
    # begin x/107 sequence
    umull   x15, w14, w11
    cmp w8, w12
    lsr x15, x15, #32
    sub w14, w14, w15
    add w14, w15, w14, lsr #1
    # end x/107 sequence
    eor w0, w13, w14, lsr #6
    b.ne    .lp