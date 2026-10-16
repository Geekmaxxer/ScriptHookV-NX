.section .text.start,"ax",%progbits
.global __module_start
.hidden __module_start
__module_start:
    b __module_start
    .word __mod0 - __module_start
    .space 0x78
.section .rodata.mod0,"a",%progbits
.align 2
__mod0:
    .ascii "MOD0"
    .word __dynamic_start - __mod0
    .word __bss_start - __mod0
    .word __bss_end - __mod0
    .word 0
    .word 0
    .word __module_runtime - __mod0
.section .bss.runtime,"aw",%nobits
.align 3
.hidden __module_runtime
__module_runtime:
    .space 0x100
