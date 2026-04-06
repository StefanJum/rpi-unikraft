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

#include <uk/plat/common/bootinfo.h>
#include <uk/plat/memory.h>
#include <uk/plat/bootstrap.h>
#include <uk/plat/time.h>
#include <arm/cpu.h>
#include <uk/plat/common/sections.h>
#include <raspi/console.h>
#include <raspi/time.h>
#include <raspi/irq.h>
#include <uk/print.h>
#include <uk/arch/types.h>
#include <stdio.h>

#include <raspi/setup.h>

static uint64_t assembly_entry;
static uint64_t hardware_init_done;

uint64_t _libraspiplat_get_reset_time(void)
{
	return assembly_entry;
}

uint64_t _libraspiplat_get_hardware_init_time(void)
{
	return hardware_init_done;
}

static void __libraspiplat_mem_init(void) 
{
	struct ukplat_bootinfo *bi = ukplat_bootinfo_get();
	if (unlikely(!bi)) {
		UK_CRASH("Failed to get bootinfo\n");
	}

	// stack
	int rc = ukplat_memregion_list_insert(
		&bi->mrds,
		&(struct ukplat_memregion_desc){
			.vbase = 0,
			.pbase = 0,
			.len   = __TEXT,
			.type  = UKPLAT_MEMRT_RESERVED,
			.flags = UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE,
		});
	if (unlikely(rc < 0))
		uk_pr_err("Failed to add stack memory region descriptor.\n");

	// text
	rc = ukplat_memregion_list_insert(
		&bi->mrds,
		&(struct ukplat_memregion_desc){
			.vbase = __TEXT,
			.pbase = __TEXT,
			.len   = (size_t) __ETEXT - (size_t) __TEXT,
			.type  = UKPLAT_MEMRT_RESERVED,
			.flags = UKPLAT_MEMRF_READ,
		});
	if (unlikely(rc < 0))
		uk_pr_err("Failed to add text memory region descriptor.\n");

	// rodata
	rc = ukplat_memregion_list_insert(
		&bi->mrds,
		&(struct ukplat_memregion_desc){
			.vbase = __RODATA,
			.pbase = __RODATA,
			.len   = (size_t) __ERODATA - (size_t) __RODATA,
			.type  = UKPLAT_MEMRT_RESERVED,
			.flags = UKPLAT_MEMRF_READ,
		});
	if (unlikely(rc < 0))
		uk_pr_err("Failed to add rodata memory region descriptor.\n");

	// TODO: Why is this section usually 0? Can we just always leave it out?
	if ((size_t) __ECTORS - (size_t) __CTORS != 0) {
		// ctors
		rc = ukplat_memregion_list_insert(
			&bi->mrds,
			&(struct ukplat_memregion_desc){
				.vbase = __CTORS,
				.pbase = __CTORS,
				.len   = (size_t) __ECTORS - (size_t) __CTORS,
				.type  = UKPLAT_MEMRT_RESERVED,
				.flags = UKPLAT_MEMRF_READ,
			});
		if (unlikely(rc < 0))
			uk_pr_err("Failed to add ctors memory region descriptor.\n");
	}

	// data
	rc = ukplat_memregion_list_insert(
		&bi->mrds,
		&(struct ukplat_memregion_desc){
			.vbase = __DATA,
			.pbase = __DATA,
			.len   = (size_t) __EDATA - (size_t) __DATA,
			.type  = UKPLAT_MEMRT_RESERVED,
			.flags = UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE,
		});
	if (unlikely(rc < 0))
		uk_pr_err("Failed to add data memory region descriptor.\n");

	// bss
	rc = ukplat_memregion_list_insert(
		&bi->mrds,
		&(struct ukplat_memregion_desc){
			.vbase = __BSS_START,
			.pbase = __BSS_START,
			.len   = (size_t) __END - (size_t) __BSS_START,
			.type  = UKPLAT_MEMRT_RESERVED,
			.flags = UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE,
		});
	if (unlikely(rc < 0))
		uk_pr_err("Failed to add bss memory region descriptor.\n");

	// heap
	rc = ukplat_memregion_list_insert(
		&bi->mrds,
		&(struct ukplat_memregion_desc){
			.vbase = __END,
			.pbase = __END,
			.len   = (size_t) ((MMIO_BASE/2 - 1) - (size_t) __END) / __PAGE_SIZE * __PAGE_SIZE,
			.type  = UKPLAT_MEMRT_FREE,
			.flags = UKPLAT_MEMRF_READ |
					UKPLAT_MEMRF_WRITE |
					UKPLAT_MEMRF_MAP,
		});
	if (unlikely(rc < 0))
		uk_pr_err("Failed to add heap memory region descriptor.\n");
}

void _libraspiplat_entry(uint64_t low0, uint64_t hi0, uint64_t low1, uint64_t hi1)
{
	__libraspiplat_mem_init();

	ukplat_irq_init();

	if (hi0 == hi1) {
		assembly_entry = ((hi0 << 32)&0xFFFFFFFF00000000) | (low0&0xFFFFFFFF);
	} else {
		assembly_entry = ((hi1 << 32)&0xFFFFFFFF00000000) | (low1&0xFFFFFFFF);
	}

    _libraspiplat_init_console();
	
	hardware_init_done = get_system_timer();

	/*
	 * Enter Unikraft
	 */
	ukplat_entry(0, 0);
}
