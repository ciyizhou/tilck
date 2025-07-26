/* SPDX-License-Identifier: BSD-2-Clause */

#include <tilck/common/basic_defs.h>
#include <tilck/common/printk.h>
#include <tilck/kernel/hal.h>
#include <tilck/kernel/interrupts.h>
#include <tilck/kernel/fault_resumable.h>
#include <tilck/kernel/process.h>
#include <tilck/kernel/arch/aarch64/arch_ints.h>

void asm_trap_entry(void);
void handle_generic_fault_int(regs_t *r, const char *fault_name);
void handle_inst_illegal_fault_int(regs_t *r, const char *fault_name);
void handle_bus_fault_int(regs_t *r, const char *fault_name);

/* AArch64 exception names - more detailed than the ones in irq.c */
const char *aarch64_detailed_exception_names[32] = {
   "Synchronous SP0", "IRQ SP0", "FIQ SP0", "SError SP0",
   "Synchronous SPx", "IRQ SPx", "FIQ SPx", "SError SPx",
   "Synchronous ELx 64", "IRQ ELx 64", "FIQ ELx 64", "SError ELx 64",
   "Synchronous EL0 64", "IRQ EL0 64", "FIQ EL0 64", "SError EL0 64",
   "Synchronous EL0 32", "IRQ EL0 32", "FIQ EL0 32", "SError EL0 32",
   "Synchronous ELx 32", "IRQ ELx 32", "FIQ ELx 32", "SError ELx 32",
   "Synchronous EL0 32", "IRQ EL0 32", "FIQ EL0 32", "SError EL0 32",
   "Synchronous ELx 32", "IRQ ELx 32", "FIQ ELx 32", "SError ELx 32"
};

static void handle_generic_fault(regs_t *r)
{
   const int int_num = r->int_num;
   handle_generic_fault_int(r, aarch64_detailed_exception_names[int_num]);
}

static void handle_inst_illegal_fault(regs_t *r)
{
   const int int_num = r->int_num;
   handle_inst_illegal_fault_int(r, aarch64_detailed_exception_names[int_num]);
}

static void handle_bus_fault(regs_t *r)
{
   const int int_num = r->int_num;
   handle_bus_fault_int(r, aarch64_detailed_exception_names[int_num]);
}

static void handle_breakpoint(regs_t *r)
{
   /*
    * Do nothing, literally. The purpose of this way of handling breakpoint is to
    * allow during development to easily put a breakpoint in user-space code and
    * from GDB (using remote debugging) to put a HW breakpoint here.
    */
   asmVolatile("nop");
}

void set_fault_handler(int ex_num, void *ptr)
{
   fault_handlers[ex_num] = (soft_int_handler_t) ptr;
}

void init_cpu_exception_handling(void)
{
   /* Set up exception vector table */
   /* Note: AArch64 uses VBAR_EL1 for exception vector table */
   asmVolatile("msr vbar_el1, %0" : : "r"(&asm_trap_entry));

   /* Set up fault handlers for common exceptions */
   set_fault_handler(AARCH64_EXC_SYNC_SP0, handle_generic_fault);
   set_fault_handler(AARCH64_EXC_SYNC_SPX, handle_generic_fault);
   set_fault_handler(AARCH64_EXC_SYNC_ELX_64, handle_generic_fault);
   set_fault_handler(AARCH64_EXC_SYNC_EL0_64, handle_generic_fault);

   /* Set up specific handlers */
   // set_fault_handler(AARCH64_EXC_INST_ILLEGAL, handle_inst_illegal_fault);
   // set_fault_handler(AARCH64_EXC_BREAKPOINT, handle_breakpoint);
   // set_fault_handler(AARCH64_EXC_DATA_ABORT, handle_bus_fault);
   // set_fault_handler(AARCH64_EXC_INST_ABORT, handle_generic_fault);
}

void handle_resumable_fault(regs_t *r)
{
   struct task *curr = get_curr_task();

   pop_nested_interrupt(); /* the fault */
   disable_interrupts_forced();
   set_return_register(curr->fault_resume_regs, 1u << regs_intnum(r));
   context_switch(curr->fault_resume_regs);
}

static void fault_in_panic(regs_t *r)
{
   const int int_num = r->int_num;

   if (is_fault_resumable(int_num))
      return handle_resumable_fault(r);

   /*
    * We might be so unlucky that printk() causes some fault(s) too: therefore,
    * not even trying to print something on the screen is safe. In order to
    * avoid generating an endless sequence of page faults in the worst case,
    * just call printk() in SAFE way here.
    */
   fault_resumable_call(
      ALL_FAULTS_MASK, printk, 5,
      "FATAL: %s [%d] while in panic state [EIP: %p]\n",
      aarch64_detailed_exception_names[int_num], int_num, regs_get_ip(r));

   /* Halt the CPU forever */
   while (true) { halt(); }
}

void handle_fault(regs_t *r)
{
	NOT_IMPLEMENTED();
}

void on_first_pdir_update(void)
{
   /* do nothing */
}
