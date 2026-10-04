ENTRY(_rom)
MEMORY {
 ROM (rx) : ORIGIN = 0x08000000, LENGTH = 32M
 EWRAM (rw) : ORIGIN = 0x02000000, LENGTH = 256K
 IWRAM (rwx) : ORIGIN = 0x03000000, LENGTH = 30K
}
SECTIONS {
 .text : { KEEP(*(.header)) *(.text.start) *(.romcode*) } > ROM
 .iwram : ALIGN(4) { __iwram_start = .; *(.text*) . = ALIGN(4); __iwram_end = .; } > IWRAM AT > ROM
 __iwram_load = LOADADDR(.iwram);
 .rodata : { KEEP(*(.save_signature)) *(.rodata*) } > ROM
 .bss (NOLOAD) : ALIGN(4) { __bss_start = .; *(.bss*) *(COMMON) . = ALIGN(4); __bss_end = .; } > EWRAM
 .data : { *(.data*) } > EWRAM AT > ROM
 /DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment*) *(.note*) }
}
ASSERT(SIZEOF(.data) == 0, "Startup does not support initialized writable data")
ASSERT(__bss_end <= 0x02040000, "EWRAM overflow")
