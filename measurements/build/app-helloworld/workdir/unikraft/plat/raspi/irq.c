/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Authors: Santiago Pagani <santiagopagani@gmail.com>
 *
 * Copyright (c) 2020, NEC Laboratories Europe GmbH, NEC Corporation.
 *                     All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <stdlib.h>
#include <uk/plat/common/irq.h>
#include <uk/print.h>
#include <uk/essentials.h>
#include <raspi/irq.h>
#include <raspi/time.h>
#include <raspi/raspi_info.h>
#include <arm/time.h>
#include <uspi/types.h>

#define InvalidateInstructionCache()	\
				__asm volatile ("ic iallu" ::: "memory")
#define FlushBranchTargetCache()	\
				__asm volatile ("dsb ish; tlbi vmalle1is; dsb ish; isb" ::: "memory")

#define InstructionSyncBarrier() __asm volatile ("isb sy" ::: "memory")
#define DataSyncBarrier()	__asm volatile ("dsb sy" ::: "memory")
#define DataMemBarrier() 	__asm volatile ("dmb sy" ::: "memory")

#define ARM_IO_BASE		0x3F000000

#define ARM_IRQS_PER_REG	32

#define ARM_IRQ1_BASE		0
#define ARM_IRQ2_BASE		(ARM_IRQ1_BASE + ARM_IRQS_PER_REG)
#define ARM_IRQBASIC_BASE	(ARM_IRQ2_BASE + ARM_IRQS_PER_REG)

#define ARM_IC_BASE		(ARM_IO_BASE + 0xB000)

#define ARM_IC_IRQ_BASIC_PENDING  (ARM_IC_BASE + 0x200)
#define ARM_IC_IRQ_PENDING_1	  (ARM_IC_BASE + 0x204)
#define ARM_IC_IRQ_PENDING_2	  (ARM_IC_BASE + 0x208)
#define ARM_IC_FIQ_CONTROL	  (ARM_IC_BASE + 0x20C)
#define ARM_IC_ENABLE_IRQS_1	  (ARM_IC_BASE + 0x210)
#define ARM_IC_ENABLE_IRQS_2	  (ARM_IC_BASE + 0x214)
#define ARM_IC_ENABLE_BASIC_IRQS  (ARM_IC_BASE + 0x218)
#define ARM_IC_DISABLE_IRQS_1	  (ARM_IC_BASE + 0x21C)
#define ARM_IC_DISABLE_IRQS_2	  (ARM_IC_BASE + 0x220)
#define ARM_IC_DISABLE_BASIC_IRQS (ARM_IC_BASE + 0x224)

#define IRQ_LINES		(ARM_IRQS_PER_REG * 2 + 8)

#define ARM_IC_IRQ_PENDING(irq)	(  (irq) < ARM_IRQ2_BASE	\
				 ? ARM_IC_IRQ_PENDING_1		\
				 : ((irq) < ARM_IRQBASIC_BASE	\
				   ? ARM_IC_IRQ_PENDING_2	\
				   : ARM_IC_IRQ_BASIC_PENDING))
#define ARM_IC_IRQS_ENABLE(irq)	(  (irq) < ARM_IRQ2_BASE	\
				 ? ARM_IC_ENABLE_IRQS_1		\
				 : ((irq) < ARM_IRQBASIC_BASE	\
				   ? ARM_IC_ENABLE_IRQS_2	\
				   : ARM_IC_ENABLE_BASIC_IRQS))
#define ARM_IC_IRQS_DISABLE(irq) (  (irq) < ARM_IRQ2_BASE	\
				 ? ARM_IC_DISABLE_IRQS_1	\
				 : ((irq) < ARM_IRQBASIC_BASE	\
				   ? ARM_IC_DISABLE_IRQS_2	\
				   : ARM_IC_DISABLE_BASIC_IRQS))
#define ARM_IRQ_MASK(irq)	(1 << ((irq) & (ARM_IRQS_PER_REG-1)))

struct irq_handler irq_handlers[IRQS_MAX];

int ukplat_irq_register(unsigned long irq, irq_handler_func_t func, void *arg)
{
	switch (irq) {
		case IRQ_ID_ARM_GENERIC_TIMER:
			break;
		case IRQ_ID_RASPI_ARM_SIDE_TIMER:
			*ENABLE_BASIC_IRQS = *ENABLE_BASIC_IRQS | IRQS_BASIC_ARM_TIMER_IRQ;
			raspi_arm_side_timer_irq_clear();
			raspi_arm_side_timer_irq_enable();
			break;
		case IRQ_ID_RASPI_USB:
			break;
		case IRQ_ID_RASPI_ARM_SYSTEM_TIMER_IRQ_3:
			break;
		default:
			// Unsupported IRQ
			uk_pr_crit("ukplat_irq_register: Unsupported IRQ\n");
			return -1;
	}

	irq_handlers[irq].func = func;
	irq_handlers[irq].arg = arg;
	return 0;
}

int ukplat_irq_init(void)
{
	DataSyncBarrier ();

	InvalidateInstructionCache ();
	FlushBranchTargetCache ();
	DataSyncBarrier ();

	InstructionSyncBarrier ();

	DataMemBarrier ();

	for (unsigned int i = 0; i < IRQS_MAX; i++) {
		irq_handlers[i].func = NULL;
		irq_handlers[i].arg = NULL;
	}
	*DISABLE_BASIC_IRQS = 0xFFFFFFFF;
	*DISABLE_IRQS_1 = 0xFFFFFFFF;
	*DISABLE_IRQS_2 = 0xFFFFFFFF;
	ukplat_lcpu_enable_irq();
	return 0;
}

void show_invalid_entry_message(int type)
{
	uk_pr_crit("IRQ: %d\n", type);
}

void show_invalid_entry_message_el1_sync(uint64_t esr_el, uint64_t far_el)
{
	uk_pr_crit("ESR_EL1: %lx, FAR_EL1: %lx, SCTLR_EL1:%lx, ELR_EL1:%lx\n", esr_el, far_el, get_sctlr_el1(), get_elr_el1());
}

#define USB_IRQ_N 9
#define SYSTEM_TIMER_IRQ_3 3
#define IRQ_LINES		(32 * 2 + 8)

void ukplat_irq_handle(void)
{
	// Check if we got irqs that we are using from the uspi code
	for (unsigned nIRQ = 0; nIRQ < IRQ_LINES; nIRQ++)
	{
		if (nIRQ != SYSTEM_TIMER_IRQ_3 && nIRQ != USB_IRQ_N) {
			continue;
		}
		u32 nPendReg = ARM_IC_IRQ_PENDING (nIRQ);
		u32 nIRQMask = ARM_IRQ_MASK (nIRQ);
		
		if (read32 (nPendReg) & nIRQMask)
		{
			if (nIRQ == USB_IRQ_N && irq_handlers[IRQ_ID_RASPI_USB].func) {
				irq_handlers[IRQ_ID_RASPI_USB].func(irq_handlers[IRQ_ID_RASPI_USB].arg);
				return;
			}
			else if (nIRQ == SYSTEM_TIMER_IRQ_3 && irq_handlers[IRQ_ID_RASPI_ARM_SYSTEM_TIMER_IRQ_3].func) {
				irq_handlers[IRQ_ID_RASPI_ARM_SYSTEM_TIMER_IRQ_3].func(irq_handlers[IRQ_ID_RASPI_ARM_SYSTEM_TIMER_IRQ_3].arg);
				return;
			}
		}
	}

	__u32 irq_bits = *IRQ_BASIC_PENDING & *ENABLE_BASIC_IRQS;
	if ((irq_bits & IRQS_BASIC_ARM_TIMER_IRQ) && irq_handlers[IRQ_ID_RASPI_ARM_SIDE_TIMER].func) {
		irq_handlers[IRQ_ID_RASPI_ARM_SIDE_TIMER].func(NULL);
		return;
	}

	if ((get_el0(cntv_ctl) & GT_TIMER_IRQ_STATUS) && irq_handlers[IRQ_ID_ARM_GENERIC_TIMER].func) {
		irq_handlers[IRQ_ID_ARM_GENERIC_TIMER].func(NULL);
		return;
	}

	uk_pr_crit("ukplat_irq_handle: Unhandled IRQ\n");
	uk_pr_crit("IRQ_BASIC_PENDING: %u\n", *IRQ_BASIC_PENDING);
	uk_pr_crit("IRQ_PENDING_1: %u\n", *IRQ_PENDING_1);
	uk_pr_crit("IRQ_PENDING_2: %u\n", *IRQ_PENDING_2);
	uk_pr_crit("ENABLE_BASIC_IRQS: %u\n", *ENABLE_BASIC_IRQS);
	uk_pr_crit("ENABLE_IRQS_1: %u\n", *ENABLE_IRQS_1);
	uk_pr_crit("ENABLE_IRQS_2: %u\n", *ENABLE_IRQS_2);
	uk_pr_crit("DISABLE_BASIC_IRQS: %u\n", *DISABLE_BASIC_IRQS);
	uk_pr_crit("DISABLE_IRQS_1: %u\n", *DISABLE_IRQS_1);
	uk_pr_crit("DISABLE_IRQS_2: %u\n", *DISABLE_IRQS_2);
	uk_pr_crit("get_el0(cntv_ctl): %lu\n", get_el0(cntv_ctl));
	uk_pr_crit("irq_handlers[IRQ_ID_ARM_GENERIC_TIMER]: %lu\n", (unsigned long)irq_handlers[IRQ_ID_ARM_GENERIC_TIMER].func);
	uk_pr_crit("irq_handlers[IRQ_ID_RASPI_ARM_SIDE_TIMER]: %lu\n", (unsigned long)irq_handlers[IRQ_ID_RASPI_ARM_SIDE_TIMER].func);
	uk_pr_crit("irq_handlers[IRQ_ID_RASPI_USB]: %lu\n", (unsigned long)irq_handlers[IRQ_ID_RASPI_USB].func);
	while(1);
}

void ukplat_lcpu_halt_irq(void)
{
	UK_ASSERT(ukplat_lcpu_irqs_disabled());

	ukplat_lcpu_enable_irq();
	halt();
	ukplat_lcpu_disable_irq();
}
