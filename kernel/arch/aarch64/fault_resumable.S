# SPDX-License-Identifier: BSD-2-Clause

#define ASM_FILE 1

#include <tilck_gen_headers/config_global.h>
#include <tilck/kernel/arch/aarch64/asm_defs.h>

.section .text

.global fault_resumable_call

FUNC(fault_resumable_call):

   /* Save frame pointer and link register */
   stp x29, x30, [sp, #-16]!
   mov x29, sp

   /* Allocate stack space for local variables */
   sub sp, sp, #(4 * 8)

   /* Load __disable_preempt address and store it */
   adrp x2, __disable_preempt
   add x2, x2, :lo12:__disable_preempt
   ldr w1, [x2]
   str w1, [sp, #0]

   /* Load current task and save fault_resume_regs */
   adrp x0, __current
   add x0, x0, :lo12:__current
   ldr x0, [x0]
   ldr x1, [x0, #TI_F_RESUME_RS_OFF]
   str x1, [sp, #16]

   /* Load and save faults_resume_mask */
   ldr w1, [x0, #TI_FAULTS_MASK_OFF]
   str w1, [sp, #8]

   /* Save callee-saved registers */
   stp x19, x20, [sp, #-16]!
   stp x21, x22, [sp, #-16]!
   stp x23, x24, [sp, #-16]!
   stp x25, x26, [sp, #-16]!
   stp x27, x28, [sp, #-16]!

   /* Save resume PC */
   adrp x2, .asm_fault_resumable_call_resume
   add x2, x2, :lo12:.asm_fault_resumable_call_resume
   str x2, [sp, #-8]!

   /* Save current stack pointer to task */
   adrp x0, __current
   add x0, x0, :lo12:__current
   ldr x0, [x0]
   mov x1, sp
   str x1, [x0, #TI_F_RESUME_RS_OFF]

   /* Set up arguments */
   str w0, [x0, #TI_FAULTS_MASK_OFF]  /* faults_mask */
   mov x19, x1                        /* func */
   mov x20, x2                        /* nargs */

   /* Set up first 8 parameters in x0-x7 */
   mov x0, x3
   mov x1, x4
   mov x2, x5
   mov x3, x6
   mov x4, x7
   mov x5, x8
   mov x6, x9
   mov x7, x10

   /* Handle additional parameters from stack */
   sub x20, x20, #8
   mov x21, x30                       /* backup link register */
   mov x22, sp                        /* backup sp */

   cmp x20, #0
   ble 2f

   /* Load additional parameters from stack */
   ldr x8, [sp, #0]
   sub x20, x20, #1
   cmp x20, #0
   ble 2f

   ldr x9, [sp, #8]
   sub x20, x20, #1
   cmp x20, #0
   ble 2f

   ldr x10, [sp, #16]
   sub x20, x20, #1
   cmp x20, #0
   ble 2f

   ldr x11, [sp, #24]
   sub x20, x20, #1
   cmp x20, #0
   ble 2f

   ldr x12, [sp, #32]
   sub x20, x20, #1
   cmp x20, #0
   ble 2f

   ldr x13, [sp, #40]
   sub x20, x20, #1
   cmp x20, #0
   ble 2f

   ldr x14, [sp, #48]
   sub x20, x20, #1
   cmp x20, #0
   ble 2f

   ldr x15, [sp, #56]

   /* Allocate stack space for remaining parameters */
   cmp x20, #0
   ble 2f
   lsl x23, x20, #3                   /* x20 * 8 */
   sub sp, sp, x23
   mov x24, sp
   bic x24, x24, #15                  /* Align to 16 bytes */
   mov sp, x24

1:
   sub x20, x20, #1
   add x25, sp, #64                   /* Skip saved registers */
   add x25, x25, x20, lsl #3
   ldr x26, [x25]
   str x26, [x24]
   add x24, x24, #8
   cmp x20, #0
   bgt 1b

2:
   /* Call the function */
   blr x19
   mov x0, #0                         /* Return 0 (no faults) */
   mov sp, x22

   /* Restore callee-saved registers */
   ldp x27, x28, [sp], #16
   ldp x25, x26, [sp], #16
   ldp x23, x24, [sp], #16
   ldp x21, x22, [sp], #16
   ldp x19, x20, [sp], #16

   /* Restore task state */
   adrp x0, __current
   add x0, x0, :lo12:__current
   ldr x0, [x0]
   ldr w1, [sp, #8]
   str w1, [x0, #TI_FAULTS_MASK_OFF]

   ldr x1, [sp, #16]
   str x1, [x0, #TI_F_RESUME_RS_OFF]

   /* Restore __disable_preempt */
   adrp x0, __disable_preempt
   add x0, x0, :lo12:__disable_preempt
   ldr w1, [sp, #0]
   str w1, [x0]

   add sp, sp, #(4 * 8)

   /* Restore frame pointer and return */
   ldp x29, x30, [sp], #16
   ret

.asm_fault_resumable_call_resume:

   /* Load fault code from stack */
   ldr x0, [sp, #80]                  /* Load fault code set in handle_resumable_fault() */

   /* Restore callee-saved registers */
   ldp x27, x28, [sp], #16
   ldp x25, x26, [sp], #16
   ldp x23, x24, [sp], #16
   ldp x21, x22, [sp], #16
   ldp x19, x20, [sp], #16

   /* Restore task state */
   adrp x1, __current
   add x1, x1, :lo12:__current
   ldr x1, [x1]
   ldr w2, [sp, #8]
   str w2, [x1, #TI_FAULTS_MASK_OFF]

   ldr x2, [sp, #16]
   str x2, [x1, #TI_F_RESUME_RS_OFF]

   /* Restore __disable_preempt */
   adrp x1, __disable_preempt
   add x1, x1, :lo12:__disable_preempt
   ldr w2, [sp, #0]
   str w2, [x1]

   add sp, sp, #(4 * 8)

   /* Restore frame pointer and return */
   ldp x29, x30, [sp], #16
   ret

END_FUNC(fault_resumable_call)
