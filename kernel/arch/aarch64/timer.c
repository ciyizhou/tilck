/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck_gen_headers/config_debug.h>

#include <tilck/common/basic_defs.h>
#include <tilck/common/utils.h>
#include <tilck/kernel/modules.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/sync.h>
#include <tilck/kernel/irq.h>
#include <tilck/kernel/kmalloc.h>
#include <tilck/kernel/timer.h>
#include <tilck/kernel/datetime.h>
#include <3rd_party/fdt_helper.h>
#include <libfdt.h>

#include <tilck/mods/irqchip.h>

extern struct irq_domain *root_domain;

static ulong aarch64_timebase;
static ulong aarch64_hz;

/* Read the current time from CNTPCT_EL0 */
static inline u64 rdtime(void)
{
   u64 time;
   __asm__ volatile("mrs %0, cntpct_el0" : "=r" (time));
   return time;
}

/* Read the frequency from CNTFRQ_EL0 */
static inline u32 rdtime_freq(void)
{
   u32 freq;
   __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (freq));
   return freq;
}

/* Set the timer compare value */
static inline void set_timer_compare(u64 value)
{
   __asm__ volatile("msr cntp_cval_el0, %0" : : "r" (value));
}

/* Enable/disable timer */
static inline void enable_timer(void)
{
   __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (1));
}

static inline void disable_timer(void)
{
   __asm__ volatile("msr cntp_ctl_el0, %0" : : "r" (0));
}

static enum irq_action aarch64_timer_irq_handler(void *ctx)
{
   enum irq_action hret = IRQ_NOT_HANDLED;

   disable_timer();
   enable_interrupts_forced();
   hret = generic_irq_handler(X86_PC_TIMER_IRQ);
   disable_interrupts_forced();

   /* Set next timer interrupt */
   set_timer_compare(rdtime() + aarch64_timebase / aarch64_hz);
   enable_timer();

   return hret;
}

static ulong fdt_parse_timebase_frequency(void)
{
   int cpus_node, len;
   const fdt32_t *prop;
   void *fdt = fdt_get_address();

   cpus_node = fdt_path_offset(fdt, "/cpus");
   if (cpus_node < 0)
      return rdtime_freq(); /* Use hardware frequency as fallback */

   prop = fdt_getprop(fdt, cpus_node, "timebase-frequency", &len);
   if (prop && len)
      return fdt32_to_cpu(*prop);

   return rdtime_freq(); /* Use hardware frequency as fallback */
}

DEFINE_IRQ_HANDLER_NODE(aarch64_timer_irq_node, aarch64_timer_irq_handler, NULL);

u32 hw_timer_setup(u32 interval)
{
   int irq;
   u64 actual_interval;

   aarch64_timebase = fdt_parse_timebase_frequency();
   aarch64_hz = TS_SCALE / interval;
   actual_interval = TS_SCALE;
   actual_interval *= aarch64_timebase / aarch64_hz;
   actual_interval /= aarch64_timebase;

   ASSERT(IN_RANGE_INC(aarch64_hz, 18, 1000));
   ASSERT(actual_interval < UINT32_MAX);

   irq = irqchip_get_free_irq(root_domain, IRQ_TIMER);
   root_domain->irq_map[IRQ_TIMER] = irq;
   irq_install_handler(irq, &aarch64_timer_irq_node);

   /* Set initial timer interrupt */
   set_timer_compare(rdtime() + aarch64_timebase / aarch64_hz);
   enable_timer();

   return (u32)actual_interval;
}
