/* SPDX-License-Identifier: BSD-2-Clause */

#define __FPU_MEMCPY_C__

/*
 * The code in this translation unit, in particular the *single* funcs defined
 * in fpu_memcpy.h have to be FAST, even in debug builds. It is important the
 * hot-patched fpu_cpy_single_256_nt, created by copying the once of the
 * fpu_cpy_*single* funcs to just execute the necessary MOVs and then just a
 * RET. No prologue/epilogue, no frame pointer, no stack variables.
 * NOTE: clearly, the code works well even if -O0 and without the PRAGMAS
 * below. Just, we want it to be fast in debug builds to improve the user
 * experience there.
 */

#if defined(__GNUC__) && !defined(__clang__)
   #pragma GCC optimize "-O3"
   #pragma GCC optimize "-fomit-frame-pointer"
#endif

#include <tilck_gen_headers/config_debug.h>
#include <tilck/common/basic_defs.h>
#include <tilck/common/string_util.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/debug_utils.h>

/* Initialize FPU memory copy */
void init_fpu_memcpy(void)
{
   /* STUB implementation */
}

/* FPU memory copy */
void fpu_memcpy(void *dest, const void *src, size_t n)
{
   /* STUB implementation - fall back to regular memcpy */
   memcpy(dest, src, n);
}

/* FPU memory set */
void fpu_memset(void *dest, int c, size_t n)
{
   /* STUB implementation - fall back to regular memset */
   memset(dest, c, n);
}

/* FPU context management */
void fpu_context_begin(void)
{
   /* STUB implementation */
}

void fpu_context_end(void)
{
   /* STUB implementation */
}

void save_current_fpu_regs(bool in_kernel)
{
   /* STUB implementation */
}

void restore_fpu_regs(void *task, bool in_kernel)
{
   /* STUB implementation */
}

void restore_current_fpu_regs(bool in_kernel)
{
   /* STUB implementation */
}

/* FPU allocation */
bool allocate_fpu_regs(arch_task_members_t *arch_fields)
{
   /* STUB implementation */
   return true;
}

/* AArch64 specific FPU memory copy functions */
void
memcpy256_failsafe(void *dest, const void *src, u32 n)
{
   memcpy32(dest, src, n * 8);
}

FASTCALL void
memcpy_single_256_failsafe(void *dest, const void *src)
{
   memcpy32(dest, src, 8);
}
