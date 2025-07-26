/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once
#include <tilck/common/basic_defs.h>

/* AArch64 CPU features structure */
struct aarch64_cpu_features {
   u64 midr_el1;           /* Main ID Register */
   u64 mpidr_el1;          /* Multiprocessor Affinity Register */
   u64 id_aa64isar0_el1;   /* AArch64 Instruction Set Attribute Register 0 */
   u64 id_aa64isar1_el1;   /* AArch64 Instruction Set Attribute Register 1 */
   u64 id_aa64pfr0_el1;    /* AArch64 Processor Feature Register 0 */
   u64 id_aa64pfr1_el1;    /* AArch64 Processor Feature Register 1 */
   u64 id_aa64mmfr0_el1;   /* AArch64 Memory Model Feature Register 0 */
   u64 id_aa64mmfr1_el1;   /* AArch64 Memory Model Feature Register 1 */
   u64 id_aa64mmfr2_el1;   /* AArch64 Memory Model Feature Register 2 */
};

static ALWAYS_INLINE bool in_hypervisor(void)
{
   // TODO: implement in_hypervisor() for RISCV
   return false;
}

extern volatile struct aarch64_cpu_features aarch64_cpu_features;

void get_cpu_features(void);
void dump_aarch64_features(void);

