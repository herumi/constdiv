.lp:
    eor w13, w8, w0
    umulh   x13, x13, x9
    eor w13, w13, w0
    eor w14, w13, w8
    umulh   x14, x14, x10
    eor w13, w14, w13
    eor w14, w13, w8
    add w8, w8, #1
    umulh   x14, x14, x11
    cmp w8, w12
    eor w0, w14, w13
    b.ne    .lp