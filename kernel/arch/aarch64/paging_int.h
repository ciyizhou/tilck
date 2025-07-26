/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

#include <tilck/common/basic_defs.h>
#include <tilck/kernel/paging.h>

/*
 * AArch64 PTE format (4KB pages):
 * | 63:52 | 51:12 | 11:10 | 9:8 | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0
 *   RES0     PPN     RES0   AP   SH  AF  nG  AP1  NS  AttrIndx  TYPE
 *
 * TYPE bits:
 *   00 = Invalid
 *   01 = Block (for L0/L1/L2)
 *   11 = Table (for L0/L1/L2)
 *   11 = Page (for L3)
 */

/* PTE type bits */
#define AARCH64_PTE_TYPE_MASK    0x3ULL
#define AARCH64_PTE_TYPE_INVALID 0x0ULL
#define AARCH64_PTE_TYPE_BLOCK   0x1ULL
#define AARCH64_PTE_TYPE_TABLE   0x3ULL
#define AARCH64_PTE_TYPE_PAGE    0x3ULL

/* PTE type values for 1-bit type field */
#define AARCH64_PTE_TYPE_INVALID_VAL 0
#define AARCH64_PTE_TYPE_BLOCK_VAL   1
#define AARCH64_PTE_TYPE_TABLE_VAL   1
#define AARCH64_PTE_TYPE_PAGE_VAL    1

/* PTE flags */
#define AARCH64_PTE_VALID       (1ULL << 0)
#define AARCH64_PTE_TABLE       (1ULL << 1)
#define AARCH64_PTE_BLOCK       (1ULL << 1)
#define AARCH64_PTE_USER        (1ULL << 6)
#define AARCH64_PTE_RO          (1ULL << 7)
#define AARCH64_PTE_SHARED      (1ULL << 8)
#define AARCH64_PTE_ACCESSED    (1ULL << 10)
#define AARCH64_PTE_DIRTY       (1ULL << 51)
#define AARCH64_PTE_CONTIG      (1ULL << 52)
#define AARCH64_PTE_NG          (1ULL << 11)
#define AARCH64_PTE_NS          (1ULL << 5)

/* Memory attributes */
#define AARCH64_MA_NORMAL       (0ULL << 2)
#define AARCH64_MA_DEVICE       (1ULL << 2)
#define AARCH64_MA_NC           (2ULL << 2)

/* Page size constants */
#define AARCH64_PAGE_SHIFT      12
#define AARCH64_PAGE_SIZE       (1ULL << AARCH64_PAGE_SHIFT)
#define AARCH64_PAGE_MASK       (AARCH64_PAGE_SIZE - 1)

/* Page table levels */
#define L0_PAGE_SHIFT        12
#define L1_PAGE_SHIFT        21
#define L2_PAGE_SHIFT        30
#define L3_PAGE_SHIFT        39

#define L0_PAGE_SIZE          (1ULL << L0_PAGE_SHIFT)
#define L1_PAGE_SIZE          (1ULL << L1_PAGE_SHIFT)
#define L2_PAGE_SIZE          (1ULL << L2_PAGE_SHIFT)
#define L3_PAGE_SIZE          (1ULL << L3_PAGE_SHIFT)

/* Number of entries per page table */
#define PTRS_PER_PT     512

/* Page frame number */
#define PFN(x)          ((x) >> AARCH64_PAGE_SHIFT)

/* Extract page table indices from virtual address */
#define L0_INDEX(vaddr) (((ulong)(vaddr) >> L0_PAGE_SHIFT) & 0x1FF)
#define L1_INDEX(vaddr) (((ulong)(vaddr) >> L1_PAGE_SHIFT) & 0x1FF)
#define L2_INDEX(vaddr) (((ulong)(vaddr) >> L2_PAGE_SHIFT) & 0x1FF)
#define L3_INDEX(vaddr) (((ulong)(vaddr) >> L3_PAGE_SHIFT) & 0x1FF)

/* Check address alignment */
#define IS_L0_PAGE_ALIGNED(addr) (!((ulong)addr & (L0_PAGE_SIZE - 1)))
#define IS_L1_PAGE_ALIGNED(addr) (!((ulong)addr & (L1_PAGE_SIZE - 1)))
#define IS_L2_PAGE_ALIGNED(addr) (!((ulong)addr & (L2_PAGE_SIZE - 1)))
#define IS_L3_PAGE_ALIGNED(addr) (!((ulong)addr & (L3_PAGE_SIZE - 1)))

/* Base page flags */
#define AARCH64_PAGE_BASE       (AARCH64_PTE_VALID | AARCH64_PTE_ACCESSED | AARCH64_PTE_DIRTY)

/* Page protection flags */
#define AARCH64_PAGE_READ       (1ULL << 1)
#define AARCH64_PAGE_WRITE      (1ULL << 2)
#define AARCH64_PAGE_EXEC       (1ULL << 3)
#define AARCH64_PAGE_USER       (1ULL << 4)
#define AARCH64_PAGE_GLOBAL     (1ULL << 5)

/* Custom software bits */
#define AARCH64_PAGE_SOFT0      (1ULL << 8)
#define AARCH64_PAGE_SOFT1      (1ULL << 9)

/*
 * When this flag is set in the 'avail' bits in page_t, it means that the page
 * is writeable even if it marked as read-only and that, on a write attempt
 * the page has to be copied (copy-on-write).
 */
#define PAGE_COW_ORIG_RW         AARCH64_PAGE_SOFT0

/*
 * When this flag is set in the 'avail' bits in page_t, it means that the page
 * is shared and, therefore, can never become a CoW page.
 */
#define PAGE_SHARED              AARCH64_PAGE_SOFT1

/* AArch64 page table entry */
union aarch64_page {
   struct {
      u64 valid : 1;        /* Valid */
      u64 type : 1;         /* Type (0=invalid, 1=block/table) */
      u64 attr_indx : 3;    /* Memory attribute index */
      u64 ns : 1;           /* Non-secure */
      u64 ap : 2;           /* Access permissions */
      u64 sh : 2;           /* Shareability */
      u64 af : 1;           /* Access flag */
      u64 ng : 1;           /* Not global */
      u64 res0 : 2;         /* Reserved */
      u64 output : 36;      /* Output address */
      u64 res1 : 4;         /* Reserved */
      u64 dbm : 1;          /* Dirty bit modifier */
      u64 cont : 1;         /* Contiguous */
      u64 pxn : 1;          /* Privileged execute never */
      u64 uxn : 1;          /* User execute never */
      u64 ignored : 4;      /* Ignored */
   };

   u64 raw;
};

/* AArch64 page table entry (alias for compatibility) */
typedef union aarch64_page aarch64_page_table_entry;

/* AArch64 page table */
struct aarch64_page_table {
   union aarch64_page entries[PTRS_PER_PT];
};

/* AArch64 page directory */
struct aarch64_page_directory {
   union aarch64_page entries[PTRS_PER_PT];
};

STATIC_ASSERT(sizeof(struct aarch64_page_table) == PAGE_DIR_SIZE);
STATIC_ASSERT(sizeof(struct aarch64_page_directory) == PAGE_DIR_SIZE);

/* Helper macros for creating page table entries */
#define AARCH64_MAKE_PAGE(paddr, flags) \
   (AARCH64_PAGE_BASE | (flags) | ((paddr) & ~AARCH64_PAGE_MASK))

#define AARCH64_MAKE_TABLE(paddr) \
   (AARCH64_PTE_VALID | AARCH64_PTE_TABLE | ((paddr) & ~AARCH64_PAGE_MASK))

#define AARCH64_MAKE_BLOCK(paddr, flags) \
   (AARCH64_PAGE_BASE | (flags) | ((paddr) & ~AARCH64_PAGE_MASK))

/* Base virtual address page directory index */
#define BASE_VADDR_PD_IDX       (USERMODE_VADDR_END >> AARCH64_L2_SHIFT)

#define EARLY_MAP_SIZE          ((8) * MB)

/* Function declarations */
void map_big_page_int(pdir_t *pdir,
                      void *vaddr,
                      ulong paddr,
                      ulong flags);

void set_pages_io(pdir_t *pdir, void *vaddr, size_t size);
