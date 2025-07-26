/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

#if !defined(__aarch64__)
   #error This header can be used only for AArch64 architecture.
#endif

#include <tilck/common/basic_defs.h>
#include <tilck/common/arch/aarch64/asm_consts.h>
#include <tilck/common/page_size.h>

/* AArch64 specific constants */
#define AARCH64_TIMER_IRQ           30
#define AARCH64_KEYBOARD_IRQ        1
#define AARCH64_COM2_COM4_IRQ       3
#define AARCH64_COM1_COM3_IRQ       4
#define AARCH64_SOUND_IRQ           5
#define AARCH64_FLOPPY_IRQ          6
#define AARCH64_LPT1_OR_SLAVE_IRQ   7
#define AARCH64_RTC_IRQ             8
#define AARCH64_ACPI_IRQ            9
#define AARCH64_PCI1_IRQ           10
#define AARCH64_PCI2_IRQ           11
#define AARCH64_PS2_MOUSE_IRQ      12
#define AARCH64_MATH_COPROC_IRQ    13
#define AARCH64_HD_IRQ             14

/* Compatibility with x86 naming */
#define X86_PC_TIMER_IRQ           AARCH64_TIMER_IRQ
#define X86_PC_KEYBOARD_IRQ        AARCH64_KEYBOARD_IRQ
#define X86_PC_COM2_COM4_IRQ       AARCH64_COM2_COM4_IRQ
#define X86_PC_COM1_COM3_IRQ       AARCH64_COM1_COM3_IRQ
#define X86_PC_SOUND_IRQ           AARCH64_SOUND_IRQ
#define X86_PC_FLOPPY_IRQ          AARCH64_FLOPPY_IRQ
#define X86_PC_LPT1_OR_SLAVE_IRQ   AARCH64_LPT1_OR_SLAVE_IRQ
#define X86_PC_RTC_IRQ             AARCH64_RTC_IRQ
#define X86_PC_ACPI_IRQ            AARCH64_ACPI_IRQ
#define X86_PC_PCI1_IRQ            AARCH64_PCI1_IRQ
#define X86_PC_PCI2_IRQ            AARCH64_PCI2_IRQ
#define X86_PC_PS2_MOUSE_IRQ       AARCH64_PS2_MOUSE_IRQ
#define X86_PC_MATH_COPROC_IRQ     AARCH64_MATH_COPROC_IRQ
#define X86_PC_HD_IRQ              AARCH64_HD_IRQ

/*
 * AArch64 exception types
 */
#define AARCH64_EXC_SYNC_SP0       0x00
#define AARCH64_EXC_IRQ_SP0        0x01
#define AARCH64_EXC_FIQ_SP0        0x02
#define AARCH64_EXC_SERROR_SP0     0x03
#define AARCH64_EXC_SYNC_SPX       0x04
#define AARCH64_EXC_IRQ_SPX        0x05
#define AARCH64_EXC_FIQ_SPX        0x06
#define AARCH64_EXC_SERROR_SPX     0x07
#define AARCH64_EXC_SYNC_ELX_64    0x08
#define AARCH64_EXC_IRQ_ELX_64     0x09
#define AARCH64_EXC_FIQ_ELX_64     0x0A
#define AARCH64_EXC_SERROR_ELX_64  0x0B
#define AARCH64_EXC_SYNC_EL0_64    0x0C
#define AARCH64_EXC_IRQ_EL0_64     0x0D
#define AARCH64_EXC_FIQ_EL0_64     0x0E
#define AARCH64_EXC_SERROR_EL0_64  0x0F


#define COM1 0x3f8
#define COM2 0x2f8
#define COM3 0x3e8
#define COM4 0x2e8

/* Defines useful when calling fault_resumable_call() */
#define ALL_FAULTS_MASK (0xFFFFFFFF)
#define PAGE_FAULT_MASK (1 << 14)

/* AArch64 specific inline functions */
static ALWAYS_INLINE void halt(void)
{
   asm volatile("wfi" : : : "memory");
}

static ALWAYS_INLINE void enable_interrupts_forced(void)
{
   asm volatile("msr daifclr, #2" : : : "memory"); /* Clear IRQ bit */
}

static ALWAYS_INLINE void disable_interrupts_forced(void)
{
   asm volatile("msr daifset, #2" : : : "memory"); /* Set IRQ bit */
}

static ALWAYS_INLINE bool are_interrupts_enabled(void)
{
   ulong daif;
   asm volatile("mrs %0, daif" : "=r"(daif));
   return !(daif & 0x40); /* Check IRQ bit */
}

static ALWAYS_INLINE void disable_interrupts(ulong *const var)
{
   ulong daif;
   asm volatile("mrs %0, daif" : "=r"(daif));
   *var = daif;
   asm volatile("msr daifset, #2" : : : "memory"); /* Set IRQ bit */
}

static ALWAYS_INLINE void enable_interrupts(const ulong *const var)
{
   asm volatile("msr daifclr, #2" : : : "memory"); /* Clear IRQ bit */
}

static ALWAYS_INLINE void invalidate_page_hw(ulong vaddr)
{
   asm volatile("tlbi vaae1, %0" : : "r"(vaddr) : "memory");
}

static ALWAYS_INLINE void write_back_and_invl_cache(void)
{
   asm volatile("dsb ish" : : : "memory");
   asm volatile("isb" : : : "memory");
}

static ALWAYS_INLINE ulong __get_curr_pdir()
{
   ulong paddr;
   asm volatile("mrs %0, ttbr0_el1" : "=r"(paddr));
   return paddr;
}

static ALWAYS_INLINE void __set_curr_pdir(ulong paddr)
{
   asm volatile("msr ttbr0_el1, %0" : : "r"(paddr) : "memory");
}

static ALWAYS_INLINE u64 RDTSC(void)
{
   u64 val;
   asm volatile("mrs %0, PMCCNTR_EL0" : "=r"(val));
   return val;
}

/* Memory barrier functions */
static ALWAYS_INLINE void memory_barrier(void)
{
   asm volatile("dsb sy" : : : "memory");
}

static ALWAYS_INLINE void instruction_barrier(void)
{
   asm volatile("isb" : : : "memory");
}

/* Cache operations */
static ALWAYS_INLINE void clean_data_cache(void)
{
   asm volatile("dsb ish" : : : "memory");
}

static ALWAYS_INLINE void invalidate_instruction_cache(void)
{
   asm volatile("ic ialluis" : : : "memory");
   asm volatile("dsb ish" : : : "memory");
   asm volatile("isb" : : : "memory");
}

static ALWAYS_INLINE ulong get_stack_ptr(void)
{
   ulong sp;
   asm volatile("mov %0, sp" : "=r"(sp));
   return sp;
}



static ALWAYS_INLINE u64 __rdtsc(void)
{
   u64 n;
   asmVolatile("mrs %0, CNTVCT_EL0" : "=r" (n));
   return n;
}

#define RDTSC() __rdtsc()

