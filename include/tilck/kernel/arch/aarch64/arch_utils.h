/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

#if defined(__TILCK_KERNEL__) && !defined(__TILCK_HAL__)
   #error Never include this header directly. Do #include <tilck/kernel/hal.h>.
#endif

#include <tilck_gen_headers/config_kernel.h>
#include <tilck/kernel/arch/aarch64/asm_defs.h>

struct aarch64_regs {
   /* General purpose registers */
   u64 x0, x1, x2, x3, x4, x5, x6, x7;
   u64 x8, x9, x10, x11, x12, x13, x14, x15;
   u64 x16, x17, x18, x19, x20, x21, x22, x23;
   u64 x24, x25, x26, x27, x28, x29, x30; /* x30 is LR */

   /* System registers */
   u64 sp_el0;      /* User stack pointer */
   u64 sp_el1;      /* Kernel stack pointer */
   u64 elr_el1;     /* Exception link register */
   u64 spsr_el1;    /* Saved program status register */
   u64 esr_el1;     /* Exception syndrome register */
   u64 far_el1;     /* Fault address register */

   /* Additional context */
   u64 int_num;
   u64 kernel_resume_pc;
   u64 usersp;
};

STATIC_ASSERT(SIZEOF_REGS == sizeof(regs_t));

struct aarch64_arch_proc_members {
   void *none;
};

struct aarch64_arch_task_members {
   u64 fpu_regs_size;
   void *fpu_regs;
};

NORETURN void context_switch(regs_t *r);

static ALWAYS_INLINE int regs_intnum(regs_t *r)
{
   return (int)(r->esr_el1 >> 26); /* Extract exception class from ESR */
}

static ALWAYS_INLINE void set_return_register(regs_t *r, ulong value)
{
   r->x0 = value;
}

static ALWAYS_INLINE ulong get_return_register(regs_t *r)
{
   return r->x0;
}

static ALWAYS_INLINE void set_return_addr(regs_t *r, ulong value)
{
   r->x30 = value; /* LR register */
}

static ALWAYS_INLINE ulong get_return_addr(regs_t *r)
{
   return r->x30; /* LR register */
}

static ALWAYS_INLINE ulong get_rem_stack(void)
{
   return (get_stack_ptr() & ((ulong)KERNEL_STACK_SIZE - 1));
}

static ALWAYS_INLINE void *regs_get_stack_ptr(regs_t *r)
{
   return TO_PTR(r->sp_el1);
}

static ALWAYS_INLINE void *regs_get_frame_ptr(regs_t *r)
{
   return TO_PTR(r->x29); /* x29 is frame pointer */
}

static ALWAYS_INLINE void *regs_get_ip(regs_t *r)
{
   return TO_PTR(r->elr_el1);
}

static ALWAYS_INLINE void regs_set_ip(regs_t *r, ulong value)
{
   r->elr_el1 = value;
}

static ALWAYS_INLINE ulong regs_get_usersp(regs_t *r)
{
   return r->usersp;
}

static ALWAYS_INLINE void regs_set_usersp(regs_t *r, ulong value)
{
   r->usersp = value;
}

static ALWAYS_INLINE ulong regs_get_sp(regs_t *r)
{
   return r->sp_el1;
}

static ALWAYS_INLINE void regs_set_sp(regs_t *r, ulong value)
{
   r->sp_el1 = value;
}

static ALWAYS_INLINE void *fdt_get_address(void)
{
   extern void * fdt_blob;
   return fdt_blob;
}

