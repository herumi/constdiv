test:
   movl    %edi, %eax
   xorl    %ecx, %ecx
   movl    $2938661835, %edx               # imm = 0xAF286BCB
   .p2align    4, 0x90
 .lp:
   movl    %ecx, %esi
   xorl    %eax, %esi
   # begin x/7 sequence
   imulq   $613566757, %rsi, %rdi          # imm = 0x24924925
   shrq    $32, %rdi
   subl    %edi, %esi
   shrl    %esi
   addl    %edi, %esi
   shrl    $2, %esi
   # end x/7 sequence
   xorl    %eax, %esi
   movl    %esi, %edi
   xorl    %ecx, %edi
   movq    %rdi, %rax
   # begin x/19 sequence
   imulq   %rdx, %rax
   shrq    $32, %rax
   subl    %eax, %edi
   shrl    %edi
   addl    %eax, %edi
   shrl    $4, %edi
   # end x/19 sequence
   xorl    %esi, %edi
   movl    %edi, %eax
   xorl    %ecx, %eax
   # begin x/107 sequence
   imulq   $842937507, %rax, %rsi          # imm = 0x323E34A3
   shrq    $32, %rsi
   subl    %esi, %eax
   shrl    %eax
   addl    %esi, %eax
   shrl    $6, %eax
   # end x/107 sequence
   xorl    %edi, %eax
   incl    %ecx
   cmpl    $1000000000, %ecx               # imm = 0x3B9ACA00
   jne .lp