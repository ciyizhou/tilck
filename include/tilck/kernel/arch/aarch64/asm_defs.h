/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck_gen_headers/config_global.h>
#include <tilck/common/arch/aarch64/asm_consts.h>

#if KERNEL_STACK_PAGES == 1
   #define ASM_KERNEL_STACK_SZ      4096
#elif KERNEL_STACK_PAGES == 2
   #define ASM_KERNEL_STACK_SZ      8192
#elif KERNEL_STACK_PAGES == 4
   #define ASM_KERNEL_STACK_SZ      16384
#else
   #error Unsupported value of KERNEL_STACK_PAGES
#endif

#define TI_F_RESUME_RS_OFF     40 /* offset of: fault_resume_regs */
#define TI_FAULTS_MASK_OFF     48 /* offset of: faults_resume_mask */

#define SIZEOF_REGS     320
#define AARCH64_SZPTR   8
#define AARCH64_LOGSZPTR 3

/* Some useful asm macros */
#ifdef ASM_FILE

#define AARCH64_PTR     .dword

#define REG_S    str
#define REG_L    ldr

#define NBITS           64

#define FUNC(x) .type x, @function; x
#define END_FUNC(x) .size x, .-(x)

.macro save_fp_ra
   sub sp, sp, #(AARCH64_SZPTR * 4)
   str lr, [sp, #(3 * AARCH64_SZPTR)]
   str x29, [sp, #(2 * AARCH64_SZPTR)]
   add x29, sp, #(AARCH64_SZPTR * 4)
.endm

.macro restore_fp_ra
   ldr lr, [sp, #(3 * AARCH64_SZPTR)]
   ldr x29, [sp, #(2 * AARCH64_SZPTR)]
   add sp, sp, #(AARCH64_SZPTR * 4)
.endm

.macro save_callee_regs

   mov x9, sp
   sub sp, sp, #SIZEOF_REGS

   str lr, [sp, #(1 * AARCH64_SZPTR)]
   str x9, [sp, #(2 * AARCH64_SZPTR)]
   str x19, [sp, #(8 * AARCH64_SZPTR)]
   str x20, [sp, #(9 * AARCH64_SZPTR)]
   str x21, [sp, #(18 * AARCH64_SZPTR)]
   str x22, [sp, #(19 * AARCH64_SZPTR)]
   str x23, [sp, #(20 * AARCH64_SZPTR)]
   str x24, [sp, #(21 * AARCH64_SZPTR)]
   str x25, [sp, #(22 * AARCH64_SZPTR)]
   str x26, [sp, #(23 * AARCH64_SZPTR)]
   str x27, [sp, #(24 * AARCH64_SZPTR)]
   str x28, [sp, #(25 * AARCH64_SZPTR)]

   mrs x9, spsr_el3
   str x9, [sp, #(33 * AARCH64_SZPTR)]

.endm

.macro resume_callee_regs

   ldr x9, [sp, #(33 * AARCH64_SZPTR)]
   msr spsr_el3, x9

   ldr lr, [sp, #(1 * AARCH64_SZPTR)]
   ldr x9, [sp, #(2 * AARCH64_SZPTR)]
   ldr x19, [sp, #(8 * AARCH64_SZPTR)]
   ldr x20, [sp, #(9 * AARCH64_SZPTR)]
   ldr x21, [sp, #(18 * AARCH64_SZPTR)]
   ldr x22, [sp, #(19 * AARCH64_SZPTR)]
   ldr x23, [sp, #(20 * AARCH64_SZPTR)]
   ldr x24, [sp, #(21 * AARCH64_SZPTR)]
   ldr x25, [sp, #(22 * AARCH64_SZPTR)]
   ldr x26, [sp, #(23 * AARCH64_SZPTR)]
   ldr x27, [sp, #(24 * AARCH64_SZPTR)]
   ldr x28, [sp, #(25 * AARCH64_SZPTR)]

   ldr sp, [sp, #(2 * AARCH64_SZPTR)]

.endm

.macro save_all_regs

   sub sp, sp, #SIZEOF_REGS

   str x1, [sp, #(1 * AARCH64_SZPTR)]

   str x3, [sp, #(3 * AARCH64_SZPTR)]
   str x4, [sp, #(4 * AARCH64_SZPTR)]
   str x5, [sp, #(5 * AARCH64_SZPTR)]
   str x6, [sp, #(6 * AARCH64_SZPTR)]
   str x7, [sp, #(7 * AARCH64_SZPTR)]
   str x8, [sp, #(8 * AARCH64_SZPTR)]
   str x9, [sp, #(9 * AARCH64_SZPTR)]
   str x10, [sp, #(10 * AARCH64_SZPTR)]
   str x11, [sp, #(11 * AARCH64_SZPTR)]
   str x12, [sp, #(12 * AARCH64_SZPTR)]
   str x13, [sp, #(13 * AARCH64_SZPTR)]
   str x14, [sp, #(14 * AARCH64_SZPTR)]
   str x15, [sp, #(15 * AARCH64_SZPTR)]
   str x16, [sp, #(16 * AARCH64_SZPTR)]
   str x17, [sp, #(17 * AARCH64_SZPTR)]
   str x18, [sp, #(18 * AARCH64_SZPTR)]
   str x19, [sp, #(19 * AARCH64_SZPTR)]
   str x20, [sp, #(20 * AARCH64_SZPTR)]
   str x21, [sp, #(21 * AARCH64_SZPTR)]
   str x22, [sp, #(22 * AARCH64_SZPTR)]
   str x23, [sp, #(23 * AARCH64_SZPTR)]
   str x24, [sp, #(24 * AARCH64_SZPTR)]
   str x25, [sp, #(25 * AARCH64_SZPTR)]
   str x26, [sp, #(26 * AARCH64_SZPTR)]
   str x27, [sp, #(27 * AARCH64_SZPTR)]
   str x28, [sp, #(28 * AARCH64_SZPTR)]
   str x29, [sp, #(29 * AARCH64_SZPTR)]
   str x30, [sp, #(30 * AARCH64_SZPTR)]

   /* Save system registers */
   mrs x9, sp_el0
   mrs x10, sp_el1
   mrs x11, elr_el1
   mrs x12, spsr_el1
   mrs x13, esr_el1
   mrs x14, far_el1

   str x9, [sp, #(31 * AARCH64_SZPTR)]   /* sp_el0 */
   str x10, [sp, #(32 * AARCH64_SZPTR)]  /* sp_el1 */
   str x11, [sp, #(33 * AARCH64_SZPTR)]  /* elr_el1 */
   str x12, [sp, #(34 * AARCH64_SZPTR)]  /* spsr_el1 */
   str x13, [sp, #(35 * AARCH64_SZPTR)]  /* esr_el1 */
   str x14, [sp, #(36 * AARCH64_SZPTR)]  /* far_el1 */

   /* Save additional context */
   /* int_num, kernel_resume_pc, usersp will be set by the caller */

.endm

.macro resume_all_regs

   /* Restore system registers */
   ldr x9, [sp, #(31 * AARCH64_SZPTR)]   /* sp_el0 */
   ldr x10, [sp, #(32 * AARCH64_SZPTR)]  /* sp_el1 */
   ldr x11, [sp, #(33 * AARCH64_SZPTR)]  /* elr_el1 */
   ldr x12, [sp, #(34 * AARCH64_SZPTR)]  /* spsr_el1 */
   ldr x13, [sp, #(35 * AARCH64_SZPTR)]  /* esr_el1 */
   ldr x14, [sp, #(36 * AARCH64_SZPTR)]  /* far_el1 */

   msr sp_el0, x9
   msr sp_el1, x10
   msr elr_el1, x11
   msr spsr_el1, x12
   msr esr_el1, x13
   msr far_el1, x14

   ldr x1, [sp, #(1 * AARCH64_SZPTR)]

   ldr x3, [sp, #(3 * AARCH64_SZPTR)]
   ldr x4, [sp, #(4 * AARCH64_SZPTR)]
   ldr x5, [sp, #(5 * AARCH64_SZPTR)]
   ldr x6, [sp, #(6 * AARCH64_SZPTR)]
   ldr x7, [sp, #(7 * AARCH64_SZPTR)]
   ldr x8, [sp, #(8 * AARCH64_SZPTR)]
   ldr x9, [sp, #(9 * AARCH64_SZPTR)]
   ldr x10, [sp, #(10 * AARCH64_SZPTR)]
   ldr x11, [sp, #(11 * AARCH64_SZPTR)]
   ldr x12, [sp, #(12 * AARCH64_SZPTR)]
   ldr x13, [sp, #(13 * AARCH64_SZPTR)]
   ldr x14, [sp, #(14 * AARCH64_SZPTR)]
   ldr x15, [sp, #(15 * AARCH64_SZPTR)]
   ldr x16, [sp, #(16 * AARCH64_SZPTR)]
   ldr x17, [sp, #(17 * AARCH64_SZPTR)]
   ldr x18, [sp, #(18 * AARCH64_SZPTR)]
   ldr x19, [sp, #(19 * AARCH64_SZPTR)]
   ldr x20, [sp, #(20 * AARCH64_SZPTR)]
   ldr x21, [sp, #(21 * AARCH64_SZPTR)]
   ldr x22, [sp, #(22 * AARCH64_SZPTR)]
   ldr x23, [sp, #(23 * AARCH64_SZPTR)]
   ldr x24, [sp, #(24 * AARCH64_SZPTR)]
   ldr x25, [sp, #(25 * AARCH64_SZPTR)]
   ldr x26, [sp, #(26 * AARCH64_SZPTR)]
   ldr x27, [sp, #(27 * AARCH64_SZPTR)]
   ldr x28, [sp, #(28 * AARCH64_SZPTR)]
   ldr x29, [sp, #(29 * AARCH64_SZPTR)]
   ldr x30, [sp, #(30 * AARCH64_SZPTR)]

   ldr x2, [sp, #(2 * AARCH64_SZPTR)]
.endm

#endif
