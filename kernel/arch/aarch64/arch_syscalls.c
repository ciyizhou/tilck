/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck_gen_headers/mod_tracing.h>

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>

#include <tilck/kernel/syscalls.h>
#include <tilck/kernel/irq.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/fs/devfs.h>
#include <tilck/kernel/fs/vfs.h>
#include <tilck/kernel/timer.h>
#include <tilck/kernel/debug_utils.h>
#include <tilck/kernel/fault_resumable.h>
#include <tilck/kernel/user.h>
#include <tilck/kernel/elf_utils.h>
#include <tilck/kernel/signal.h>
#include <tilck/mods/tracing.h>
#include <tilck/kernel/process.h>

typedef long (*syscall_type)(
   ulong, ulong, ulong, ulong, ulong, ulong // args
);

typedef long (*syscall_raw_regs)(
   void *, // regs *
   ulong, ulong, ulong, ulong, ulong, ulong
);

#define SYSFL_NO_TRACE                      0b00000001
#define SYSFL_NO_SIG                        0b00000010
#define SYSFL_NO_PREEMPT                    0b00000100
#define SYSFL_RAW_REGS                      0b00001000

struct syscall {

   union {
      void *func;
      syscall_type fptr;
   };

   u32 flags;
};

static void unknown_syscall_int(regs_t *r, u32 sn)
{
   trace_printk(5, "Unknown syscall %i", (int)sn);
   r->x0 = (ulong) -ENOSYS;
}

static void __unknown_syscall(void)
{
   struct task *curr = get_curr_task();
   regs_t *r = curr->state_regs;
   const u32 sn = r->x8;
   unknown_syscall_int(r, sn);
}

static long sys_aarch64_flush_icache(ulong start, ulong end, ulong flags)
{
   /* Check the reserved flags. */
   if (UNLIKELY(flags & ~(1UL)))
      return -EINVAL;

   /* TODO: implement aarch64_flush_icache() */
   return 0;
}

#define DECL_SYS(func, flags) { {func}, flags }
#define DECL_UNKNOWN_SYSCALL  DECL_SYS(__unknown_syscall, 0)

static struct syscall syscalls[MAX_SYSCALLS] =
{
   [0] = DECL_SYS(sys_io_setup, 0),
   [1] = DECL_SYS(sys_io_destroy, 0),
   [2] = DECL_SYS(sys_io_submit, 0),
   [3] = DECL_SYS(sys_io_cancel, 0),
   [4] = DECL_SYS(sys_io_getevents, 0),
   [5] = DECL_SYS(sys_setxattr, 0),
   [6] = DECL_SYS(sys_lsetxattr, 0),
   [7] = DECL_SYS(sys_fsetxattr, 0),
   [8] = DECL_SYS(sys_getxattr, 0),
   [9] = DECL_SYS(sys_lgetxattr, 0),
   [10] = DECL_SYS(sys_fgetxattr, 0),
   [11] = DECL_SYS(sys_listxattr, 0),
   [12] = DECL_SYS(sys_llistxattr, 0),
   [13] = DECL_SYS(sys_flistxattr, 0),
   [14] = DECL_SYS(sys_removexattr, 0),
   [15] = DECL_SYS(sys_lremovexattr, 0),
   [16] = DECL_SYS(sys_fremovexattr, 0),
   [17] = DECL_SYS(sys_getcwd, 0),
   [18] = DECL_SYS(sys_lookup_dcookie, 0),
   [19] = DECL_SYS(sys_eventfd2, 0),
   [20] = DECL_SYS(sys_epoll_create1, 0),
   [21] = DECL_SYS(sys_epoll_ctl, 0),
   [22] = DECL_SYS(sys_epoll_pwait, 0),
   [23] = DECL_SYS(sys_dup, 0),
   [24] = DECL_SYS(sys_dup3, 0),
   [25] = DECL_SYS(sys_fcntl64, 0),
   /* Add more syscalls as needed */
};

void *get_syscall_func_ptr(u32 n)
{
   if (n >= MAX_SYSCALLS)
      return NULL;
   return syscalls[n].func;
}

int get_syscall_num(void *func)
{
   for (int i = 0; i < MAX_SYSCALLS; i++) {
      if (syscalls[i].func == func)
         return i;
   }
   return -1;
}

static NO_INLINE void
do_syscall_int(syscall_type fptr, regs_t *r, bool raw_regs)
{
   long ret = fptr(r->x0, r->x1, r->x2, r->x3, r->x4, r->x5);
   r->x0 = ret;
}

static void do_special_syscall(regs_t *r)
{
   struct task *curr = get_curr_task();
   const u32 sn = r->x8;
   const u32 fl = syscalls[sn].flags;
   const syscall_type fptr = syscalls[sn].fptr;
   const bool signals = ~fl & SYSFL_NO_SIG;
   const bool preemptable = ~fl & SYSFL_NO_PREEMPT;
   const bool traceable = ~fl & SYSFL_NO_TRACE;
   const bool raw_regs = fl & SYSFL_RAW_REGS;

   if (signals)
      process_signals(curr, sig_pre_syscall, r);

   if (preemptable)
      enable_preemption();

   if (traceable)
      trace_sys_enter(sn,r->x0,r->x1,r->x2,r->x3,r->x4,r->x5);

   do_syscall_int(fptr, r, raw_regs);

   if (traceable)
      trace_sys_exit(sn,r->x0,r->x1,r->x2,r->x3,r->x4,r->x5, r->x8);

   if (preemptable)
      disable_preemption();

   if (signals)
      process_signals(curr, sig_in_syscall, r);
}

static void do_syscall(regs_t *r)
{
   struct task *curr = get_curr_task();
   const u32 sn = r->x8;
   const syscall_type fptr = syscalls[sn].fptr;

   process_signals(curr, sig_pre_syscall, r);
   enable_preemption();
   {
      trace_sys_enter(sn,r->x0,r->x1,r->x2,r->x3,r->x4,r->x5);
      do_syscall_int(fptr, r, false);
      trace_sys_exit(sn,r->x0,r->x1,r->x2,r->x3,r->x4,r->x5, r->x8);
   }
   disable_preemption();
   process_signals(curr, sig_in_syscall, r);
}

void handle_syscall(regs_t *r)
{
   const u32 sn = r->x8;

   /*
    * Advance ELR_EL1 0x4 to avoid executing the original
    * svc instruction after eret.
    */
   r->elr_el1 += 0x4;

   save_current_task_state(r, false);
   set_current_task_in_kernel();

   if (LIKELY(sn < ARRAY_SIZE(syscalls))) {

      if (LIKELY(syscalls[sn].flags == 0))
         do_syscall(r);
      else
         do_special_syscall(r);

   } else {

      unknown_syscall_int(r, sn);
   }

   set_current_task_in_user_mode();
}

void init_syscall_interfaces(void)
{
   /* TODO: initialize aarch64 syscall interfaces */
}
