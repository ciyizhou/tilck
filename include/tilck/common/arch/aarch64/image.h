/* SPDX-License-Identifier: BSD-2-Clause */

#pragma once

#ifndef __aarch64__
   #error This header can be used only when building for aarch64.
#endif

/* aarch64 kernel image header */
struct linux_image_h {
   u32 code0;        /* Executable code */
   u32 code1;        /* Executable code */
   u64 text_offset;  /* Image load offset, LE */
   u64 image_size;   /* Effective Image size, LE */
   u64 flags;        /* Kernel flags, LE */
   u64 res2;         /* reserved */
   u64 res3;         /* reserved */
   u64 res4;         /* reserved */
   u32 magic;        /* Magic number */
   u32 res5;         /* reserved */
};
