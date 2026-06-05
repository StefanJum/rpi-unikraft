#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define OPTEE_SMC_CALLS_UID 0xB2000000
#define OPTEE_SMC_CALLS_SHM 0xB2000007
#define OPTEE_SMC_CALLS_REV 0xB2000001

#define OPTEE_SMC_CALL_WITH_ARG         0x32000004
#define OPTEE_MSG_CMD_MY_TOTP           12
#define OPTEE_MSG_ATTR_TYPE_VALUE_INOUT 0x3
#define OPTEE_SMC_RETURN_OK             0x0

struct msg_param {
	uint64_t attr;
	union { struct { uint64_t a, b, c; } value; uint8_t octets[24]; } u;
};
struct msg_arg {
	uint32_t cmd, func, session, cancel_id, pad, ret, ret_origin, num_params;
	struct msg_param params[];
};


// OP-TEE UID (returns in x0..x3): 384FB3E0 E7F811E3 AF630002 A5D5C51B
// See optee_os/core/arch/arm/include/sm/optee_smc.h

static inline void smc_call(uint64_t fid,
                            uint64_t a1, void * a2,
                            uint64_t a3, uint64_t a4,
                            uint64_t *o0, uint64_t *o1,
                            uint64_t *o2, uint64_t *o3)
{
    register uint64_t r0 __asm__("x0") = fid;
    register uint64_t r1 __asm__("x1") = a1;
    register uint64_t r2 __asm__("x2") = (uint64_t)a2;
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

void totp_via_std(void)
{
	volatile struct msg_arg *arg = (void *)0x08000000;   /* reserved SHM */

	memset((void *)arg, 0, sizeof(*arg) + sizeof(struct msg_param));
	printf("MEMSET DONE\n");

	while (1) {

		arg->cmd        = OPTEE_MSG_CMD_MY_TOTP;
		arg->num_params = 1;
		arg->params[0].attr      = OPTEE_MSG_ATTR_TYPE_VALUE_INOUT;
		arg->params[0].u.value.a = (uint64_t)time(NULL);     /* time in */
		asm volatile("dmb ish" ::: "memory");

		uint64_t pa = 0x08000000, o0, o1, o2, o3;
		 /*a1 = upper32 of PA, a2 = lower32 of PA, a3 = 0 (arg is in predefined SHM) */
		smc_call(OPTEE_SMC_CALL_WITH_ARG, pa >> 32, (void *)(pa & 0xffffffff),
				0, 0, &o0, &o1, &o2, &o3);

		/*if (o0 == OPTEE_SMC_RETURN_OK)*/
			printf("OTP = %lu (tee ret=0x%x)\n",
					(unsigned long)arg->params[0].u.value.a, arg->ret);
		/*else*/
			/*printf("std call failed: a0=0x%lx\n", (unsigned long)o0);*/

		sleep(5);
	}
}

void check_optee_alive(void)
{
    uint64_t o0, o1, o2, o3;
    uint64_t otp = 1000000;
    volatile char *a = (char *)0x08000010;
    printf("SHM %hhx\n", *a);
    smc_call(OPTEE_SMC_CALLS_SHM, 0, 0, 0, 0, &o0, &o1, &o2, &o3);

    printf("OP-TEE UID: %08lx %08lx %08lx %08lx\n",
	   (unsigned long)o0, (unsigned long)o1,
	   (unsigned long)o2, (unsigned long)o3);

    while (1) {
	    smc_call(OPTEE_SMC_CALLS_UID, time(NULL), 0, 0, 0, &o0, &o1, &o2, &o3);

	    if (otp != o0) {
		    otp = o0;

		    printf("OP-TEE OTP: %ld\n", (unsigned long)o0);
		    printf("OP-TEE UID: %08lx %08lx %08lx %08lx\n",
			   (unsigned long)o0, (unsigned long)o1,
			   (unsigned long)o2, (unsigned long)o3);
	    }

	    /*sleep(30);*/
    }
    // Expected: 384FB3E0-E7F8-11E3-AF63-0002A5D5C51B
    /*smc_call(OPTEE_SMC_CALLS_REV, 0, 0, 0, 0, &o0, &o1, &o2, &o3);*/

    /*printf("OP-TEE UID: %08lx %08lx %08lx %08lx\n",*/
           /*(unsigned long)o0, (unsigned long)o1,*/
           /*(unsigned long)o2, (unsigned long)o3);*/
}

int main()
{
	uint64_t el;
	asm volatile("mrs %0, CurrentEL" : "=r"(el));
	printf("EL: %lu\n", el);
	totp_via_std();
	check_optee_alive();
	check_optee_alive();

	return 0;
}
