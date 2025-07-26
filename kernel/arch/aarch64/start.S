# SPDX-License-Identifier: BSD-2-Clause

#define ASM_FILE 1

#include <tilck_gen_headers/config_global.h>
#include <tilck_gen_headers/config_kernel.h>
#include <tilck_gen_headers/config_boot.h>
#include <tilck/kernel/arch/aarch64/asm_defs.h>

#include <multiboot.h>

.section .bss
.global kernel_initial_stack
.comm kernel_initial_stack, ASM_KERNEL_STACK_SZ, 8192

.section .text.start
.global _start
.global linux_image
.global _boot_cpu_hartid

#define AARCH64_IMAGE_MAGIC            "ARM\x64"

linux_image:
FUNC(_start):
   /*
    * Image header expected by Linux boot-loaders. The image header data
    * structure is described in image.h.
    * Do not modify it without modifying the structure and all bootloaders
    * that expects this header format!!
    */
   /* jump to start kernel */
   b _start_kernel
   /* reserved */

   .word 0
   .balign 8

   /* Image load offset(2MB) from start of RAM */
   .quad 0x200000

   /*
    * Effective size of kernel image. Note that we marked also the ELF headers
    * as LOAD segments, and they take 4K (0x1000) bytes, before _start.
    */
   .quad _end - _start + 0x1000
   .quad 0
   .quad 0
   .quad 0
   .quad 0
   .ascii AARCH64_IMAGE_MAGIC
   .long 0

_start_kernel:
   /* Disable interrupts */
   msr daifset, #0xf

   /* Disable FPU */
   msr cptr_el3, xzr

   /* The first hart to get the lottery continues to execute */
   adrp x6, _boot_lottery
   add x6, x6, :lo12:_boot_lottery
   mov x7, #1
1:  ldaxr x8, [x6]
   add x8, x8, x7
   stlxr w9, x8, [x6]
   cbnz w9, 1b
   cbnz x8, _wait_forever

   /* clear bss */
   adrp x3, __bss_start
   add x3, x3, :lo12:__bss_start
   adrp x4, __bss_stop
   add x4, x4, :lo12:__bss_stop
   cmp x4, x3
   ble clear_bss_done
clear_bss:
   str xzr, [x3], #8
   cmp x3, x4
   blt clear_bss
clear_bss_done:

   /* Setup temporary exception handler */
   adrp x5, _early_fault
   add x5, x5, :lo12:_early_fault
   msr vbar_el3, x5

   mov x19, x0   /* Save hart ID */
   mov x20, x1   /* Save DTB physical address */
   adrp x2, _boot_cpu_hartid
   add x2, x2, :lo12:_boot_cpu_hartid
   str w0, [x2]

   adrp x1, kernel_initial_stack
   add x1, x1, :lo12:kernel_initial_stack
   add sp, x1, ASM_KERNEL_STACK_SZ

   mov x0, x20
   bl prepare_early_mapping

   bl parse_fdt
   mov x21, x0   /* Save multiboot info structure physical address */

   bl early_get_cpu_features

   mov x0, x20
   bl init_early_mapping

   adrp x0, page_size_buf
   add x0, x0, :lo12:page_size_buf
   ldr x1, =0x8000000000000000  /* TTBR0_EL3 */
   orr x0, x0, x1

   adrp x1, kernel_va_pa_offset
   add x1, x1, :lo12:kernel_va_pa_offset
   ldr x1, [x1]

   /* Enable paging */
   dsb ish
   tlbi vmalle1
   dsb ish
   isb
   msr ttbr0_el3, x0

   /* Calculate return address */
   adrp x30, .next_step
   add x30, x30, :lo12:.next_step
   add x30, x30, x1

   /* Return to .next_step using high virtual address */
   ret

.next_step:

   /* Enable access to user memory */
   mrs x0, sctlr_el3
   orr x0, x0, #(1 << 0)  /* SCTLR_EL3.M */
   msr sctlr_el3, x0

   adrp x1, kernel_initial_stack
   add x1, x1, :lo12:kernel_initial_stack
   add sp, x1, ASM_KERNEL_STACK_SZ
   mov x1, x21   /* 2st argument: multiboot information structure */
   ldr x0, =MULTIBOOT_BOOTLOADER_MAGIC   /* 1nd argument: multiboot magic */
   bl kmain

.align 2
_wait_forever:
   /* We lack SMP support, so park this hart */
   wfi
   b _wait_forever

.align 3
_early_fault:
   wfi
   b _early_fault

END_FUNC(_start)

.section .data
.align 3
_boot_lottery:
   .dword 0
_boot_cpu_hartid:
   .word 0
