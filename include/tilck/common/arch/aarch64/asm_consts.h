/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

/* Status register flags */
#define SPSR_EL3_MASK     0x0000FF00UL /* Mask for SPSR_EL3 */
#define SPSR_EL3_DAIF     0x000000F0UL /* DAIF bits */
#define SPSR_EL3_A        0x00000100UL /* SError interrupt mask */
#define SPSR_EL3_I        0x00000200UL /* IRQ interrupt mask */
#define SPSR_EL3_F        0x00000400UL /* FIQ interrupt mask */
#define SPSR_EL3_D        0x00000800UL /* Debug exception mask */

#define SPSR_EL3_M        0x0000000FUL /* Mode bits */
#define SPSR_EL3_M_EL0T   0x00000000UL /* EL0t */
#define SPSR_EL3_M_EL1T   0x00000004UL /* EL1t */
#define SPSR_EL3_M_EL1H   0x00000005UL /* EL1h */
#define SPSR_EL3_M_EL2T   0x00000008UL /* EL2t */
#define SPSR_EL3_M_EL2H   0x00000009UL /* EL2h */
#define SPSR_EL3_M_EL3T   0x0000000CUL /* EL3t */
#define SPSR_EL3_M_EL3H   0x0000000DUL /* EL3h */

/* SCTLR_EL3 flags */
#define SCTLR_EL3_M       0x00000001UL /* MMU enable */
#define SCTLR_EL3_A       0x00000002UL /* Alignment check enable */
#define SCTLR_EL3_C       0x00000004UL /* Data cache enable */
#define SCTLR_EL3_SA      0x00000008UL /* Stack alignment check enable */
#define SCTLR_EL3_I       0x00001000UL /* Instruction cache enable */
#define SCTLR_EL3_D       0x00002000UL /* Debug enable */
#define SCTLR_EL3_CP15BEN 0x00004000UL /* CP15 barrier enable */
#define SCTLR_EL3_ITD     0x00008000UL /* IT disable */
#define SCTLR_EL3_SED     0x00010000UL /* SError disable */
#define SCTLR_EL3_UMA     0x00020000UL /* User mask access */
#define SCTLR_EL3_SCTLR2  0x00040000UL /* SCTLR2 enable */
#define SCTLR_EL3_nTWE    0x00080000UL /* Not trap WFE */
#define SCTLR_EL3_nTWI    0x00100000UL /* Not trap WFI */
#define SCTLR_EL3_nTLSMD  0x00200000UL /* Not trap LSMAccess */
#define SCTLR_EL3_LSMAOE  0x00400000UL /* LSMAccess override enable */
#define SCTLR_EL3_T0      0x00800000UL /* T0 */
#define SCTLR_EL3_EE      0x02000000UL /* Exception endianness */
#define SCTLR_EL3_E0E     0x04000000UL /* EL0 endianness */
#define SCTLR_EL3_SPAN    0x08000000UL /* Set privileged access never */
#define SCTLR_EL3_EIS     0x10000000UL /* Early interrupt select */
#define SCTLR_EL3_THE     0x20000000UL /* Thumb exception enable */
#define SCTLR_EL3_TIDCP   0x40000000UL /* TIDCP */
#define SCTLR_EL3_TID0    0x80000000UL /* TID0 */

/* Exception syndrome register (ESR_EL3) */
#define ESR_EL3_EC_MASK   0x0000003FUL /* Exception class mask */
#define ESR_EL3_EC_SHIFT  26
#define ESR_EL3_IL_MASK   0x00000001UL /* Instruction length mask */
#define ESR_EL3_IL_SHIFT  25
#define ESR_EL3_ISS_MASK  0x0001FFFFUL /* Instruction specific syndrome */

/* Exception classes */
#define ESR_EL3_EC_UNKNOWN        0x00
#define ESR_EL3_EC_WFI_WFE        0x01
#define ESR_EL3_EC_CP15_32        0x03
#define ESR_EL3_EC_CP15_64        0x04
#define ESR_EL3_EC_CP14_MR        0x05
#define ESR_EL3_EC_CP14_LS        0x06
#define ESR_EL3_EC_FP_ASIMD       0x07
#define ESR_EL3_EC_CP10_ID        0x08
#define ESR_EL3_EC_CP14_64        0x0C
#define ESR_EL3_EC_ILLEGAL_STATE  0x0E
#define ESR_EL3_EC_SVC32          0x11
#define ESR_EL3_EC_HVC32          0x12
#define ESR_EL3_EC_SMC32          0x13
#define ESR_EL3_EC_SVC64          0x15
#define ESR_EL3_EC_HVC64          0x16
#define ESR_EL3_EC_SMC64          0x17
#define ESR_EL3_EC_SYS64          0x18
#define ESR_EL3_EC_IMP_DEF_EL1   0x1F
#define ESR_EL3_EC_IMP_DEF_EL0   0x20
#define ESR_EL3_EC_IMP_DEF_EL2   0x21
#define ESR_EL3_EC_MSR_MRS_EL1   0x22
#define ESR_EL3_EC_MSR_MRS_EL2   0x23
#define ESR_EL3_EC_MSR_MRS_EL3   0x24
#define ESR_EL3_EC_MSR_MRS_EL0   0x25
#define ESR_EL3_EC_IMP_DEF_EL3   0x26
#define ESR_EL3_EC_MSR_MRS_EL0_2 0x27
#define ESR_EL3_EC_MSR_MRS_EL2_2 0x28
#define ESR_EL3_EC_MSR_MRS_EL1_2 0x29
#define ESR_EL3_EC_MSR_MRS_EL3_2 0x2A
#define ESR_EL3_EC_MSR_MRS_EL0_3 0x2B
#define ESR_EL3_EC_MSR_MRS_EL2_3 0x2C
#define ESR_EL3_EC_MSR_MRS_EL1_3 0x2D
#define ESR_EL3_EC_MSR_MRS_EL3_3 0x2E
#define ESR_EL3_EC_MSR_MRS_EL0_4 0x2F
#define ESR_EL3_EC_MSR_MRS_EL2_4 0x30
#define ESR_EL3_EC_MSR_MRS_EL1_4 0x31
#define ESR_EL3_EC_MSR_MRS_EL3_4 0x32
#define ESR_EL3_EC_MSR_MRS_EL0_5 0x33
#define ESR_EL3_EC_MSR_MRS_EL2_5 0x34
#define ESR_EL3_EC_MSR_MRS_EL1_5 0x35
#define ESR_EL3_EC_MSR_MRS_EL3_5 0x36
#define ESR_EL3_EC_MSR_MRS_EL0_6 0x37
#define ESR_EL3_EC_MSR_MRS_EL2_6 0x38
#define ESR_EL3_EC_MSR_MRS_EL1_6 0x39
#define ESR_EL3_EC_MSR_MRS_EL3_6 0x3A
#define ESR_EL3_EC_MSR_MRS_EL0_7 0x3B
#define ESR_EL3_EC_MSR_MRS_EL2_7 0x3C
#define ESR_EL3_EC_MSR_MRS_EL1_7 0x3D
#define ESR_EL3_EC_MSR_MRS_EL3_7 0x3E
#define ESR_EL3_EC_MSR_MRS_EL0_8 0x3F

/* Interrupt types */
#define IRQ_SOFTWARE      0
#define IRQ_TIMER         1
#define IRQ_EXTERNAL      2

/* System call number */
#define SYSCALL_SOFT_INTERRUPT   ESR_EL3_EC_SVC64
