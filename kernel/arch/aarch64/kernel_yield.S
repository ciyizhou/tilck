//s SPDX-License-Identifier: BSD-2-Clause

#define ASM_FILE 1

#include <tilck_gen_headers/config_global.h>
#include <tilck_gen_headers/config_kernel.h>
#include <tilck_gen_headers/config_debug.h>

#include <tilck/kernel/arch/aarch64/asm_defs.h>
#include <multiboot.h>

.section .text

.global asm_save_regs_and_schedule
.global panic_save_current_state
.global switch_to_initial_kernel_stack

/* Saves the current (kernel) state as if an interrupt occurred while running
 * in kernel mode.
 */

asm_save_regs_and_schedule:
   save_all_regs

   adrp x0, .kernel_yield_resume
   add  x0, x0, :lo12:.kernel_yield_resume
   str  x0, [sp, #(34 * AARCH64_SZPTR)]  /* save kernel_resume_pc */

   mov x0, sp
   mov x1, #0
   bl save_current_task_state

#if DEBUG_CHECKS && KRN_TRACK_NESTED_INTERR
   bl check_not_in_irq_handler
#endif

   bl do_schedule

   /* do_schedule() just returned: restore SP and just return */

   add sp, sp, #SIZEOF_REGS

   mov x0, #0      /* No context-switch return value */

   resume_all_regs
   ret

.kernel_yield_resume:
   resume_all_regs

   mov x0, #1      /* Context-switch return value */
   ret

panic_save_current_state:
   save_all_regs

   mov x0, sp
   bl panic_save_current_task_state
   add sp, sp, #SIZEOF_REGS

   resume_all_regs
   ret

switch_to_initial_kernel_stack:
   /* save the fp-sp offset */
   mov x0, x29
   mov x1, sp
   sub x0, x0, x1

   /* change the stack pointer and the frame pointer */
   adrp x29, kernel_initial_stack
   add  x29, x29, :lo12:kernel_initial_stack
   add  x29, x29, #ASM_KERNEL_STACK_SZ
   mov  sp, x29

   /* restore the fp-sp difference */
   sub sp, sp, x0
   ret
