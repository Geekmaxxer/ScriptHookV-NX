OUTPUT_FORMAT(elf64-littleaarch64)
OUTPUT_ARCH(aarch64)
ENTRY(__module_start)
PHDRS {text PT_LOAD FLAGS(5);ro PT_LOAD FLAGS(4);data PT_LOAD FLAGS(6);dynamic PT_DYNAMIC FLAGS(6);}
SECTIONS {
 . = 0;
 .text : {KEEP(*(.text.start)) *(.text .text.*)} :text
 .plt : {*(.plt .plt.*)} :text
 . = ALIGN(0x1000);
 .rodata : {*(.rodata.mod0) *(.rodata .rodata.*)} :ro
 .hash : {*(.hash)} :ro
 .gnu.hash : {*(.gnu.hash)} :ro
 .dynsym : {*(.dynsym)} :ro
 .dynstr : {*(.dynstr)} :ro
 .rela.dyn : {*(.rela.dyn)} :ro
 .rela.plt : {*(.rela.plt)} :ro
 . = ALIGN(0x1000);
 .data : {*(.data .data.*) *(.data.rel.ro .data.rel.ro.*)} :data
 .got : {*(.got .got.*)} :data
 .dynamic : {HIDDEN(__dynamic_start = .);*(.dynamic)} :data :dynamic
 . = ALIGN(0x1000);
 .bss (NOLOAD) : {HIDDEN(__bss_start = .);*(.bss .bss.*) *(COMMON) . = ALIGN(0x1000);HIDDEN(__bss_end = .);} :data
 /DISCARD/ : {*(.interp) *(.comment) *(.eh_frame*)}
}
