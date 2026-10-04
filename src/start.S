.syntax unified
.cpu arm7tdmi
.arm
.section .header,"ax"
.global _rom
_rom:
 b _start
 .space 188
.section .text.start,"ax"
.global _start
_start:
 mov r0, #0x12
 msr cpsr_c, r0
 ldr sp, =0x03007fa0
 mov r0, #0x1f
 msr cpsr_c, r0
 ldr sp, =0x03007f00
 ldr r0, =__bss_start
 ldr r1, =__bss_end
 mov r2, #0
1:
 cmp r0, r1
 strlo r2, [r0], #4
 blo 1b
 ldr r0, =__iwram_load
 ldr r1, =__iwram_start
 ldr r2, =__iwram_end
3:
 cmp r1, r2
 ldrlo r3, [r0], #4
 strlo r3, [r1], #4
 blo 3b
 ldr r0, =main
 mov lr, pc
 bx r0
2: b 2b
/* Fast restoring division in IWRAM; no BIOS call per projected vertex. */
.section .text.div,"ax"
.global __aeabi_idiv
.global __aeabi_idivmod
.global __aeabi_uidiv
.global __aeabi_uidivmod
__aeabi_idiv:
__aeabi_idivmod:
 push {r4, lr}
 mov r4, r0
 eor r12, r0, r1
 cmp r0, #0
 rsblt r0, r0, #0
 cmp r1, #0
 rsblt r1, r1, #0
 bl __aeabi_uidiv
 cmp r12, #0
 rsblt r0, r0, #0
 cmp r4, #0
 rsblt r1, r1, #0
 pop {r4, lr}
 bx lr
__aeabi_uidiv:
__aeabi_uidivmod:
 cmp r1, #0
 beq 7f
 mov r2, r1
 mov r3, #1
 mov r1, #0
4:
 cmp r2, r0
 bhi 5f
 tst r2, #0x80000000
 bne 5f
 mov r2, r2, lsl #1
 mov r3, r3, lsl #1
 b 4b
5:
 cmp r0, r2
 subcs r0, r0, r2
 orrcs r1, r1, r3
 mov r2, r2, lsr #1
 movs r3, r3, lsr #1
 bne 5b
 mov r2, r0
 mov r0, r1
 mov r1, r2
 bx lr
7:
 mov r1, r0
 mov r0, #0
 bx lr
/* Freestanding ARM ABI structure-copy helpers (all alignments supported). */
.section .romcode,"ax",%progbits
.global __aeabi_memcpy
.global __aeabi_memcpy4
.global __aeabi_memcpy8
__aeabi_memcpy:
__aeabi_memcpy4:
__aeabi_memcpy8:
 cmp r2,#0
 bxeq lr
1:
 ldrb r3,[r1],#1
 strb r3,[r0],#1
 subs r2,r2,#1
 bne 1b
 bx lr
