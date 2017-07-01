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

#include <timer.h>
#include <types.h>
#include <error.h>
#include <mmu.h>

extern void set_ptr(u64, u64);
extern u64  get_ptr(u64);
#define mmio_read get_ptr
#define mmio_write set_ptr

#define TIMER_BASE		KADDR(0x3F003000)
#define TIMER_CLO		0x4

#define TIMER_C3		0x18
#define TIMER_INT		KADDR(0x3F00B210)

static u64 timer_base = TIMER_BASE;

void timer_set_base(u32 base)
{
	timer_base = base;
}

int usleep(useconds_t usec)
{
	struct timer_wait tw = register_timer(usec);
	while(!compare_timer(tw));
	return 0;	
}

struct timer_wait register_timer(useconds_t usec)
{
	struct timer_wait tw;
	tw.rollover = 0;
	tw.trigger_value = 0;

	if(usec < 0)
	{
		//errno = EINVAL;
		return tw;
	}
	u32 cur_timer = mmio_read(timer_base + TIMER_CLO);
	u32 trig = cur_timer + (u32)usec;

	if(cur_timer == 0)
		trig = 0;

	tw.trigger_value = trig;
	if(trig > cur_timer)
		tw.rollover = 0;
	else
		tw.rollover = 1;
	return tw;
}

int compare_timer(struct timer_wait tw)
{
	u32 cur_timer = mmio_read(timer_base + TIMER_CLO);

	if(tw.trigger_value == 0)
		return 1;

	if(cur_timer < tw.trigger_value)
	{
		if(tw.rollover)
			tw.rollover = 0;
	}
	else if(!tw.rollover)
		return 1;

	return 0;
}

void setup_clock_int(u32 ns) {
    static u32 last;
    if (ns != 0) {
        last = ns;
    }
	mmio_write(TIMER_INT, 1 << 3);
	mmio_write(timer_base + TIMER_C3, last + mmio_read(timer_base + TIMER_CLO));
}

void clear_clock_int() {
    mmio_write(timer_base, 1 << 3);
}