/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck_gen_headers/config_mm.h>

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>
#include <tilck/common/utils.h>

#include <tilck/kernel/paging.h>
#include <tilck/kernel/paging_hw.h>
#include <tilck/kernel/irq.h>
#include <tilck/kernel/kmalloc.h>
#include <tilck/kernel/debug_utils.h>
#include <tilck/kernel/sched.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/user.h>
#include <tilck/kernel/elf_utils.h>
#include <tilck/kernel/system_mmap.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/signal.h>
#include <tilck/kernel/process_mm.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/vdso.h>
#include <tilck/kernel/cmdline.h>

#include <tilck/mods/tracing.h>

#include <sys/mman.h>      // system header
#include "paging_int.h"
#include "paging_generic.h"

extern u32 __mem_lower_kb;
extern u32 __mem_upper_kb;

u32 *pageframes_refcount;
ulong phys_mem_lim;
struct kmalloc_heap *hi_vmem_heap;

void *ioremap(ulong paddr, size_t size)
{
   ulong offset;
   size_t count;
   size_t page_count;
   const u32 pg_flags = PAGING_FL_RW;

   offset = paddr & (PAGE_SIZE - 1);
   paddr -= offset;
   size = pow2_round_up_at(size + offset, PAGE_SIZE);
   page_count = size / PAGE_SIZE;

   void *vaddr = hi_vmem_reserve(size);
   if (!vaddr) {
      printk("Unable to reserve hi vmem for ioremap() at %p\n", (void *)paddr);
      return NULL;
   }

   count = map_kernel_pages(vaddr,
                            paddr,
                            page_count,
                            pg_flags);

   if (count < page_count) {
      printk("WARNING: unable to map ioremap() at %p\n", vaddr);
      unmap_kernel_pages(vaddr, count, false);
      hi_vmem_release(vaddr, size);
      return NULL;
   }

   set_pages_io(get_kernel_pdir(), vaddr, size);
   return vaddr + offset;
}

void iounmap(void *vaddr)
{
   size_t size = 0;

   ASSERT(IS_PAGE_ALIGNED(vaddr));
   disable_preemption();
   {
      per_heap_kfree(hi_vmem_heap, vaddr, &size, 0);
   }
   enable_preemption();
   ASSERT(IS_PAGE_ALIGNED(size));

   unmap_kernel_pages(vaddr, size / PAGE_SIZE, false);
}

void retain_pageframes_mapped_at(pdir_t *pdir, void *vaddrp, size_t len)
{
   ASSERT(IS_PAGE_ALIGNED(vaddrp));
   ASSERT(IS_PAGE_ALIGNED(len));

   ulong paddr;
   ulong vaddr = (ulong)vaddrp;
   const ulong vaddr_end = vaddr + len;

   for (; vaddr < vaddr_end; vaddr += PAGE_SIZE) {

      if (get_mapping2(pdir, (void *)vaddr, &paddr) < 0)
         continue; /* not mapped, that's fine */

      __pf_ref_count_inc(paddr);
   }
}

void release_pageframes_mapped_at(pdir_t *pdir, void *vaddrp, size_t len)
{
   ASSERT(IS_PAGE_ALIGNED(vaddrp));
   ASSERT(IS_PAGE_ALIGNED(len));

   ulong paddr;
   ulong vaddr = (ulong)vaddrp;
   const ulong vaddr_end = vaddr + len;

   for (; vaddr < vaddr_end; vaddr += PAGE_SIZE) {

      if (get_mapping2(pdir, (void *)vaddr, &paddr) < 0)
         continue; /* not mapped, that's fine */

      __pf_ref_count_dec(paddr);
   }
}

void invalidate_page(ulong vaddr)
{
   invalidate_page_hw(vaddr);
}

void init_paging(void)
{
   size_t pagesframes_refcount_bufsize;

   /* Initialize physical memory limit */
   phys_mem_lim = (ulong)MIN((__mem_upper_kb << 10) - (__mem_lower_kb << 10),
                              LINEAR_MAPPING_SIZE);

   /* Allocate pageframes reference count array */
   pagesframes_refcount_bufsize =
      (phys_mem_lim >> PAGE_SHIFT) * sizeof(pageframes_refcount[0]);

   pageframes_refcount = kzmalloc(pagesframes_refcount_bufsize);

   if (!pageframes_refcount) {
      printk("Unable to allocate pageframes_refcount\n");
      panic("Unable to allocate pageframes_refcount");
   }

   /* Initialize high virtual memory heap */
   init_hi_vmem_heap();

   /* Initialize paging for AArch64 */
   early_init_paging();
}

void *hi_vmem_reserve(size_t size)
{
   void *res = NULL;

   disable_preemption();
   {
      if (LIKELY(hi_vmem_heap != NULL))
         res = per_heap_kmalloc(hi_vmem_heap, &size, 0);
   }
   enable_preemption();
   return res;
}

void hi_vmem_release(void *ptr, size_t size)
{
   disable_preemption();
   {
      per_heap_kfree(hi_vmem_heap, ptr, &size, 0);
   }
   enable_preemption();
}

bool hi_vmem_avail(void)
{
   return pageframes_refcount != NULL;
}

int virtual_read(pdir_t *pdir, void *extern_va, void *dest, size_t len)
{
   return virtual_read_unsafe(pdir, extern_va, dest, len);
}

int virtual_write(pdir_t *pdir, void *extern_va, void *src, size_t len)
{
   return virtual_write_unsafe(pdir, extern_va, src, len);
}

NODISCARD size_t
map_zero_pages(pdir_t *pdir,
               void *vaddrp,
               size_t page_count,
               u32 pg_flags)
{
   size_t mapped = 0;
   const ulong vaddr = (ulong)vaddrp;

   for (size_t i = 0; i < page_count; i++) {
      int rc = map_zero_page(pdir, (void *)(vaddr + i * PAGE_SIZE), pg_flags);
      if (rc < 0)
         break;
      mapped++;
   }

   return mapped;
}

void *
map_framebuffer(pdir_t *pdir,
                ulong paddr,
                ulong vaddr,
                ulong size,
                bool user_mmap)
{
   if (!get_kernel_pdir())
      return failsafe_map_framebuffer(paddr, size);

   if (!pageframes_refcount)
      return failsafe_map_framebuffer(paddr, size);

   size_t count;
   const size_t page_count = pow2_round_up_at(size, PAGE_SIZE) / PAGE_SIZE;
   const u32 pg_flags = PAGING_FL_RW |
                        PAGING_FL_SHARED |
                        (user_mmap ? PAGING_FL_US : 0);

   if (!vaddr) {

      ASSERT(!user_mmap); /* user mappings always have a vaddr at this layer */
      vaddr = (ulong) hi_vmem_reserve(size);

      if (!vaddr) {

         /*
          * This should NEVER happen. The allocation of the hi vmem does not
          * depend at all from the system. It's all on Tilck. We have 128 MB
          * of virtual space that we can allocate as we want. Unless there's
          * a bug in kmalloc(), we'll never get here.
          */

         if (in_panic()) {

            /*
             * But, in the extremely unlucky case we end up here, there's still
             * one thing we can do, at least to be able to show something on
             * the screen: use a failsafe VADDR for the framebuffer.
             */

            vaddr = FAILSAFE_FB_VADDR;

         } else {

            panic("Unable to reserve hi vmem for the framebuffer");
         }
      }
   }

   count = map_pages(pdir,
                     (void *)vaddr,
                     paddr,
                     page_count,
                     pg_flags);

   if (count < page_count) {

      if (user_mmap) {

         /* This is bad, but not terrible */
         printk("WARNING: unable to mmap framebuffer at %p\n", (void *)vaddr);
         unmap_pages_permissive(pdir, (void *)vaddr, count, false);
         return NULL;
      }

      /*
       * What if this is the only framebuffer available for showing something
       * on the screen? Well, we're screwed. But this should *never* happen.
       */

      panic("Unable to map the framebuffer in the virtual space");
   }

   if (kopt_fb_no_wc) {
      printk("paging: skip marking framebuffer pages as WC (kopt_fb_no_wc)\n");
      return (void *)vaddr;
   }

   size = pow2_round_up_at(size, PAGE_SIZE);
   // set_pages_pat_wc(pdir, (void *) vaddr, size);
	NOT_IMPLEMENTED();
   return (void *)vaddr;
}

void handle_page_fault(regs_t *r)
{
   handle_page_fault_int(r);
}
