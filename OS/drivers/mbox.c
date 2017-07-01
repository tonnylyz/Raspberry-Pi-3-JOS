/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <mbox.h>
#include <types.h>

extern void set_ptr(u64, u64);
extern u64  get_ptr(u64);
#define mmio_read get_ptr
#define mmio_write set_ptr

#define MBOX_FULL		0x80000000
#define	MBOX_EMPTY		0x40000000

static u64 mbox_base = MBOX_BASE;

void mbox_set_base(u64 base)
{
	mbox_base = base;
}

u32 mbox_read(u8 channel)
{
	while(1)
	{
		while(mmio_read(mbox_base + MBOX_STATUS) & MBOX_EMPTY);

		u32 data = mmio_read(mbox_base + MBOX_READ);
		u8 read_channel = (u8)(data & 0xf);
		if(read_channel == channel)
			return (data & 0xfffffff0);
	}
}

void mbox_write(u8 channel, u32 data)
{
	while(mmio_read(mbox_base + MBOX_STATUS) & MBOX_FULL);
	mmio_write(mbox_base + MBOX_WRITE, (data & 0xfffffff0) | (u32)(channel & 0xf));
}

