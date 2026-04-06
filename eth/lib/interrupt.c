#include <uspienv/interrupt.h>
#include <uspienv/synchronize.h>
#include <uk/assert.h>
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

void InterruptSystemEnableIRQ (unsigned nIRQ)
{
	DataMemBarrier ();

	UK_ASSERT (nIRQ < IRQ_LINES);

	write32 (ARM_IC_IRQS_ENABLE (nIRQ), ARM_IRQ_MASK (nIRQ));

	DataMemBarrier ();
}
