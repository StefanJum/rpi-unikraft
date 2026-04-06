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

#ifndef __RASPI_IRQ_H__
#define __RASPI_IRQ_H__

#include <raspi/sysregs.h>

typedef int (*irq_handler_func_t)(void *);

int ukplat_irq_register(unsigned long irq, irq_handler_func_t func, void *arg);

/* type definitions */
typedef unsigned long k_sigset_t;
typedef struct {
	unsigned long fds_bits[128 / sizeof(long)];
} k_fd_set;

/* sigaction */
typedef void (*uk_sighandler_t)(int);
typedef void (*uk_sigrestore_t)(void);

struct uk_sigaction {
	uk_sighandler_t k_sa_handler;
	int k_sa_flags;
	uk_sigrestore_t k_sa_restorer;
	k_sigset_t k_sa_mask;
};
/* IRQ handlers declarations */
struct irq_handler {
	/* func() special values:
	 *   NULL: free,
	 *   HANDLER_RESERVED: reserved
	 */
	irq_handler_func_t func;
	void *arg;

	struct uk_sigaction oldaction;
};

#define IRQ_BASIC_PENDING	((volatile __u32 *)(MMIO_BASE+0x0000B200))
#define IRQ_PENDING_1		((volatile __u32 *)(MMIO_BASE+0x0000B204))
#define IRQ_PENDING_2		((volatile __u32 *)(MMIO_BASE+0x0000B208))
#define FIQ_CONTROL			((volatile __u32 *)(MMIO_BASE+0x0000B20C))
#define ENABLE_IRQS_1		((volatile __u32 *)(MMIO_BASE+0x0000B210))
#define ENABLE_IRQS_2		((volatile __u32 *)(MMIO_BASE+0x0000B214))
#define ENABLE_BASIC_IRQS	((volatile __u32 *)(MMIO_BASE+0x0000B218))
#define DISABLE_IRQS_1		((volatile __u32 *)(MMIO_BASE+0x0000B21C))
#define DISABLE_IRQS_2		((volatile __u32 *)(MMIO_BASE+0x0000B220))
#define DISABLE_BASIC_IRQS	((volatile __u32 *)(MMIO_BASE+0x0000B224))

#define IRQS_BASIC_ARM_TIMER_IRQ	(1 << 0)

#define IRQS_1_SYSTEM_TIMER_IRQ_0	(1 << 0)
#define IRQS_1_SYSTEM_TIMER_IRQ_1	(1 << 1)
#define IRQS_1_SYSTEM_TIMER_IRQ_2	(1 << 2)
#define IRQS_1_SYSTEM_TIMER_IRQ_3	(1 << 3)
#define IRQS_1_USB_IRQ				(1 << 9)

#define IRQS_MAX							4
#define IRQ_ID_ARM_GENERIC_TIMER			0
#define IRQ_ID_RASPI_ARM_SIDE_TIMER			1
#define IRQ_ID_RASPI_USB					2
#define IRQ_ID_RASPI_ARM_SYSTEM_TIMER_IRQ_3 3

void irq_vector_init( void );
int ukplat_irq_init(void);

#endif /* __RASPI_IRQ_H__ */