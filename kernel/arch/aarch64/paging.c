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

/* AArch64 specific variables */
ulong linear_va_pa_offset;
ulong kernel_va_pa_offset;
static ulong base_pa;

pdir_t *__kernel_pdir;
static char kpdir_buf[sizeof(pdir_t)] ALIGNED_AT(PAGE_SIZE);

/* Early page table allocation */
#define EARLY_PT_NUM 4
static struct aarch64_page_table early_pt[EARLY_PT_NUM] ALIGNED_AT(PAGE_SIZE);

static struct aarch64_page_table *alloc_early_pt(void)
{
   static int early_pt_used = 0;

   if (early_pt_used >= EARLY_PT_NUM)
      return NULL;

   return &early_pt[early_pt_used++];
}

/* Address translation macros */
#define AARCH64_VA_TO_PA(va) ((ulong)(va) - linear_va_pa_offset)
#define AARCH64_PA_TO_VA(pa) ((void *)((ulong)(pa) + linear_va_pa_offset))

/* Page table entry access macros */
#define AARCH64_PTE_GET_PADDR(pte) ((pte).output << AARCH64_PAGE_SHIFT)
#define AARCH64_PTE_SET_PADDR(pte, paddr) ((pte).output = (paddr) >> AARCH64_PAGE_SHIFT)

/*
 * Used in early boot(before kmalloc initialization),
 * only big pages are mapped.
 */
static inline void
create_early_page_table(pdir_t *pdir,
                        ulong vaddr,
                        ulong paddr,
                        size_t length,
                        bool in_vm)
{
   ulong end_va = vaddr + length;
   // ulong pmd_paddr;
   union aarch64_page *e;
   struct aarch64_page_directory *tmpdir;

   for (; vaddr < end_va;) {

      tmpdir = (struct aarch64_page_directory *)pdir;

      /* For AArch64, we use a simplified 2-level page table (L2 -> L0) */
      int l2_idx = L2_INDEX(vaddr);
      int l0_idx = L0_INDEX(vaddr);

      e = &tmpdir->entries[l2_idx];

      if (!e->valid) {
         /* Create new page table */
         struct aarch64_page_table *pt;

         if (in_vm)
            pt = (struct aarch64_page_table *)KERNEL_VA_TO_PA((ulong)alloc_early_pt());
         else
            pt = (struct aarch64_page_table *)alloc_early_pt();

         /* Set up L2 entry pointing to the new page table */
         e->raw = 0;
         e->valid = 1;
         e->type = AARCH64_PTE_TYPE_TABLE_VAL;
         e->output = LIN_VA_TO_PA(pt) >> AARCH64_PAGE_SHIFT;
      }

      /* Get the page table */
      struct aarch64_page_table *pt = PA_TO_LIN_VA(e->output << AARCH64_PAGE_SHIFT);

      /* Create page table entry */
      pt->entries[l0_idx].raw = 0;
      pt->entries[l0_idx].valid = 1;
      pt->entries[l0_idx].type = AARCH64_PTE_TYPE_PAGE_VAL;
      pt->entries[l0_idx].attr_indx = 0; /* Normal memory */
      pt->entries[l0_idx].ns = 1;        /* Non-secure */
      pt->entries[l0_idx].ap = 3;        /* Read/write */
      pt->entries[l0_idx].sh = 0;        /* Non-shareable */
      pt->entries[l0_idx].af = 1;        /* Access flag */
      pt->entries[l0_idx].ng = 0;        /* Global */
      pt->entries[l0_idx].output = paddr >> AARCH64_PAGE_SHIFT;

      vaddr += PAGE_SIZE;
      paddr += PAGE_SIZE;
   }
}
/* Initialize paging for AArch64 */
void early_init_paging(void)
{
   /* For now, use a simple linear mapping */
   linear_va_pa_offset = BASE_VA;
   kernel_va_pa_offset = KERNEL_BASE_VA;

   __kernel_pdir = PA_TO_LIN_VA(KERNEL_VA_TO_PA(kpdir_buf));
   set_kernel_process_pdir(__kernel_pdir);
   printk("kernel base va:    %p\n", TO_PTR(KERNEL_BASE_VA));
   printk("kernel vaddr:      %p\n", TO_PTR(KERNEL_VADDR));
   printk("base va:           %p\n", TO_PTR(BASE_VA));
   printk("linear mapping:    %lu MB\n", LINEAR_MAPPING_MB);
   printk("\n");

   if (KRN32_LIN_VADDR) {
      /* We're all set up */
      return;
   }

   /*
    * We need to map the kernel's binary into
    * its new pdir.
    */
   create_early_page_table(__kernel_pdir,
                           KERNEL_BASE_VA,
                           KERNEL_VA_TO_PA(KERNEL_BASE_VA),
                           EARLY_MAP_SIZE, true);
}

/* Create a new page directory */
pdir_t *pdir_create(void)
{
   struct aarch64_page_directory *pdir;

   pdir = kzmalloc(sizeof(struct aarch64_page_directory));
   if (!pdir)
      return NULL;

   return (pdir_t *)pdir;
}

/* Clone a page directory */
pdir_t *pdir_clone(pdir_t *pdir)
{
   struct aarch64_page_directory *new_pdir;
   struct aarch64_page_directory *old_pdir;

   if (!pdir)
      return NULL;

   new_pdir = kzmalloc(sizeof(struct aarch64_page_directory));
   if (!new_pdir)
      return NULL;

   old_pdir = (struct aarch64_page_directory *)pdir;

   /* Copy the page directory entries */
   memcpy(new_pdir->entries, old_pdir->entries, sizeof(new_pdir->entries));

   return (pdir_t *)new_pdir;
}

/* Destroy a page directory */
void pdir_destroy(pdir_t *pdir)
{
   if (pdir)
      kfree(pdir);
}

/* Map a page */
int map_page(pdir_t *pdir, void *vaddr, ulong paddr, u32 pg_flags)
{
   const bool rw = !!(pg_flags & PAGING_FL_RW);
   const bool us = !!(pg_flags & PAGING_FL_US);
   ulong avail_bits = 0;
   ulong hw_pg_flags = 0;
   int rc;

   if (pg_flags & PAGING_FL_SHARED)
      avail_bits |= PAGE_SHARED;

   if (pg_flags & PAGING_FL_DO_ALLOC) {

      void *va;
      ASSERT(paddr == 0);

      if (!(va = kmalloc(PAGE_SIZE)))
         return -ENOMEM;

      if (pg_flags & PAGING_FL_ZERO_PG)
         bzero(va, PAGE_SIZE);

      paddr = LIN_VA_TO_PA(va);

   } else {

      /* PAGING_FL_ZERO_PG cannot be used without PAGING_FL_DO_ALLOC */
      ASSERT(~pg_flags & PAGING_FL_ZERO_PG);
   }

   hw_pg_flags = AARCH64_PAGE_BASE |
                 (rw ? AARCH64_PAGE_WRITE : 0) |
                 (us ? AARCH64_PAGE_USER : AARCH64_PAGE_GLOBAL) |
                 avail_bits;

   rc = map_page_int(pdir, vaddr, paddr, hw_pg_flags);

   if (UNLIKELY(rc != 0) && (pg_flags & PAGING_FL_DO_ALLOC)) {

      kfree2(PA_TO_LIN_VA(paddr), PAGE_SIZE);
   }

   return rc;
}

/* Unmap a page (permissive version) */
static inline int
__unmap_page(pdir_t *pdir, void *vaddrp, bool free_pageframe, bool permissive)
{
   struct aarch64_page_table *pt;
   const ulong vaddr = (ulong)vaddrp;
   int l2_idx = L2_INDEX(vaddr);
   int l0_idx = L0_INDEX(vaddr);

   if (l2_idx >= PTRS_PER_PT)
      return -EINVAL;

   struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;

   if (!pd->entries[l2_idx].valid) {
      if (permissive)
         return -EINVAL;
      else
         ASSERT(false);
   }

   pt = PA_TO_LIN_VA(pd->entries[l2_idx].output << AARCH64_PAGE_SHIFT);

   if (!pt->entries[l0_idx].valid) {
      if (permissive)
         return -EINVAL;
      else
         ASSERT(false);
   }

   const ulong paddr = (ulong)pt->entries[l0_idx].output << AARCH64_PAGE_SHIFT;

   pt->entries[l0_idx].raw = 0;
   invalidate_page_hw(vaddr);

   if (!pf_ref_count_dec(paddr) && free_pageframe) {
      ASSERT(paddr != KERNEL_VA_TO_PA(&zero_page));
      kfree2(PA_TO_LIN_VA(paddr), PAGE_SIZE);
   }

   return 0;
}

/* Unmap a page */
void unmap_page(pdir_t *pdir, void *vaddr, bool do_free)
{
   __unmap_page(pdir, vaddr, do_free, false);
}

/* Unmap a page (permissive version) */
int unmap_page_permissive(pdir_t *pdir, void *vaddr, bool do_free)
{
   return __unmap_page(pdir, vaddr, do_free, true);
}

/* Check if a page is mapped */
bool is_mapped(pdir_t *pdir, void *vaddr)
{
   struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;
   ulong vaddr_ul = (ulong)vaddr;
   int l2_idx = L2_INDEX(vaddr_ul);

   if (l2_idx >= PTRS_PER_PT)
      return false;

   return pd->entries[l2_idx].valid != 0;
}

/* Get the physical address of a mapped page */
ulong get_mapping(pdir_t *pdir, void *vaddr)
{
   struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;
   ulong vaddr_ul = (ulong)vaddr;
   int l2_idx = L2_INDEX(vaddr_ul);

   if (l2_idx >= PTRS_PER_PT)
      return 0;

   if (!pd->entries[l2_idx].valid)
      return 0;

   return AARCH64_PTE_GET_PADDR(pd->entries[l2_idx]);
}

/* Map a page with hardware flags */
NODISCARD int
map_page_int(pdir_t *pdir, void *vaddrp, ulong paddr, ulong hw_flags)
{
   struct aarch64_page_table *pt;
   const ulong vaddr = (ulong) vaddrp;

   ASSERT(IS_PAGE_ALIGNED(vaddr)); // the vaddr must be page-aligned
   ASSERT(IS_PAGE_ALIGNED(paddr)); // the paddr must be page-aligned

   /* For now, implement a simplified 2-level page mapping (L2 -> L0) */
   int l2_idx = L2_INDEX(vaddr);
   int l0_idx = L0_INDEX(vaddr);

   if (l2_idx >= PTRS_PER_PT)
      return -EINVAL;

   struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;

   /* Check if page table exists at L2 level */
   if (!pd->entries[l2_idx].valid) {
      /* Create new page table */
      pt = kzalloc_obj(struct aarch64_page_table);
      if (!pt)
         return -ENOMEM;

      /* Set up L2 entry pointing to the new page table */
      pd->entries[l2_idx].raw = 0;
      pd->entries[l2_idx].valid = 1;
      pd->entries[l2_idx].type = AARCH64_PTE_TYPE_TABLE_VAL;
      pd->entries[l2_idx].output = LIN_VA_TO_PA(pt) >> AARCH64_PAGE_SHIFT;
   } else {
      /* Get existing page table */
      pt = PA_TO_LIN_VA(pd->entries[l2_idx].output << AARCH64_PAGE_SHIFT);
   }

   /* Check if page is already mapped */
   if (pt->entries[l0_idx].valid)
      return -EADDRINUSE;

   /* Create page table entry */
   pt->entries[l0_idx].raw = 0;
   pt->entries[l0_idx].valid = 1;
   pt->entries[l0_idx].type = AARCH64_PTE_TYPE_PAGE_VAL;
   pt->entries[l0_idx].attr_indx = 0; /* Normal memory */
   pt->entries[l0_idx].ns = 1;        /* Non-secure */
   pt->entries[l0_idx].ap = (hw_flags & AARCH64_PAGE_WRITE) ? 3 : 1;
   pt->entries[l0_idx].sh = 0;        /* Non-shareable */
   pt->entries[l0_idx].af = 1;        /* Access flag */
   pt->entries[l0_idx].ng = (hw_flags & AARCH64_PAGE_GLOBAL) ? 0 : 1;
   pt->entries[l0_idx].output = paddr >> AARCH64_PAGE_SHIFT;

   pf_ref_count_inc(paddr);
   invalidate_page_hw(vaddr);
   return 0;
}

/* Unmap multiple pages */
void unmap_pages(pdir_t *pdir, void *vaddr, size_t page_count, bool do_free)
{
   for (size_t i = 0; i < page_count; i++) {
      unmap_page(pdir, (char *)vaddr + (i << PAGE_SHIFT), do_free);
   }
}

/* Unmap multiple pages (permissive version) */
size_t unmap_pages_permissive(pdir_t *pdir, void *vaddr, size_t page_count, bool do_free)
{
   size_t unmapped_pages = 0;
   int rc;

   for (size_t i = 0; i < page_count; i++) {
      rc = unmap_page_permissive(pdir, (char *)vaddr + (i << PAGE_SHIFT), do_free);
      unmapped_pages += (rc == 0);
   }

   return unmapped_pages;
}

/* Map a zero page */
NODISCARD int map_zero_page(pdir_t *pdir, void *vaddrp, u32 pg_flags)
{
   ulong avail_bits = 0;
   ulong hw_pg_flags = 0;
   const bool us = !!(pg_flags & PAGING_FL_US);

   /* Zero pages are always private */
   ASSERT(!(pg_flags & PAGING_FL_SHARED));

   if (pg_flags & PAGING_FL_RW)
      avail_bits |= PAGE_COW_ORIG_RW;

   hw_pg_flags = AARCH64_PAGE_BASE |
                 (us ? AARCH64_PAGE_USER : AARCH64_PAGE_GLOBAL) |
                 avail_bits;

   return map_page_int(pdir,
                       vaddrp,
                       KERNEL_VA_TO_PA(&zero_page),
                       hw_pg_flags);
}

/* Map multiple pages */
NODISCARD size_t map_pages(pdir_t *pdir,
                           void *vaddr,
                           ulong paddr,
                           size_t page_count,
                           u32 pg_flags)
{
   const bool us = !!(pg_flags & PAGING_FL_US);
   const bool rw = !!(pg_flags & PAGING_FL_RW);
   const bool big_pages = !!(pg_flags & PAGING_FL_BIG_PAGES_ALLOWED);
   ulong avail_bits = 0;
   ulong hw_pg_flags = 0;

   if (pg_flags & PAGING_FL_SHARED)
      avail_bits |= PAGE_SHARED;

   if (pg_flags & PAGING_FL_DO_ALLOC)
      NOT_IMPLEMENTED();

   hw_pg_flags = AARCH64_PAGE_BASE |
                 (rw ? AARCH64_PAGE_WRITE : 0) |
                 (us ? AARCH64_PAGE_USER : AARCH64_PAGE_GLOBAL) |
                 avail_bits;

   return map_pages_int(pdir,
                        vaddr,
                        paddr,
                        page_count,
                        big_pages,
                        hw_pg_flags);
}

/* Map multiple pages with hardware flags */
NODISCARD size_t map_pages_int(pdir_t *pdir,
                               void *vaddr,
                               ulong paddr,
                               size_t page_count,
                               bool big_pages_allowed,
                               ulong hw_flags)
{
   int rc;
   size_t pages = 0;

   ASSERT(IS_L0_PAGE_ALIGNED(vaddr));
   ASSERT(IS_L0_PAGE_ALIGNED(paddr));

   /* For AArch64, we use a simplified approach without big pages for now */
   for (size_t i = 0; i < page_count; i++, pages++) {
      rc = map_page_int(pdir, vaddr, paddr, hw_flags);
      if (UNLIKELY(rc < 0))
         goto out;
      vaddr += PAGE_SIZE;
      paddr += PAGE_SIZE;
   }

out:
   return pages;
}


/* Set 4KB page attributes */
static void set_4kb_page_attr(pdir_t *pdir, void *vaddrp, ulong attr)
{
   struct aarch64_page_table *pt;
   const ulong vaddr = (ulong)vaddrp;
   int l2_idx = L2_INDEX(vaddr);
   int l0_idx = L0_INDEX(vaddr);

   if (l2_idx >= PTRS_PER_PT)
      return;

   struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;

   if (!pd->entries[l2_idx].valid)
      return;

   pt = PA_TO_LIN_VA(pd->entries[l2_idx].output << AARCH64_PAGE_SHIFT);

   if (!pt->entries[l0_idx].valid)
      return;

   /* Set memory attributes */
   pt->entries[l0_idx].attr_indx = (attr >> 2) & 0x7;
   invalidate_page_hw(vaddr);
}

/* Set pages as IO (non-cacheable) */
void set_pages_io(pdir_t *pdir, void *vaddr, size_t size)
{
   ASSERT(IS_L0_PAGE_ALIGNED(vaddr));
   ASSERT(IS_L0_PAGE_ALIGNED(size));

   const void *end = vaddr + size;

   while (vaddr < end) {
      /* For AArch64, we use a simplified approach without big pages for now */
      set_4kb_page_attr(pdir, vaddr, AARCH64_MA_DEVICE);
      vaddr += PAGE_SIZE;
   }
}

/* Check if address is in big page */
static inline bool in_big_page(pdir_t *pdir, void *vaddrp)
{
   /* For AArch64, we use a simplified approach without big pages for now */
   return false;
}

/* Get mapping with physical address reference */
int get_mapping2(pdir_t *pdir, void *vaddrp, ulong *pa_ref)
{
   struct aarch64_page_table *pt;
   const ulong vaddr = (ulong)vaddrp;
   int l2_idx = L2_INDEX(vaddr);
   int l0_idx = L0_INDEX(vaddr);

   if (l2_idx >= PTRS_PER_PT)
      return -EFAULT;

   struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;

   if (!pd->entries[l2_idx].valid)
      return -EFAULT;

   pt = PA_TO_LIN_VA(pd->entries[l2_idx].output << AARCH64_PAGE_SHIFT);

   if (!pt->entries[l0_idx].valid)
      return -EFAULT;

   *pa_ref = ((ulong)pt->entries[l0_idx].output << AARCH64_PAGE_SHIFT) |
              (vaddr & OFFSET_IN_PAGE_MASK);
   return 0;
}

/* Set page read/write permissions */
void set_page_rw(pdir_t *pdir, void *vaddrp, bool rw)
{
   struct aarch64_page_table *pt;
   const ulong vaddr = (ulong)vaddrp;
   int l2_idx = L2_INDEX(vaddr);
   int l0_idx = L0_INDEX(vaddr);

   if (l2_idx >= PTRS_PER_PT)
      return;

   struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;

   if (!pd->entries[l2_idx].valid)
      return;

   pt = PA_TO_LIN_VA(pd->entries[l2_idx].output << AARCH64_PAGE_SHIFT);

   if (!pt->entries[l0_idx].valid)
      return;

   /* Set access permissions */
   pt->entries[l0_idx].ap = rw ? 3 : 1;
   invalidate_page_hw(vaddr);
}

/* Virtual read unsafe */
int virtual_read_unsafe(pdir_t *pdir, void *extern_va, void *dest, size_t len)
{
   ulong pgoff, pa;
   size_t tot, to_read;
   void *va;

   ASSERT(len <= INT32_MAX);

   for (tot = 0; tot < len; extern_va += to_read, tot += to_read) {
      if (get_mapping2(pdir, extern_va, &pa) < 0)
         return -EFAULT;

      pgoff = ((ulong)extern_va) & OFFSET_IN_PAGE_MASK;
      to_read = MIN(PAGE_SIZE - pgoff, len - tot);

      va = PA_TO_LIN_VA(pa);
      memcpy(dest + tot, va, to_read);
   }

   return (int)tot;
}

/* Virtual write unsafe */
int virtual_write_unsafe(pdir_t *pdir, void *extern_va, void *src, size_t len)
{
   ulong pgoff, pa;
   size_t tot, to_write;
   void *va;

   ASSERT(len <= INT32_MAX);

   for (tot = 0; tot < len; extern_va += to_write, tot += to_write) {
      if (get_mapping2(pdir, extern_va, &pa) < 0)
         return -EFAULT;

      pgoff = ((ulong)extern_va) & OFFSET_IN_PAGE_MASK;
      to_write = MIN(PAGE_SIZE - pgoff, len - tot);

      va = PA_TO_LIN_VA(pa);
      memcpy(va, src + tot, to_write);
   }

   return (int)tot;
}

/* Handle page fault */
void handle_page_fault_int(regs_t *r)
{
   /* STUB implementation for now */
   panic("Page fault at %p", (void *)r->elr_el1);
}

/* Global variables for early mapping */
static ulong base_pa;

void *prepare_early_mapping(ulong fdt_paddr)
{
   extern struct linux_image_h linux_image;
   extern char _start;

   const ulong kernel_pa = (ulong)&_start - 0x1000;
   const ulong text_offset = linux_image.text_offset;

   base_pa = kernel_pa - text_offset;

   kernel_va_pa_offset = KERNEL_BASE_VA - base_pa;
   linear_va_pa_offset = BASE_VA - base_pa;

   /* Kernel physical address must be aligned to L1 level big pages(2/4 MB) */
   ASSERT(IS_L1_PAGE_ALIGNED(kernel_pa));

   /* Copy the flattened device tree to 1MB below kernel's head */
   memcpy((void *)(kernel_pa - MB), (void *)fdt_paddr, MB);

   /* Return physical addr of FDT */
   return (void *)(kernel_pa - MB);
}




void init_hi_vmem_heap(void)
{
   size_t pages = 0;
   size_t rem_pages = HI_VMEM_SIZE >> PAGE_SHIFT;
   pdir_t *pdir = get_kernel_pdir();
   void *vaddr = (void *)HI_VMEM_START;
   struct aarch64_page_table *pt;

   if (LINEAR_MAPPING_END > HI_VMEM_START) {
      panic("LINEAR_MAPPING_MB (%d) is too big", LINEAR_MAPPING_MB);
   }

   hi_vmem_heap = kmalloc_create_regular_heap(HI_VMEM_START,
                                              HI_VMEM_SIZE,
                                              4 * PAGE_SIZE);  // min_block_size

   if (!hi_vmem_heap)
      panic("Failed to create the hi vmem heap");

   for (; pages < rem_pages; pages++) {

      /* For AArch64, we use a simplified 2-level page table (L2 -> L0) */
      int l2_idx = L2_INDEX(vaddr);
      // int l0_idx = AARCH64_L0_INDEX(vaddr);

      struct aarch64_page_directory *pd = (struct aarch64_page_directory *)pdir;
      union aarch64_page *e = &pd->entries[l2_idx];

      if (!e->valid) {
         /* Create new page table */
         pt = kzalloc_obj(struct aarch64_page_table);

         if (!pt)
            panic("kmalloc FAIL in init_hi_vmem_heap()\n");

         /* Set up L2 entry pointing to the new page table */
         e->raw = 0;
         e->valid = 1;
         e->type = AARCH64_PTE_TYPE_TABLE_VAL;
         e->output = LIN_VA_TO_PA(pt) >> AARCH64_PAGE_SHIFT;
      } else {
         /* Get the page table */
         pt = PA_TO_LIN_VA(e->output << AARCH64_PAGE_SHIFT);
      }

      vaddr += PAGE_SIZE;
   }
}

void init_early_mapping(void)
{
   extern char _start;

   pdir_t *pgd = (void *)&page_size_buf[0];

   /* Identity map the first 8 MB */
   create_early_page_table(pgd, base_pa, base_pa, EARLY_MAP_SIZE, false);

   /* Map the first 8 MB also at BASE_VA */
   create_early_page_table(pgd, BASE_VA, base_pa, EARLY_MAP_SIZE, false);

   if (!KRN32_LIN_VADDR) {
      /* Map the first 8 MB AT KERNEL_BASE_VA*/
      create_early_page_table(pgd,
                              KERNEL_BASE_VA,
                              base_pa,
                              EARLY_MAP_SIZE,
                              false);
   }

   /* Flush entire TLB */
   invalidate_page_hw(0);
   return;
}

void *failsafe_map_framebuffer(ulong paddr, ulong size)
{
   /*
    * Paging has not been initialized yet: probably we're in panic.
    * At this point, the kernel still uses page_size_buf as pdir, with only
    * the first 8 MB of the physical mapped at BASE_VA.
    */

   ulong vaddr = FAILSAFE_FB_VADDR;
   __kernel_pdir = PA_TO_LIN_VA(KERNEL_VA_TO_PA(page_size_buf));

   u32 big_pages_to_use = pow2_round_up_at(size, L1_PAGE_SIZE) / L1_PAGE_SIZE;

   for (u32 i = 0; i < big_pages_to_use; i++) {

      create_early_page_table(__kernel_pdir,
                             vaddr + i * L1_PAGE_SIZE,
                             paddr + i * L1_PAGE_SIZE,
                             L1_PAGE_SIZE,
                             false);
   }

   return (void *)vaddr;
}

