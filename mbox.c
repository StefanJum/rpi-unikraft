/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *               https://github.com/bztsrc/raspi3-tutorial
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include <raspi/mbox.h>
#include <raspi/sysregs.h>

/* mailbox message buffer */
volatile unsigned int  __attribute__((aligned(16))) mbox[MBOX_BUFFER_LENGTH];

extern void clean_and_invalidate_dcache_range(void *start, unsigned long length);
extern void invalidate_dcache_range(void *start, unsigned long length);

#define VIDEOCORE_MBOX  (MMIO_BASE+0x0000B880)
#define MBOX_READ       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x0))
#define MBOX_POLL       ((volatile unsigned int*)(VIDEOCORE_MBOX+0x10))
#define MBOX_SENDER     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x14))
#define MBOX_STATUS     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x18))
#define MBOX_CONFIG     ((volatile unsigned int*)(VIDEOCORE_MBOX+0x1C))
#define MBOX_WRITE      ((volatile unsigned int*)(VIDEOCORE_MBOX+0x20))
#define MBOX_RESPONSE   0x80000000
#define MBOX_FULL       0x80000000
#define MBOX_EMPTY      0x40000000

/**
 * Make a mailbox call. Returns 0 on failure, non-zero on success
 */
int mbox_call(unsigned char ch)
{
    // Calculate the address to send in the mailbox
    unsigned int r = (((unsigned int)((unsigned long)&mbox)&~0xF) | (ch&0xF));

    // Ensure data coherency by cleaning and invalidating the data cache for the address
    clean_and_invalidate_dcache_range(&mbox, MBOX_BUFFER_LENGTH);

    // Wait until we can write to the mailbox
    do { asm volatile("nop"); } while (*MBOX_STATUS & MBOX_FULL);

    // Write the address of our message to the mailbox with channel identifier
    *MBOX_WRITE = r;

    // Now wait for the response
    while (1) {
        // Is there a response?
        do { asm volatile("nop"); } while (*MBOX_STATUS & MBOX_EMPTY);

        // Is it a response to our message?
        if (r == *MBOX_READ) {
            // Is it a valid successful response?

            invalidate_dcache_range(&mbox, MBOX_BUFFER_LENGTH);
            return mbox[1] == MBOX_RESPONSE;
        }
    }
    return 0;
}