/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>

#include <tilck/kernel/debug_utils.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/irq.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/elf_utils.h>
#include <tilck/kernel/paging_hw.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/arch/aarch64/arch_utils.h>

#include <elf.h>

static bool fp_is_valid(ulong fp, ulong sp)
{
   ulong low = sp + 2 * sizeof(void *);
   ulong high = (sp + (KERNEL_STACK_SIZE - 1)) & ~(KERNEL_STACK_SIZE - 1);

   if (!fp || fp < low || fp > high || fp & 0xf)
      return false;
   else
      return true;
}

static size_t
stackwalk_aarch64(void **frames,
                  size_t count,
                  void *fp,
                  pdir_t *pdir)
{
   bool curr_pdir = false;
   void *retAddr;
   size_t i;
   ulong old_fp;

   if (!fp) {
      fp = __builtin_frame_address(0);
   }

   if (!pdir) {
      pdir = get_curr_pdir();
      curr_pdir = true;
   }

   for (i = 0; i < count; i++) {
      old_fp = (ulong)fp;
      if ((ulong)fp < BASE_VA)
         break;

      if (curr_pdir) {
         retAddr = *((void **)fp + 1); // [fp+8]: return address
         fp = *((void **)fp + 0);      // [fp+0]: previous fp
      } else {
         if (virtual_read(pdir, (void **)fp + 1,
                          &retAddr, sizeof(retAddr)) < 0)
            break;
         if (virtual_read(pdir, (void **)fp + 0,
                          &fp, sizeof(fp)) < 0)
            break;
      }

      if (!fp_is_valid((ulong)fp, old_fp)) {
         if (fp_is_valid((ulong)retAddr, old_fp)) {
            fp = retAddr;
            continue;
         }
         break;
      }
      frames[i] = retAddr;
   }
   return i;
}

void dump_stacktrace(void *fp, pdir_t *pdir)
{
   void *frames[32] = {0};
   size_t c = stackwalk_aarch64(frames, ARRAY_SIZE(frames), fp, pdir);
   printk("Stacktrace (%lu frames):\n", c);
   for (size_t i = 0; i < c; i++) {
      long off = 0;
      u32 sym_size;
      ulong va = (ulong)frames[i];
      const char *sym_name;
      sym_name = find_sym_at_addr(va, &off, &sym_size);
      if (sym_name && off == 0) {
         sym_name = find_sym_at_addr(va - 1, &off, &sym_size);
         off++;
      }
      printk("[%p] %s + 0x%lx\n", TO_PTR(va), sym_name ? sym_name : "???", off);
   }
   printk("\n");
}

void dump_spsr(ulong spsr)
{
   printk("spsr_el1: %p\n", TO_PTR(spsr));
   // 可根据需要解析SPSR各bit
}

void dump_regs(regs_t *r)
{
   dump_spsr(r->spsr_el1);
   printk("x0 : %016lx  x1 : %016lx  x2 : %016lx  x3 : %016lx\n", r->x0, r->x1, r->x2, r->x3);
   printk("x4 : %016lx  x5 : %016lx  x6 : %016lx  x7 : %016lx\n", r->x4, r->x5, r->x6, r->x7);
   printk("x8 : %016lx  x9 : %016lx  x10: %016lx  x11: %016lx\n", r->x8, r->x9, r->x10, r->x11);
   printk("x12: %016lx  x13: %016lx  x14: %016lx  x15: %016lx\n", r->x12, r->x13, r->x14, r->x15);
   printk("x16: %016lx  x17: %016lx  x18: %016lx  x19: %016lx\n", r->x16, r->x17, r->x18, r->x19);
   printk("x20: %016lx  x21: %016lx  x22: %016lx  x23: %016lx\n", r->x20, r->x21, r->x22, r->x23);
   printk("x24: %016lx  x25: %016lx  x26: %016lx  x27: %016lx\n", r->x24, r->x25, r->x26, r->x27);
   printk("x28: %016lx  x29: %016lx  x30: %016lx\n", r->x28, r->x29, r->x30);
   printk("sp: %016lx  elr_el1: %016lx\n", r->sp_el0, r->elr_el1);
   printk("spsr_el1: %016lx  esr_el1: %016lx  far_el1: %016lx\n",
          r->spsr_el1, r->esr_el1, r->far_el1);
}

void dump_raw_stack(ulong addr)
{
   printk("Raw stack dump:\n");
   for (int i = 0; i < 36; i += 4) {
      printk("%p: ", TO_PTR(addr));
      for (int j = 0; j < 4; j++) {
         printk("%016lx ", *(ulong *)addr);
         addr += sizeof(ulong);
      }
      printk("\n");
   }
}

int debug_qemu_turn_off_machine(void)
{
   poweroff();
   return 0;
}
