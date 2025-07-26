/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>
#include <tilck/common/string_util.h>

#ifdef __FPU_MEMCPY_C__
   #define EXTERN extern
#else
   #define EXTERN
#endif


/* AArch64 FPU memory copy functions */
void init_fpu_memcpy(void);
void fpu_memcpy(void *dest, const void *src, size_t n);
void fpu_memset(void *dest, int c, size_t n);

/* FPU context management */
void fpu_context_begin(void);
void fpu_context_end(void);
void save_current_fpu_regs(bool in_kernel);
void restore_fpu_regs(void *task, bool in_kernel);
void restore_current_fpu_regs(bool in_kernel);

/* FPU allocation */
bool allocate_fpu_regs(arch_task_members_t *arch_fields);

/* AArch64 specific FPU memory copy functions */
void memcpy256_failsafe(void *dest, const void *src, u32 n);
FASTCALL void memcpy_single_256_failsafe(void *dest, const void *src);

/* Non-temporal hint for the destination */
/* 'n' is the number of 32-byte (256-bit) data packets to copy */
EXTERN inline void fpu_memcpy256_nt(void *dest, const void *src, u32 n)
{
   memcpy256_failsafe(dest, src, n);
}

/* 'n' is the number of 32-byte (256-bit) data packets to copy */
EXTERN inline void fpu_memcpy256(void *dest, const void *src, u32 n)
{
   memcpy256_failsafe(dest, src, n);
}

/* Non-temporal hint for the source */
/* 'n' is the number of 32-byte (256-bit) data packets to copy */
EXTERN inline void fpu_memcpy256_nt_read(void *dest, const void *src, u32 n)
{
   memcpy256_failsafe(dest, src, n);
}

EXTERN inline void fpu_memset256(void *dest, u32 val32, u32 n)
{
   memset(dest, (int)val32, n << 5);
}

EXTERN ALWAYS_INLINE FASTCALL void
fpu_cpy_single_256_nt(void *dest, const void *src)
{
   memcpy_single_256_failsafe(dest, src);
}

EXTERN ALWAYS_INLINE FASTCALL void
fpu_cpy_single_256_nt_read(void *dest, const void *src)
{
   memcpy_single_256_failsafe(dest, src);
}
