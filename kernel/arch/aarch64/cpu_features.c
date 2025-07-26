/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/assert.h>

#include <tilck/common/printk.h>
#include <tilck/common/string_util.h>
#include <tilck/common/arch/aarch64/aarch64_utils.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/paging.h>
#include <3rd_party/fdt_helper.h>
#include <libfdt.h>

volatile struct aarch64_cpu_features aarch64_cpu_features;

static char model_name[64];
static char isa_string_buf[256];

static int fdt_parse_model(void *fdt)
{
   int node, len;
   const char *prop;

   node = fdt_path_offset(fdt, "/");
   if (node < 0)
      return -1;

   prop = fdt_getprop(fdt, node, "model", &len);
   if (prop && len > 0) {
      strncpy(model_name, prop, sizeof(model_name) - 1);
      model_name[sizeof(model_name) - 1] = '\0';
   }

   return 0;
}

static int fdt_parse_isa_extensions(void *fdt)
{
   int node, len;
   const char *prop;

   node = fdt_path_offset(fdt, "/cpus");
   if (node < 0)
      return -1;

   prop = fdt_getprop(fdt, node, "compatible", &len);
   if (prop && len > 0) {
      strncpy(isa_string_buf, prop, sizeof(isa_string_buf) - 1);
      isa_string_buf[sizeof(isa_string_buf) - 1] = '\0';
   }

   return 0;
}

/*
 * We must set the contents of aarch64_cpu_features before early_init_paging(),
 * because when setup the kernel mapping, we have to make sure the vendor
 * defined page attribute bits are set correctly in aarch64_cpu_features.
 */
void early_get_cpu_features(void)
{
   struct aarch64_cpu_features *f = (void *)&aarch64_cpu_features;
   void *dtb = (void *)LIN_VA_TO_PA(fdt_get_address());

   /* Read CPU identification registers */
   __asm__ volatile("mrs %0, midr_el1" : "=r" (f->midr_el1));
   __asm__ volatile("mrs %0, mpidr_el1" : "=r" (f->mpidr_el1));
   __asm__ volatile("mrs %0, id_aa64isar0_el1" : "=r" (f->id_aa64isar0_el1));
   __asm__ volatile("mrs %0, id_aa64isar1_el1" : "=r" (f->id_aa64isar1_el1));
   __asm__ volatile("mrs %0, id_aa64pfr0_el1" : "=r" (f->id_aa64pfr0_el1));
   __asm__ volatile("mrs %0, id_aa64pfr1_el1" : "=r" (f->id_aa64pfr1_el1));
   __asm__ volatile("mrs %0, id_aa64mmfr0_el1" : "=r" (f->id_aa64mmfr0_el1));
   __asm__ volatile("mrs %0, id_aa64mmfr1_el1" : "=r" (f->id_aa64mmfr1_el1));
   __asm__ volatile("mrs %0, id_aa64mmfr2_el1" : "=r" (f->id_aa64mmfr2_el1));

   fdt_parse_model(dtb);
   fdt_parse_isa_extensions(dtb);
}

void get_cpu_features(void)
{
   /*
    * Already done in early_get_cpu_features(),
    * here just dump cpu features
    */
   dump_aarch64_features();
}

void dump_aarch64_features(void)
{
   struct aarch64_cpu_features *f = (void *)&aarch64_cpu_features;

   printk("MODEL: %s\n", model_name);
   printk("MIDR_EL1: 0x%lx\n", f->midr_el1);
   printk("MPIDR_EL1: 0x%lx\n", f->mpidr_el1);
   printk("ID_AA64ISAR0_EL1: 0x%lx\n", f->id_aa64isar0_el1);
   printk("ID_AA64ISAR1_EL1: 0x%lx\n", f->id_aa64isar1_el1);
   printk("ID_AA64PFR0_EL1: 0x%lx\n", f->id_aa64pfr0_el1);
   printk("ID_AA64PFR1_EL1: 0x%lx\n", f->id_aa64pfr1_el1);
   printk("ID_AA64MMFR0_EL1: 0x%lx\n", f->id_aa64mmfr0_el1);
   printk("ID_AA64MMFR1_EL1: 0x%lx\n", f->id_aa64mmfr1_el1);
   printk("ID_AA64MMFR2_EL1: 0x%lx\n", f->id_aa64mmfr2_el1);
   printk("ISA: %s\n", isa_string_buf);
}
