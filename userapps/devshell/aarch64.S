// SPDX-License-Identifier: BSD-2-Clause

.text

.global is_stack_aligned_16
.global execute_illegal_instruction

// This function checks if SP was aligned at 16-bytes boundary *before* the
// function was called.
is_stack_aligned_16:
   mov x1, sp
   and x0, x1, #0xf
   cbz x0, .ok
   mov x0, #0
   b .out
.ok:
   mov x0, #1
.out:
   ret

execute_illegal_instruction:
   .zero 4
   ret // We should never get here
