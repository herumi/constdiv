test:
    movl    %edi, %eax
    xorl    %ecx, %ecx
    movabsq $2635249153617166336, %rsi      # imm = 0x24924924A0000000
    movabsq $970881267157434368, %rdi       # imm = 0xD79435E58000000
    movabsq $172399477334736896, %r8        # imm = 0x2647C6946000000
    .p2align    4
.lp:
    movl    %ecx, %edx
    xorl    %eax, %edx
    mulxq   %rsi, %r9, %r9 # corresponds to x/7
    xorl    %eax, %r9d
    movl    %r9d, %edx
    xorl    %ecx, %edx
    mulxq   %rdi, %r10, %r10 # corresponds to x/19
    xorl    %r9d, %r10d
    movl    %r10d, %edx
    xorl    %ecx, %edx
    mulxq   %r8, %rax, %rax # corresponds to x/107
    xorl    %r10d, %eax
    incl    %ecx
    cmpl    $1000000000, %ecx
    jne .lp