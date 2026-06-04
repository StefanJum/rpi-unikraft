#include <stdint.h>
#include <stdio.h>

#define OPTEE_SMC_CALLS_UID 0xB2000001

// OP-TEE UID (returns in x0..x3): 384FB3E0 E7F811E3 AF630002 A5D5C51B
// See optee_os/core/arch/arm/include/sm/optee_smc.h

static inline void smc_call(uint64_t fid,
                            uint64_t a1, uint64_t a2,
                            uint64_t a3, uint64_t a4,
                            uint64_t *o0, uint64_t *o1,
                            uint64_t *o2, uint64_t *o3)
{
    register uint64_t r0 __asm__("x0") = fid;
    register uint64_t r1 __asm__("x1") = a1;
    register uint64_t r2 __asm__("x2") = a2;
    register uint64_t r3 __asm__("x3") = a3;
    register uint64_t r4 __asm__("x4") = a4;

    __asm__ volatile("smc #0"
                     : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3)
                     : "r"(r4)
                     : "x5", "x6", "x7", "x8", "memory");

    if (o0) *o0 = r0;
    if (o1) *o1 = r1;
    if (o2) *o2 = r2;
    if (o3) *o3 = r3;
}

void check_optee_alive(void)
{
    uint64_t o0, o1, o2, o3;

    smc_call(OPTEE_SMC_CALLS_UID, 0, 0, 0, 0, &o0, &o1, &o2, &o3);

    printf("OP-TEE UID: %08lx %08lx %08lx %08lx\n",
           (unsigned long)o0, (unsigned long)o1,
           (unsigned long)o2, (unsigned long)o3);

    // Expected: 384FB3E0-E7F8-11E3-AF63-0002A5D5C51B
}

int main()
{
	uint64_t el;
	asm volatile("mrs %0, CurrentEL" : "=r"(el));
	printf("EL: %lu\n", el);
	check_optee_alive();
	check_optee_alive();

	return 0;
}
