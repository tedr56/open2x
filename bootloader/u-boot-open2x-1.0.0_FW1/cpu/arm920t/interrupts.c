/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * (C) Copyright 2002
 * Gary Jennejohn, DENX Software Engineering, <gj@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <arm920t.h>
/*
#if defined(CONFIG_S3C2400)
#include <s3c2400.h>
#elif defined(CONFIG_S3C2410)
#include <s3c2410.h>
#elif defined(CONFIG_MMSP20)
*/
#include <mmsp20.h>
/*
#endif
*/
#include <asm/proc-armv/ptrace.h>

extern void reset_cpu(ulong addr);
unsigned int timer_load_val = 0;

/* macro to read the 16 bit timer */

#if defined(CONFIG_MMSP20) 
static inline ulong READ_TIMER(void)
{
	MMSP20_TIMER_WATCHDOG * const timers = MMSP20_GetBase_TIMERS();
	
	return (timers->TCOUNT); 
}
#else
static inline ulong READ_TIMER(void)
{
	S3C24X0_TIMERS * const timers = S3C24X0_GetBase_TIMERS();

	return (timers->TCNTO4 & 0xffff);
}
#endif

#ifdef CONFIG_USE_IRQ
/* enable IRQ interrupts */
void enable_interrupts (void)
{
	unsigned long temp;
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "bic %0, %0, #0x80\n"
			     "msr cpsr_c, %0"
			     : "=r" (temp)
			     :
			     : "memory");
}


/*
 * disable IRQ/FIQ interrupts
 * returns true if interrupts had been enabled before we disabled them
 */
int disable_interrupts (void)
{
	unsigned long old,temp;
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "orr %1, %0, #0xc0\n"
			     "msr cpsr_c, %1"
			     : "=r" (old), "=r" (temp)
			     :
			     : "memory");
	return (old & 0x80) == 0;
}
#else
void enable_interrupts (void)
{
	return;
}
int disable_interrupts (void)
{
	return 0;
}
#endif


void bad_mode (void)
{
	panic ("Resetting CPU ...\n");
	reset_cpu (0);
}

void show_regs (struct pt_regs *regs)
{
	unsigned long flags;
	const char *processor_modes[] = {
	"USER_26",	"FIQ_26",	"IRQ_26",	"SVC_26",
	"UK4_26",	"UK5_26",	"UK6_26",	"UK7_26",
	"UK8_26",	"UK9_26",	"UK10_26",	"UK11_26",
	"UK12_26",	"UK13_26",	"UK14_26",	"UK15_26",
	"USER_32",	"FIQ_32",	"IRQ_32",	"SVC_32",
	"UK4_32",	"UK5_32",	"UK6_32",	"ABT_32",
	"UK8_32",	"UK9_32",	"UK10_32",	"UND_32",
	"UK12_32",	"UK13_32",	"UK14_32",	"SYS_32",
	};

	flags = condition_codes (regs);

	printf ("pc : [<%08lx>]    lr : [<%08lx>]\n"
		"sp : %08lx  ip : %08lx  fp : %08lx\n",
		instruction_pointer (regs),
		regs->ARM_lr, regs->ARM_sp, regs->ARM_ip, regs->ARM_fp);
	printf ("r10: %08lx  r9 : %08lx  r8 : %08lx\n",
		regs->ARM_r10, regs->ARM_r9, regs->ARM_r8);
	printf ("r7 : %08lx  r6 : %08lx  r5 : %08lx  r4 : %08lx\n",
		regs->ARM_r7, regs->ARM_r6, regs->ARM_r5, regs->ARM_r4);
	printf ("r3 : %08lx  r2 : %08lx  r1 : %08lx  r0 : %08lx\n",
		regs->ARM_r3, regs->ARM_r2, regs->ARM_r1, regs->ARM_r0);
	printf ("Flags: %c%c%c%c",
		flags & CC_N_BIT ? 'N' : 'n',
		flags & CC_Z_BIT ? 'Z' : 'z',
		flags & CC_C_BIT ? 'C' : 'c', flags & CC_V_BIT ? 'V' : 'v');
	printf ("  IRQs %s  FIQs %s  Mode %s%s\n",
		interrupts_enabled (regs) ? "on" : "off",
		fast_interrupts_enabled (regs) ? "on" : "off",
		processor_modes[processor_mode (regs)],
		thumb_mode (regs) ? " (T)" : "");
}

void do_undefined_instruction (struct pt_regs *pt_regs)
{
	printf ("undefined instruction\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_software_interrupt (struct pt_regs *pt_regs)
{
	printf ("software interrupt\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_prefetch_abort (struct pt_regs *pt_regs)
{
	printf ("prefetch abort\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_data_abort (struct pt_regs *pt_regs)
{
	printf ("data abort\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_not_used (struct pt_regs *pt_regs)
{
	printf ("not used\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_fiq (struct pt_regs *pt_regs)
{
	printf ("fast interrupt request\n");
	show_regs (pt_regs);
	bad_mode ();
}

void do_irq (struct pt_regs *pt_regs)
{
	printf ("interrupt request\n");
	show_regs (pt_regs);
	bad_mode ();
}


static ulong timestamp;
static ulong lastdec;

#if defined(CONFIG_MMSP20)

/* timer configration */
int interrupt_init (void)
{
	MMSP20_TIMER_WATCHDOG * const timers = MMSP20_GetBase_TIMERS();

	/* */
	/* */
	if (timer_load_val == 0)
	{
		/* 10ms configration */
		timer_load_val = 0x12000;
	}
	/* 10ms timer configration */
	lastdec = timer_load_val;
	/* Timer stop */
	timers->TCONTROL = (timers->TCONTROL & ~0x01) | 0x01;
	timestamp = 0;
	return (0);
}

#else
int interrupt_init (void)
{
	S3C24X0_TIMERS * const timers = S3C24X0_GetBase_TIMERS();

	/* use PWM Timer 4 because it has no output */
	/* prescaler for Timer 4 is 16 */
	timers->TCFG0 = 0x0f00;
	if (timer_load_val == 0)
	{
		/*
		 * for 10 ms clock period @ PCLK with 4 bit divider = 1/2
		 * (default) and prescaler = 16. Should be 10390
		 * @33.25MHz and 15625 @ 50 MHz
		 */
		timer_load_val = get_PCLK()/(2 * 16 * 100);
	}
	/* load value for 10 ms timeout */
	lastdec = timers->TCNTB4 = timer_load_val;
	/* auto load, manual update of Timer 4 */
	timers->TCON = (timers->TCON & ~0x0700000) | 0x600000;
	/* auto load, start Timer 4 */
	timers->TCON = (timers->TCON & ~0x0700000) | 0x500000;
	timestamp = 0;

	return (0);
}
#endif
/*
 * timer without interrupts
 */

void reset_timer (void)
{
	reset_timer_masked ();
}
#if defined (CONFIG_MMSP20)
ulong get_timer (ulong base)
{
	ulong now = READ_TIMER();
	
	while(base) base-- ;

	return now;
}
#else
ulong get_timer (ulong base)
{
	return get_timer_masked () - base;
}
#endif
void set_timer (ulong t)
{
	timestamp = t;
}

void udelay (unsigned long usec)
{
	ulong tmo;


#if defined (CONFIG_MMSP20)
	ulong ReadTime;	

/* 	printf("%lu usec\n", usec);	*/
	
	if (usec < 10) usec = 10;

	tmo = get_timer(10) + ((usec / 10) * 29);

	if (ReadTime < tmo)
		while (get_timer(10) < tmo);
	else  {
	   while (get_timer(10) < 0xffffffff && get_timer(10) > (tmo + 73728));
	   while (get_timer(10) < tmo);
	}
#else
	tmo = usec / 1000;
	tmo *= (timer_load_val * 100);
	tmo /= 1000;

	tmo += get_timer (0);

	while (get_timer_masked () < tmo)
		/*NOP*/;
#endif
}

void reset_timer_masked (void)
{
	/* reset time */
	lastdec = READ_TIMER();
	timestamp = 0;
}

ulong get_timer_masked (void)
{
	ulong now = READ_TIMER();

	if (lastdec >= now) {
		/* normal mode */
		timestamp += lastdec - now;
	} else {
		/* we have an overflow ... */
		timestamp += lastdec + timer_load_val - now;
	}
	lastdec = now;
	return timestamp;
}

void udelay_masked (unsigned long usec)
{
	ulong tmo;

	tmo = usec / 1000;
	tmo *= (timer_load_val * 100);
	tmo /= 1000;

	reset_timer_masked ();

	while (get_timer_masked () < tmo)
		/*NOP*/;
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk (void)
{
	ulong tbclk;

#if defined(CONFIG_SMDK2400) || defined(CONFIG_TRAB) 
	tbclk = timer_load_val * 100;
#elif defined(CONFIG_SMDK2410) || defined(CONFIG_VCMA9) || defined(CONFIG_EPLAY) || defined(CONFIG_MMSP2DTK)
	tbclk = CFG_HZ;
#else
#	error "tbclk not configured"
#endif

	return tbclk;
}
