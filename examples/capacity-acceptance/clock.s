.section .text.capacity_diagnostic_tick,"ax",%progbits
.global capacity_diagnostic_tick
.hidden capacity_diagnostic_tick
.type capacity_diagnostic_tick,%function
capacity_diagnostic_tick:
    // Same Horizon GetSystemTick boundary as runtime/core/start.s.
    svc 0x1e
    ret
