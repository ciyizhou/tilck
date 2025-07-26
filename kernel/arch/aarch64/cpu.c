/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/string_util.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/kmalloc.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/process_mm.h>
#include <tilck/kernel/debug_utils.h>

/* Initialize CPU */
void init_cpu(void)
{
   /* STUB implementation */
}

/* Get CPU ID */
u32 get_cpu_id(void)
{
   /* STUB implementation */
   return 0;
}

/* Get CPU count */
u32 get_cpu_count(void)
{
   /* STUB implementation */
   return 1;
}

/* Set CPU affinity */
void set_cpu_affinity(u32 cpu_id)
{
   /* STUB implementation */
}

/* Get current CPU ID */
u32 get_curr_cpu_id(void)
{
   /* STUB implementation */
   return 0;
}

void enable_cpu_features(void) { /* nothing needed for AArch64 */ }
