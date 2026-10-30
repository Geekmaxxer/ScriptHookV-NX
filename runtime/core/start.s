.section .text.start,"ax",%progbits
.global __module_start
.hidden __module_start
__module_start:
    ret
    .word __mod0 - __module_start
    .word 0
    .word 0

.section .rodata.mod0,"a",%progbits
.align 2
__mod0:
    .ascii "MOD0"
    .word __dynamic_start - __mod0
    .word __bss_start - __mod0
    .word __bss_end - __mod0
    .word __eh_start - __mod0
    .word __eh_end - __mod0
    .word __module_runtime - __mod0

.section .bss.runtime,"aw",%nobits
.align 3
.hidden __module_runtime
__module_runtime:
    .space 0x100

// Horizon OS Syscall Wrappers
.section .text.sh_query_memory,"ax",%progbits
.global sh_query_memory
.hidden sh_query_memory
sh_query_memory:
    str x1, [sp, #-16]!
    svc 0x6
    ldr x2, [sp], #16
    str w1, [x2]
    ret

.section .text.sh_debug_print,"ax",%progbits
.global sh_debug_print
.hidden sh_debug_print
sh_debug_print:
    svc 0x27
    ret

.section .text.sh_get_tick,"ax",%progbits
.global sh_get_tick
.hidden sh_get_tick
sh_get_tick:
    svc 0x1e
    ret

.section .text.sh_get_thread_pointer,"ax",%progbits
.global sh_get_thread_pointer
.hidden sh_get_thread_pointer
sh_get_thread_pointer:
    mrs x0, tpidr_el0
    ret

.section .text.sh_sleep_thread,"ax",%progbits
.global sh_sleep_thread
.hidden sh_sleep_thread
sh_sleep_thread:
    svc 0x0b
    ret

// AAPCS64 cooperative call boundary. x18 and TPIDR_EL0 belong to the OS
// thread and are deliberately untouched. Offsets match FiberContext asserts.
.section .text.sh_fiber_switch,"ax",%progbits
.align 2
.global sh_fiber_switch
.hidden sh_fiber_switch
sh_fiber_switch:
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x2, sp
    str x2, [x0, #96]
    stp d8, d9, [x0, #104]
    stp d10, d11, [x0, #120]
    stp d12, d13, [x0, #136]
    stp d14, d15, [x0, #152]
    mrs x2, fpcr
    mrs x3, fpsr
    stp x2, x3, [x0, #168]
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldp x29, x30, [x1, #80]
    ldr x2, [x1, #96]
    mov sp, x2
    ldp d8, d9, [x1, #104]
    ldp d10, d11, [x1, #120]
    ldp d12, d13, [x1, #136]
    ldp d14, d15, [x1, #152]
    ldp x2, x3, [x1, #168]
    msr fpcr, x2
    msr fpsr, x3
    ret
