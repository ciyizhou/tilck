#define ASM_FILE 1

#include <tilck_gen_headers/config_global.h>
#include <tilck_gen_headers/config_sched.h>
#include <tilck/kernel/arch/aarch64/asm_defs.h>
#include <multiboot.h>

.section .text

.global asm_nop_loop
.global asm_do_bogomips_loop

// Loop used to perform short delays, used by delay_us().
// a0(x0): loop count
// Clobbers: x0
// Returns: nothing

asm_nop_loop:
1:
    nop
    nop
    subs x0, x0, #1
    b.ne 1b
    ret

// Function used to measure Tilck's bogoMIPS, see do_bogomips_loop().
// Outer loop increments __bogo_loops atomically every BOGOMIPS_CONST inner loops.
// Terminates when __bogo_loops becomes negative.
//
// a0(x0): scratch
// a1(x1): scratch
// Returns: nothing

asm_do_bogomips_loop:
    nop
    nop
    nop

2:
    mov x0, BOGOMIPS_CONST
3:
    nop
    nop
    subs x0, x0, #1
    b.ne 3b

    mov x0, #1
    adrp x1, __bogo_loops
    add  x1, x1, :lo12:__bogo_loops
    mov x2, #1
4:
    ldxr w0, [x1]
    add w3, w0, w2
    stxr w4, w3, [x1]
    cbnz w4, 4b
    cmp w0, #0
    b.ge 2b
    ret
