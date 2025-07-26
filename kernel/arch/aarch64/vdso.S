# SPDX-License-Identifier: BSD-2-Clause

#define ASM_FILE 1
#include <tilck_gen_headers/config_mm.h>
#include <tilck/kernel/arch/aarch64/asm_defs.h>

.text

.global vdso_begin
.global vdso_end

.balign 4096
vdso_begin:

.align 4
/* When each signal handler returns, it will jump here */
.post_sig_handler:
   mov x8, #139 /* sys_rt_sigreturn_impl() */
   svc #0       /* do the syscall */

.align 4
/* When we cannot immediately KILL a task != current, we make it call sys_pause()
 * and it will be killed after the task switch, in handle_syscall().
 */
.pause_trampoline:
   mov x0, #0
   mov x1, #0
   mov x2, #0
   mov x3, #0
   mov x4, #0
   mov x8, #73  /* sys_ppoll() */
   svc #0

.space 4096-(.-vdso_begin), 0
vdso_end:

.global post_sig_handler_user_vaddr
post_sig_handler_user_vaddr:
   AARCH64_PTR USER_VDSO_VADDR + (.post_sig_handler - vdso_begin)

.global pause_trampoline_user_vaddr
pause_trampoline_user_vaddr:
   AARCH64_PTR USER_VDSO_VADDR + (.pause_trampoline - vdso_begin)
