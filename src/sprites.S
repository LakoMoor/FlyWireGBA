/* flybody-derived graphics; Apache-2.0, see THIRD_PARTY_NOTICES.md. */
#ifdef __APPLE__
.section __TEXT,__const
.balign 4
.globl _fly_sprites
_fly_sprites:
#else
.section .rodata
.balign 4
.global fly_sprites
fly_sprites:
#endif
.incbin "assets/fly.sprites"
#if !defined(__APPLE__) && !defined(__arm__)
.section .note.GNU-stack,"",@progbits
#endif
