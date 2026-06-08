#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#define OPTEE_SMC_CALLS_UID 0xB2000000
#define OPTEE_SMC_CALLS_SHM 0xB2000007
#define OPTEE_SMC_CALLS_REV 0xB2000001

#define OPTEE_SMC_CALL_WITH_ARG         0x32000004
#define OPTEE_MSG_CMD_MY_TOTP           12
#define OPTEE_MSG_CMD_RSA_DEC           13
#define OPTEE_MSG_ATTR_TYPE_VALUE_INOUT 0x3
#define OPTEE_SMC_RETURN_OK             0x0

static const unsigned char ct_bin[] = {
  0xa4, 0x36, 0x95, 0xfc, 0x2d, 0x6c, 0x74, 0xcd, 0xe3, 0xe4, 0xb6, 0x56,
  0x24, 0xde, 0x46, 0x6a, 0xf6, 0x6b, 0xbf, 0x6e, 0x3e, 0x7d, 0xde, 0xad,
  0x02, 0xd0, 0xab, 0xd0, 0x8c, 0xd9, 0x12, 0x82, 0xf7, 0xc6, 0x32, 0xc1,
  0x3f, 0x33, 0xe1, 0x88, 0x09, 0xe2, 0x61, 0x62, 0x1b, 0xe6, 0x32, 0x93,
  0x4d, 0x30, 0x6b, 0x3c, 0x27, 0x53, 0x3c, 0x33, 0x62, 0xbd, 0x73, 0x7a,
  0xc7, 0x3d, 0xc7, 0x81, 0x63, 0x20, 0x42, 0x63, 0x3f, 0xd5, 0x97, 0x13,
  0x74, 0xa5, 0xd9, 0xdb, 0xf6, 0x48, 0x46, 0xc5, 0x71, 0x56, 0x5a, 0x70,
  0x38, 0xd1, 0xb2, 0x25, 0xdd, 0x97, 0x2b, 0x65, 0xcd, 0x55, 0x87, 0xd5,
  0x81, 0x7b, 0x74, 0x72, 0x09, 0x50, 0x7f, 0x9e, 0x62, 0xb0, 0xd2, 0x65,
  0x70, 0x63, 0xc1, 0x36, 0x62, 0xcf, 0x79, 0x01, 0xdb, 0x6d, 0x84, 0xbd,
  0xa3, 0xb6, 0xb2, 0x61, 0xe8, 0xea, 0x57, 0xc4, 0x02, 0xc6, 0x06, 0x63,
  0x42, 0xf7, 0xad, 0x15, 0x9e, 0xee, 0xa6, 0xc4, 0xd0, 0xbd, 0x1b, 0x87,
  0xa4, 0x1c, 0xa9, 0x94, 0x70, 0xc4, 0x07, 0x92, 0x7f, 0x7b, 0x9e, 0xba,
  0xd2, 0x8f, 0x9b, 0xe2, 0x47, 0xb1, 0x44, 0x7f, 0x8c, 0xf9, 0x00, 0xa8,
  0xca, 0xaa, 0x1f, 0x67, 0xb9, 0xe4, 0x78, 0x8b, 0xb7, 0xe1, 0x28, 0x55,
  0x08, 0x65, 0xe8, 0x4b, 0xf9, 0x78, 0x3f, 0x84, 0xfa, 0xaa, 0xa3, 0x8a,
  0x26, 0x44, 0xc4, 0xfd, 0xdd, 0x76, 0xa9, 0x3c, 0x46, 0x1f, 0x51, 0xa1,
  0x54, 0xbd, 0x52, 0x89, 0xba, 0x65, 0x83, 0xcb, 0x36, 0xef, 0x5f, 0x3f,
  0x0b, 0xdf, 0x13, 0xba, 0xdb, 0xd7, 0x8d, 0x0a, 0x94, 0x39, 0xcc, 0x11,
  0xdf, 0xd5, 0x79, 0xb6, 0xd8, 0x17, 0xbc, 0x07, 0x0b, 0x7f, 0x5f, 0x0d,
  0x65, 0x80, 0x8f, 0x31, 0x99, 0xd3, 0x0e, 0x8f, 0x4a, 0xee, 0x63, 0x0b,
  0x1c, 0x80, 0x3e, 0x13
};

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

#define OPTEE_MSG_CMD_RSA_DEC  13
  /* public key (e=17, n=3233) — NOT secret, fine to keep in NW */
static uint64_t modexp(uint64_t b, uint64_t e, uint64_t m){ unsigned __int128 r=1,x=b%m;
	for(;e;e>>=1){ if(e&1) r=(r*x)%m; x=(x*x)%m; } return (uint64_t)r; }

void rsa_demo(void)
{
	volatile struct msg_arg *arg = (void *)0x08000000;  /* reserved SHM   */
	volatile uint8_t *ct = (uint8_t *)0x08001000;        /* ciphertext in  */
	volatile uint8_t *pt = (uint8_t *)0x08002000;        /* plaintext out  */

	for (int i = 0; i < 256; i++)          /* 1. ciphertext -> SHM */
		ct[i] = ct_bin[i];

	memset((void *)arg, 0, sizeof(*arg) + sizeof(struct msg_param));
	arg->cmd            = OPTEE_MSG_CMD_RSA_DEC;     /* 2. build msg arg   */
	arg->num_params     = 1;
	arg->params[0].attr = OPTEE_MSG_ATTR_TYPE_VALUE_INOUT;
	arg->params[0].u.value.a = 256;                 /* ciphertext length  */
	__asm__ volatile("dmb ish" ::: "memory");

	uint64_t o0, o1, o2, o3, pa = 0x08000000;        /* 3. yielding SMC    */
	smc_call(OPTEE_SMC_CALL_WITH_ARG, pa >> 32,
			(void *)(pa & 0xffffffff), 0, 0, &o0, &o1, &o2, &o3);

	__asm__ volatile("dmb ish" ::: "memory");        /* 4. read result     */
	if (o0 == OPTEE_SMC_RETURN_OK && arg->ret == 0) {
		uint64_t len = arg->params[0].u.value.a;
		printf("RSA OK (%lu bytes): ", (unsigned long)len);
		for (uint64_t i = 0; i < len; i++) putchar(pt[i]);
		putchar('\n');                           /* expect: hello from unikraft */
	} else {
		printf("RSA failed: a0=0x%lx tee_ret=0x%x\n",
				(unsigned long)o0, arg->ret);
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
	rsa_demo();
	totp_via_std();
	check_optee_alive();
	check_optee_alive();

	return 0;
}
