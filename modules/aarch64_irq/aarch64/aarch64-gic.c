/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck_gen_headers/config_debug.h>

#include <tilck/common/basic_defs.h>
#include <tilck/common/utils.h>
#include <tilck/kernel/modules.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/sync.h>
#include <tilck/kernel/irq.h>
#include <tilck/kernel/sched.h>
#include <tilck/kernel/kmalloc.h>
#include <tilck/kernel/timer.h>
#include <tilck/kernel/datetime.h>
#include <3rd_party/fdt_helper.h>
#include <libfdt.h>

#include <tilck/mods/irqchip.h>
#include <tilck/common/printk.h>

/* GIC registers */
#define GICD_CTLR        0x0000
#define GICD_TYPER       0x0004
#define GICD_IIDR        0x0008
#define GICD_IGROUPR     0x0080
#define GICD_ISENABLER   0x0100
#define GICD_ICENABLER   0x0180
#define GICD_ISPENDR     0x0200
#define GICD_ICPENDR     0x0280
#define GICD_ISACTIVER   0x0300
#define GICD_ICACTIVER   0x0380
#define GICD_IPRIORITYR  0x0400
#define GICD_ITARGETSR   0x0800
#define GICD_ICFGR       0x0c00
#define GICD_NSACR       0x0e00
#define GICD_SGIR        0x0f00
#define GICD_CPENDSGIR   0x0f10
#define GICD_SPENDSGIR   0x0f20

#define GICC_CTLR        0x0000
#define GICC_PMR         0x0004
#define GICC_BPR         0x0008
#define GICC_IAR         0x000c
#define GICC_EOIR        0x0010
#define GICC_RPR         0x0014
#define GICC_HPPIR       0x0018
#define GICC_ABPR        0x001c
#define GICC_AIAR        0x0020
#define GICC_AEOIR       0x0024
#define GICC_AHPPIR      0x0028
#define GICC_APR         0x00d0
#define GICC_NSAPR       0x00e0
#define GICC_IIDR        0x00fc
#define GICC_DIR         0x1000

/* GIC distributor control register bits */
#define GICD_CTLR_ENABLEGRP0     (1 << 0)
#define GICD_CTLR_ENABLEGRP1     (1 << 1)
#define GICD_CTLR_ENABLEGRP1NS   (1 << 2)
#define GICD_CTLR_ENABLEGRP1S    (1 << 3)

/* GIC CPU interface control register bits */
#define GICC_CTLR_ENABLEGRP0     (1 << 0)
#define GICC_CTLR_ENABLEGRP1     (1 << 1)
#define GICC_CTLR_ACKCTL         (1 << 2)
#define GICC_CTLR_FIQEN          (1 << 3)
#define GICC_CTLR_CBPR           (1 << 4)
#define GICC_CTLR_EOIMODE        (1 << 9)

/* GIC interrupt types */
#define GIC_SPI                 0
#define GIC_PPI                 1
#define GIC_SGI                 2
#define GIC_LPI                 3

/* GIC interrupt states */
#define GIC_IRQ_STATE_INACTIVE  0
#define GIC_IRQ_STATE_PENDING   1
#define GIC_IRQ_STATE_ACTIVE    2
#define GIC_IRQ_STATE_ACTIVE_PENDING 3

/* GIC configuration */
struct gic_config {
   void *dist_base;
   void *cpu_base;
   u32 num_irqs;
   u32 num_cpus;
};

struct gic_config gic_config;
struct irq_domain *root_domain;


/* GIC register access functions */
static inline u32 gicd_readl(u32 offset)
{
   return *(volatile u32 *)(gic_config.dist_base + offset);
}

static inline void gicd_writel(u32 offset, u32 val)
{
   *(volatile u32 *)(gic_config.dist_base + offset) = val;
}

static inline u32 gicc_readl(u32 offset)
{
   return *(volatile u32 *)(gic_config.cpu_base + offset);
}

static inline void gicc_writel(u32 offset, u32 val)
{
   *(volatile u32 *)(gic_config.cpu_base + offset) = val;
}

/* Initialize GIC */
static int gic_init(void)
{
   u32 typer;
   u32 i;

   /* Read GIC type */
   typer = gicd_readl(GICD_TYPER);
   gic_config.num_irqs = ((typer & 0x1f) + 1) * 32;
   gic_config.num_cpus = ((typer >> 5) & 0x7) + 1;

   /* Disable all interrupts */
   for (i = 0; i < gic_config.num_irqs; i += 32) {
      gicd_writel(GICD_ICENABLER + (i / 32) * 4, 0xffffffff);
      gicd_writel(GICD_ICPENDR + (i / 32) * 4, 0xffffffff);
   }

   /* Set all interrupts to group 0 */
   for (i = 0; i < gic_config.num_irqs; i += 32) {
      gicd_writel(GICD_IGROUPR + (i / 32) * 4, 0x00000000);
   }

   /* Set all interrupts to level triggered */
   for (i = 0; i < gic_config.num_irqs; i += 16) {
      gicd_writel(GICD_ICFGR + (i / 16) * 4, 0x00000000);
   }

   /* Set all interrupts to CPU 0 */
   for (i = 32; i < gic_config.num_irqs; i += 4) {
      gicd_writel(GICD_ITARGETSR + (i / 4) * 4, 0x01010101);
   }

   /* Enable GIC distributor */
   gicd_writel(GICD_CTLR, GICD_CTLR_ENABLEGRP0);

   /* Enable GIC CPU interface */
   gicc_writel(GICC_CTLR, GICC_CTLR_ENABLEGRP0);
   gicc_writel(GICC_PMR, 0xff);

   return 0;
}

/* Enable interrupt */
static void gic_enable_irq(int irq, void *priv)
{
   u32 reg = GICD_ISENABLER + (irq / 32) * 4;
   u32 bit = 1 << (irq % 32);
   gicd_writel(reg, bit);
}

/* Disable interrupt */
static void gic_disable_irq(int irq, void *priv)
{
   u32 reg = GICD_ICENABLER + (irq / 32) * 4;
   u32 bit = 1 << (irq % 32);
   gicd_writel(reg, bit);
}

/* Acknowledge interrupt */
static u32 gic_acknowledge_irq(void)
{
   return gicc_readl(GICC_IAR);
}

/* End of interrupt */
static void gic_end_of_irq(u32 irq)
{
   gicc_writel(GICC_EOIR, irq);
}

/* GIC IRQ handler */
static enum irq_action gic_irq_handler(void *ctx)
{
   u32 irq;
   enum irq_action hret = IRQ_NOT_HANDLED;

   irq = gic_acknowledge_irq();
   if (irq < 1020) {
      hret = generic_irq_handler(irq);
   }
   gic_end_of_irq(irq);

   return hret;
}

/* Parse GIC from device tree */
static int gic_fdt_parse(void *fdt)
{
   int node;
   u64 addr, size;

   /* Find GIC distributor node */
   node = fdt_node_offset_by_compatible(fdt, -1, "arm,gic-v3");
   if (node < 0) {
      node = fdt_node_offset_by_compatible(fdt, -1, "arm,gic-v2");
   }
   if (node < 0) {
      return -1;
   }

   /* Get GIC distributor base address */
   if (fdt_get_node_addr_size(fdt, node, 0, &addr, &size) < 0) {
      return -1;
   }
   gic_config.dist_base = (void *)addr;

   /* Get GIC CPU interface base address */
   if (fdt_get_node_addr_size(fdt, node, 1, &addr, &size) < 0) {
      return -1;
   }
   gic_config.cpu_base = (void *)addr;

   return 0;
}

void arch_irq_handling(regs_t *r)
{
   int hwirq, irq;
   u32 esr_el1, ec;

   ASSERT(!are_interrupts_enabled());
   ASSERT(!is_preemption_enabled());

   // Extract exception class from ESR_EL1
   esr_el1 = (u32)(r->esr_el1 >> 26);
   ec = esr_el1 & 0x3F;

   // Check if this is an IRQ (external interrupt)
   if (ec == ESR_EL3_EC_IMP_DEF_EL1) {
      // For GIC, the hardware IRQ number is in the ISS field
      hwirq = (int)(r->esr_el1 & 0x1FFFF);
      irq = root_domain->irq_map[hwirq];
      ASSERT(irq);

      push_nested_interrupt(regs_intnum(r));
      {
         generic_irq_handler(irq);
      }
      pop_nested_interrupt();
   }
}

struct fdt_irqchip_ops aarch64_gic_ops = {
   .hwirq_set_mask = gic_enable_irq,
   .hwirq_clear_mask = gic_disable_irq,
};

/* Initialize AArch64 IRQ system */
int aarch64_irq_init(void)
{
   void *fdt = fdt_get_address();

   /* Parse GIC from device tree */
   if (gic_fdt_parse(fdt) < 0) {
      printk("Failed to parse GIC from device tree\n");
      return -1;
   }

   /* Initialize GIC */
   if (gic_init() < 0) {
      printk("Failed to initialize GIC\n");
      return -1;
   }

   /* Register IRQ domain */
   root_domain = irqchip_register_irq_domain(NULL,
                                             -1, /* fdt_node */
                                             1020, /* int_nums */
                                             &aarch64_gic_ops);

   if (!root_domain) {
      printk("Failed to register AArch64 GIC IRQ domain\n");
      return -1;
   }

   return 0;
}

/* FDT-based GIC initialization */
int aarch64_gic_fdt_init(void *fdt, int node, const struct fdt_match *match)
{
   /* Parse GIC from device tree */
   if (gic_fdt_parse(fdt) < 0) {
      printk("Failed to parse GIC from device tree\n");
      return -1;
   }

   /* Initialize GIC */
   if (gic_init() < 0) {
      printk("Failed to initialize GIC\n");
      return -1;
   }

   /* Register IRQ domain */
   root_domain = irqchip_register_irq_domain(NULL,
                                             node,
                                             1020, /* int_nums */
                                             &aarch64_gic_ops);

   if (!root_domain) {
      printk("Failed to register AArch64 GIC IRQ domain\n");
      return -1;
   }

   return 0;
}

static const struct fdt_match aarch64_gic_ids[] = {
   {.compatible = "arm,gic-v3"},
   {.compatible = "arm,gic-v2"},
   { }
};

REGISTER_FDT_IRQCHIP(aarch64_gic, aarch64_gic_ids, aarch64_gic_fdt_init)
