#include <foundation.h>
#include <kernel/task.h>

enum fault_type {
	USAGE_FAULT	= (1 << 3),
	BUS_FAULT	= (1 << 1),
	MM_FAULT	= (1 << 0),
};

enum usage_fault_status {
	UNDEFINSTR	= (1 << 16), /* Undefined instruction */
	INVSTATE	= (1 << 17), /* Invalid state of EPSR */
	INVPC		= (1 << 18), /* Invalid PC load by EXC_RETURN */
	NOCP		= (1 << 19), /* No coprocessor */
	UNALIGNED	= (1 << 24), /* Unaligned access */
	DIVBYZERO	= (1 << 25), /* Divide by zero */
};

enum bus_fault_status {
	IBUSERR		= (1 <<  8), /* Instruction bus error */
	PRECISERR	= (1 <<  9), /* Precise data bus error */
	IMPRECISERR	= (1 << 10), /* Imprecise data bus error */
	UNSTKERR	= (1 << 11), /* Fault on unstacking for a return from exception */
	STKERR		= (1 << 12), /* Fault on stacking for exception */
	BFARVALID	= (1 << 15), /* Address Register valid flag */
};

enum mm_fault_status {
	IACCVIOL	= (1 << 0), /* Instruction access violation flag */
	DACCVIOL	= (1 << 1), /* Data access violation flag */
	MUNSTKERR	= (1 << 3), /* Fault on unstacking for a return from exception */
	MSTKERR		= (1 << 4), /* Fault on stacking for exception */
	MMARVALID	= (1 << 7), /* Address register valid flag */
};

static inline void print_context(unsigned int *regs)
{
	unsigned int i;

	for (i = 0; i < NR_CONTEXT; i++)
		printk("  0x%08x <%08x>\n", &regs[i], regs[i]);
}

static inline void print_task_status(struct task *task)
{
	printk( "  task->sp         0x%08x\n"
		"  task->base       0x%08x\n"
		"  task->heap       0x%08x\n"
		"  task->kernel     0x%08x\n"
		"  task->kernel->sp 0x%08x\n"
		"  task->state      0x%08x\n"
		"  task->irqflag    0x%08x\n"
		"  task->addr       0x%08x\n"
		"  task             0x%08x\n"
		"  task->parent     0x%08x - 0x%08x\n",
		task->mm.sp, task->mm.base, task->mm.heap,
		task->mm.kernel.base, task->mm.kernel.sp, task->state,
		task->irqflag, task->addr, task,
		task->parent, task->parent->addr);
}

static inline void print_kernel_status(unsigned int *sp, unsigned int lr,
		unsigned int psr)
{
	printk( "  Kernel SP        0x%08x\n"
		"  Stacked PSR      0x%08x\n"
		"  Stacked PC       0x%08x\n"
		"  Stacked LR       0x%08x\n"
		"  Current LR       0x%08x\n"
		"  Current PSR      0x%08x(vector number:%d)\n",
		sp, sp[7], sp[6], sp[5], lr, psr, psr & 0x1ff);
}

static inline void print_user_status(unsigned int *sp)
{
	printk( "  User SP          0x%08x\n"
		"  Stacked PSR      0x%08x\n"
		"  Stacked PC       0x%08x\n"
		"  Stacked LR       0x%08x\n",
		sp, sp[7], sp[6], sp[5]);
}

static inline void busfault()
{
	if (SCB_CFSR & IBUSERR)
		error("  Instruction bus error");
	if (SCB_CFSR & PRECISERR)
		error("  Precise data bus error");
	if (SCB_CFSR & IMPRECISERR)
		error("  Imprecise data bus error");
	if (SCB_CFSR & UNSTKERR)
		error("  on unstacking for a return from exception");
	if (SCB_CFSR & STKERR)
		error("  on stacking for exception");

	if (SCB_CFSR & BFARVALID)
		error("Fault address:\n  0x%08x", SCB_BFAR);
}

static inline void usagefault()
{
	if (SCB_CFSR & UNDEFINSTR)
		error("  Undefined instruction");
	if (SCB_CFSR & INVSTATE)
		error("  Invalid state of EPSR");
	if (SCB_CFSR & INVPC)
		error("  Invalid PC load by EXC_RETURN");
	if (SCB_CFSR & NOCP)
		error("  No coprocessor");
	if (SCB_CFSR & UNALIGNED)
		error("  Unalignd access");
	if (SCB_CFSR & DIVBYZERO)
		error("  Divide by zero");
}

void __attribute__((naked)) isr_fault()
{
	/* disable interrupts */
	/* save registers */

	unsigned int sp, lr, psr, usp;

	sp  = __get_sp();
	psr = __get_psr();
	lr  = __get_lr();
	usp = __get_usp();

	error("\nFault type: %x <%08x>\n"
		"  (%x=Usage fault, %x=Bus fault, %x=Memory management fault)",
		SCB_SHCSR & 0xfff, SCB_SHCSR, USAGE_FAULT, BUS_FAULT, MM_FAULT);

	error("Fault source: ");
	busfault();
	usagefault();

	printk("\nKernel Space\n");
	print_kernel_status((unsigned int *)sp, lr, psr);

	printk("\nUser Space\n");
	print_user_status((unsigned int *)usp);

	printk("\nTask Status\n");
	print_task_status(current);

	printk("\nCurrent Context\n");
	print_context((unsigned int *)usp);

	printk("\nSCB_ICSR  0x%08x\n"
		"SCB_CFSR  0x%08x\n"
		"SCB_HFSR  0x%08x\n"
		"SCB_MMFAR 0x%08x\n"
		"SCB_BFAR  0x%08x\n",
		SCB_ICSR, SCB_CFSR, SCB_HFSR, SCB_MMFAR, SCB_BFAR);

	/* led for debugging */
#ifdef LED_DEBUG
	__turn_port_clock((reg_t *)PORTD, ON);
	SET_PORT_PIN(PORTD, 2, PIN_OUTPUT_50MHZ);
	unsigned int j;
	while (1) {
		write_port((reg_t *)PORTD, scan_port((reg_t *)PORTD) ^ 4);

		for (i = 100; i; i--) {
			for (j = 10; j; j--) {
				__asm__ __volatile__(
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						"nop		\n\t"
						::: "memory");
			}
		}
	}
#endif

	while (1);

	/* restore registers */
	/* enable interrupts */
}
