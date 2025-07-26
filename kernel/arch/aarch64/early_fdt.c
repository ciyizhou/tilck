/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck_gen_headers/mod_ramfb.h>
#include <tilck/common/basic_defs.h>
#include <tilck/common/boot.h>
#include <tilck/common/string_util.h>
#include <tilck/common/printk.h>
#include <tilck/common/utils.h>
#include <tilck/kernel/errno.h>
#include <tilck/kernel/kmalloc.h>
#include <tilck/kernel/hal.h>
#include <3rd_party/fdt_helper.h>
#include <libfdt.h>
#include <multiboot.h>
#include "paging_int.h"

#define CMDLINE_BUF_SZ 1024

/* AArch64 specific variables */
static multiboot_info_t *mbi;
static multiboot_module_t *mod;
static multiboot_memory_map_t *mmmap;
static int mmmap_count = 0;
static char *cmdline_buf;
static ulong initrd_paddr;
static ulong initrd_size;
void *fdt_blob;   //save virtual address of fdt

void alloc_mbi(void)
{
   void *mbi_memtop;

   /*
    * Place multiboot information at the top of
    * 8 MB identity mapping space.
    */
   mbi_memtop = (void *)(LIN_VA_TO_PA(BASE_VA) + EARLY_MAP_SIZE);

   mbi = (multiboot_info_t *)((ulong)mbi_memtop - sizeof(*mbi));
   bzero(mbi, sizeof(*mbi));

   mod = (multiboot_module_t *)((ulong)mbi - sizeof(*mod));
   bzero(mod, sizeof(*mod));

   cmdline_buf = (char *)mod - CMDLINE_BUF_SZ;
   bzero(cmdline_buf, CMDLINE_BUF_SZ);
}

static void
add_multiboot_mmap(u64 addr, u64 size, u32 type)
{
   /* Allocate a new multiboot_memory_map */
   if (!mmmap)
      mmmap = (void *)(cmdline_buf - sizeof(multiboot_memory_map_t));
   else
      mmmap--;

   mmmap_count++;
   bzero(mmmap, sizeof(multiboot_memory_map_t));

   if (addr < mbi->mem_lower * KB)
      mbi->mem_lower = (u32)(addr / KB);

   if (addr + size > mbi->mem_upper * KB)
      mbi->mem_upper = (u32)((addr + size) / KB);

   *mmmap = (multiboot_memory_map_t) {
      .size = sizeof(multiboot_memory_map_t) - sizeof(u32),
      .addr = addr,
      .len = size,
      .type = type,
   };
}

void setup_multiboot_info(ulong ramdisk_paddr, ulong ramdisk_size)
{
   mbi->flags |= MULTIBOOT_INFO_MEMORY;

   if (cmdline_buf[0]) {
      mbi->flags |= MULTIBOOT_INFO_CMDLINE;
      mbi->cmdline = (ulong)cmdline_buf;
   }

   mbi->flags |= MULTIBOOT_INFO_MODS;
   mbi->mods_addr = (ulong)mod;
   mbi->mods_count = 1;
   mod->mod_start = ramdisk_paddr;
   mod->mod_end = mod->mod_start + ramdisk_size;

   mbi->flags |= MULTIBOOT_INFO_MEM_MAP;
   mbi->mmap_addr = (ulong)mmmap;
   mbi->mmap_length = mmmap_count * sizeof(multiboot_memory_map_t);
}

static int
fdt_get_node_linux_usable_memory(void *fdt, int node, int index,
                                 uint64_t *addr, uint64_t *size)
{
   const fdt32_t *reg;
   int len, rc;
   u64 reg_addr, reg_size;

   reg = fdt_getprop(fdt, node, "reg", &len);
   if (!reg)
      return -FDT_ERR_NOTFOUND;

   rc = fdt_get_node_addr_size(fdt, node, index, &reg_addr, &reg_size);
   if (rc)
      return rc;

   *addr = reg_addr;
   *size = reg_size;
   return 0;
}

static int
fdt_add_multiboot_mmap(void *fdt, int node, u32 type)
{
   int index = 0;
   u64 addr, size;
   int rc;

   while (1) {
      rc = fdt_get_node_linux_usable_memory(fdt, node, index, &addr, &size);
      if (rc)
         break;

      add_multiboot_mmap(addr, size, type);
      index++;
   }

   return 0;
}

static inline u64
fdt_read64(const fdt32_t *cell, size_t size)
{
   if (size == 8)
      return fdt64_to_cpu(*(const fdt64_t *)cell);
   else
      return fdt32_to_cpu(*cell);
}

static int fdt_parse_chosen(void *fdt)
{
   int node, len;
   const char *prop;

   node = fdt_path_offset(fdt, "/chosen");
   if (node < 0)
      return 0;

   prop = fdt_getprop(fdt, node, "bootargs", &len);
   if (prop && len > 0) {
      strncpy(cmdline_buf, prop, len);
      cmdline_buf[len] = '\0';
   }

   return 0;
}

static int fdt_parse_memory(void *fdt)
{
   int node, len;
   const fdt32_t *reg;
   u64 addr, size;
   int index = 0;

   node = fdt_path_offset(fdt, "/memory");
   if (node < 0)
      return 0;

   reg = fdt_getprop(fdt, node, "reg", &len);
   if (!reg)
      return 0;

   while (index * 8 < len) {
      addr = fdt_read64(&reg[index * 2], 8);
      size = fdt_read64(&reg[index * 2 + 1], 8);
      add_multiboot_mmap(addr, size, MULTIBOOT_MEMORY_AVAILABLE);
      index++;
   }

   return 0;
}

static int fdt_parse_reserved_memory(void *fdt)
{
   int node, child;

   node = fdt_path_offset(fdt, "/reserved-memory");
   if (node < 0)
      return 0;

   fdt_for_each_subnode(child, fdt, node) {
      u64 addr, size;
      int rc = fdt_get_node_addr_size(fdt, child, 0, &addr, &size);
      if (!rc)
         add_multiboot_mmap(addr, size, MULTIBOOT_MEMORY_RESERVED);
   }

   return 0;
}

/*
 * Parse flattened device tree(fdt), translate memory layout,
 * kernel command line and framebuffer into multiboot format used by tilck.
 */
multiboot_info_t *parse_fdt(void *fdt_pa)
{
   /* check device tree validity */
   if (fdt_check_header(fdt_pa))
      return NULL;

   alloc_mbi();

   fdt_parse_chosen(fdt_pa);
   fdt_parse_memory(fdt_pa);
   fdt_parse_reserved_memory(fdt_pa);

   setup_multiboot_info(initrd_paddr, initrd_size);

   fdt_blob = PA_TO_LIN_VA(fdt_pa);
   return mbi;
}
