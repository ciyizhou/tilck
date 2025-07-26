# SPDX-License-Identifier: BSD-2-Clause

#define ASM_FILE 1
#include <tilck_gen_headers/config_global.h>
#include <tilck/kernel/arch/aarch64/asm_defs.h>

.section .text
.global asm_trap_entry
.global asm_trap_entry_resume
.global context_switch

.align 3
FUNC(asm_trap_entry):

   /* Save all registers */
   save_all_regs

   /* Save kernel resume PC */
   adr x9, asm_trap_entry_resume
   str x9, [sp, #(37 * AARCH64_SZPTR)]

   /* Save exception number */
   mrs x9, esr_el1
   lsr x9, x9, #26
   and x9, x9, #0x3F
   str x9, [sp, #(36 * AARCH64_SZPTR)]

   /* Check if this is an IRQ (exception class 0x1) */
   cmp x9, #0x1
   beq .handle_irq

   /* Check if this is a syscall (exception class 0x5) */
   cmp x9, #0x5
   beq .handle_syscall

   /* Handle other exceptions */
   mov x0, sp
   bl fault_entry
   b .resume

.handle_syscall:
   mov x0, sp
   bl syscall_entry
   b .resume

.handle_irq:
   /* Set int_num for IRQ */
   mov x9, #32
   str x9, [sp, #(36 * AARCH64_SZPTR)]
   mov x0, sp
   bl irq_entry

.resume:
asm_trap_entry_resume:
   /* Restore all registers */
   resume_all_regs
   eret

END_FUNC(asm_trap_entry)

FUNC(context_switch):
   mov sp, x0  /* Make SP = function's 1st argument: regs *context */
   ldr x9, [sp, #(37 * AARCH64_SZPTR)]
   ret         /* Now x9's value is `kernel_resume_pc` */
               /* By default, that value is `asm_trap_entry_resume` or */
               /* 'kernel_yield_resume' */

END_FUNC(context_switch)
