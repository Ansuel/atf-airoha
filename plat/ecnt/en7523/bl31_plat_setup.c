/*
 * Copyright (c) 2013-2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/common_def.h>
#include <plat/common/platform.h>
#include <drivers/auth/auth_mod.h>

#include <ecnt_plat_common.h>
#include <plat_private.h>
#include <ecnt_scu.h>
#include <asm/tc3162.h>

extern void bl31_db_entrypoint(void);
extern int console_ecnt_register(uintptr_t baseaddr, console_t *console);
#ifdef TCSUPPORT_CPU_EN7581
extern int efuse_check_eco(void);
#endif
extern uint32_t uartDisable;
static entry_point_info_t bl32_ep_info;
static entry_point_info_t bl33_ep_info;
static console_t console;
#if defined(TCSUPPORT_CPU_EN7581)
#define L2C_SRAM_CONFIG
#endif
#ifdef L2C_SRAM_CONFIG
//#define L2C_SRAM_VERIFY
#endif

#define TZPC_MODULE_ATTR_BASE	0x1FBE2E04

#define readReg32(reg) (*(volatile unsigned int *)(reg))
#define writeReg32(reg,data) (*(volatile unsigned int *)(reg)=data)
#define S_2G				(0x80000000)
#define S_8G				(0x200000000)

#if 1
#define CSR_MCUSYS_BASE     (0x1EFBE000)
#define CSR_MP0_AXI_CONFIG  (CSR_MCUSYS_BASE+0x002C)
#define CSR_L2C_CFG_MP0     (CSR_MCUSYS_BASE+0x07F0)
#define S_128K              (0x20000)
#define S_256K              (0x40000)
#define S_512K              (0x80000)

enum l2c_type {
    l2t_half_l2c_half_sram=0,   /* 256_l2c_256_sram for 7581,   128_l2c_128_sram for 7523 */
    l2t_full_sram,              /* 512_sram for 7581,           256_sram for 7523 */
    l2t_full_l2c                /* 512_l2c for 7581,            256_l2c for 7523 */
};

/* cache APIs are defined in bootloader/ATF/arm-trusted-firmware-2.3/lib/cpus/aarch64/cortex_a53.S */
extern void ca53_disable_flush_caches(void);
extern void ca53_enable_icache(void);

/* when CSR_L2C_CFG_MP0[0]==0, CSR_L2C_CFG_MP0[11:8]==0,1,2 for 128k,256k,512k L2 cache respectively.
 * when CSR_L2C_CFG_MP0[0]==1, CSR_L2C_CFG_MP0[11:8]==0,1,2 for 128k,256k,512k L2 sram respectively. */
#if (defined(TCSUPPORT_CPU_EN7581)) && (!defined(TCSUPPORT_CPU_AN7583))
static unsigned int l2c_config[] = {0x101/*256K_L2C+256K_SRAM*/, 0x301/*512K_SRAM*/, 0x300/*512K_L2C*/};
static char *l2c_type_name[] = {"256K_L2C+256K_SRAM", "512K_SRAM", "512K_L2C"};
static unsigned int t_size[] = {S_256K, S_512K, 0};
#else
static unsigned int l2c_config[] = {0x001/*128K_L2C+128K_SRAM*/, 0x101/*256K_SRAM*/, 0x100/*256K_L2C*/};
static char *l2c_type_name[] = {"128K_L2C+128K_SRAM", "256K_SRAM", "256K_L2C"};
static unsigned int t_size[] = {S_128K, S_256K, 0};
#endif
#ifdef L2C_SRAM_VERIFY
#define L2C_SRAM_INTER_BASE (0x08000000)
#if (defined(TCSUPPORT_CPU_EN7581)) && (!defined(TCSUPPORT_CPU_AN7583))
#define L2C_SRAM_EXTER_BASE (0x1EF00000)
#else
#define L2C_SRAM_EXTER_BASE (0x1EFC0000)
#endif

static unsigned long t_base[] = {(unsigned long)L2C_SRAM_EXTER_BASE, (unsigned long)L2C_SRAM_INTER_BASE};
static unsigned long t_pat[] = {0xa5a5a5a5a5a5a5a5, 0x5a5a5a5a5a5a5a5a, 0xffffffffffffffff, 0x0, 0x0000ffff0000ffff};
#endif
#endif

#pragma weak calculate_dram_size

unsigned long long calculate_dram_size(void)
{    
#if defined(TCSUPPORT_CPU_EN7581)
	return S_8G;
#else
	return S_2G;
#endif
}


static void platform_setup_cpu(void)
{
	/* Set Core to Arch64 */
	#if (defined(TCSUPPORT_CPU_EN7581)) && (!defined(TCSUPPORT_CPU_AN7583))
    mmio_setbits_32((MCUCFG_BASE + 0x3C), 0xf << 12); /* for cpu0~3 */
    #else
	mmio_setbits_32((MCUCFG_BASE + 0x3C), 0x3 << 12); /* for cpu0~1 */
    #endif
}

static void set_tzpc_attr(void)
{
	unsigned int val = 0;

	/* set NPU SRAM to non secure*/
	val = readReg32(TZPC_MODULE_ATTR_BASE);
	val |= (MODULE_NPU_SRAM_OFF);
    #if defined(L2C_SRAM_CONFIG)
    #if !defined(TCSUPPORT_AUTOBENCH)
    if (isAN7581GT||isAN7566GT||isAN7581FG||isAN7551GT)
    #endif
        val |= (1<<15); /* set l2c_sram as non-secure for test */
    #endif
    //val |= (1<<11); /* set fe_sram as non-secure for test */
	/* set EFUSE to secure*/
	val &= ~(MODULE_EFUSE_OFF);
	writeReg32(TZPC_MODULE_ATTR_BASE, val);
    return;
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * the security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;


	next_image_info = (type == NON_SECURE) ? &bl33_ep_info : &bl32_ep_info;

	if (next_image_info->pc){
		if(!uartDisable){
		INFO(" next pc = 0x%x \n",next_image_info->pc );
		}
	}

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

void debug_init(void)
{
	tf_log_set_max_level(LOG_LEVEL_VERBOSE);
}

/*******************************************************************************
 * Perform any BL3-1 early platform setup. Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & EL3 in BL1) before they
 * are lost (potentially). This needs to be done before the MMU is initialized
 * so that the memory layout can be used while creating page tables.
 * BL2 has flushed this information to memory, so we are guaranteed to pick up
 * good data.
 ******************************************************************************/
void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
    SET_PARAM_HEAD(&bl33_ep_info, PARAM_EP,
	    VERSION_2, NON_SECURE | EXECUTABLE);

    bl33_ep_info.pc = BL33_BASE;
#ifdef TCSUPPORT_CPU_ARMV8_64
	ERROR("force change spsr...\n");
	bl33_ep_info.spsr = 0x3c9;
	bl33_ep_info.args.arg0 = 0;
#else
	bl33_ep_info.spsr = plat_get_spsr_for_bl33_entry();
#endif

	#ifdef TCSUPPORT_OPTEE
	SET_PARAM_HEAD(&bl32_ep_info, PARAM_EP,
	    VERSION_2, SECURE | EXECUTABLE);

	bl32_ep_info.pc = BL32_BASE;
	bl32_ep_info.spsr = 0x3c5;
	bl32_ep_info.args.arg0 = 0;
	#endif
	
	console_ecnt_register(EN7523_UART0_BASE, &console);
	debug_init();

	VERBOSE("bl31_setup\n");
}

/*******************************************************************************
 * Perform any BL3-1 platform setup code
 ******************************************************************************/
void bl31_platform_setup(void)
{
	uint32_t i = 0;
	platform_setup_cpu();

	write_cntfrq_el0(plat_get_syscnt_freq2());
	generic_delay_timer_init();

	/* setup the TZPC attr here*/ 
	set_tzpc_attr();
	
	efuse_init();

	plat_ecnt_io_setup();

	/* Initialize the gic cpu and distributor interfaces */
	plat_arm_gic_driver_init();
	plat_arm_gic_init();

	/* enable SPM mode */
	/* set bypass_cpu_spic_mode = 0 */
	mmio_clrbits_32((SPM_BASE + 0x1C), 1 << 28);
	udelay(3);

	/* For each core */
	for (i = 0; i < PLATFORM_CORE_COUNT; i++)
	{
		/* set spmc_resetpwron_config_cpu[m] = 0 */
		mmio_clrbits_32((SPM_BASE + 0x1C), 1 << (8 - i));

		/* set spmc_pwr_rst_cpu[m] = 0 */
		mmio_clrbits_32((SPM_BASE + 0x1C), 1 << (13 - i));
	}

	/* set spmc_resetpwron_config_cputop = 0 */
	mmio_clrbits_32((SPM_BASE + 0x1C), 1 << 4);

	/* set spmc_pwr_rst_cputop = 0 */
	mmio_clrbits_32((SPM_BASE + 0x1C), 1 << 9);

	/* set spmc_pwr_clk_dis_cputop = 0 */
	mmio_clrbits_32((SPM_BASE + 0x1C), 1 << 17);
}

#if 1
/* bit0:    0 for data or unified,  1 for instruction 
 * bit1~3:  0 for level1,           1 for level2 */ 
#define CSSEL_L1D   (0x0)
#define CSSEL_L1I   (0x1)
#define CSSEL_L2U   (0x2)

static char *cache_name[] = {"L1D", "L1I", "L2U"};
unsigned int cssel_config[] = {CSSEL_L1D, CSSEL_L1I, CSSEL_L2U};

/* Indirect stringification.  Doing two levels allows the parameter to be a
 * macro itself.  For example, compile with -DFOO=bar, __stringify(FOO)
 * converts to "bar".
 * Note: __stringify is copied from linux-5.4.55/include/linux/stringify.h
 */
#define __stringify_1(x...)	#x
#define __stringify(x...)	__stringify_1(x)

/*
 * The "Z" constraint normally means a zero immediate, but when combined with
 * the "%x0" template means XZR.
 * Note: write_sysreg is copied from linux-5.4.55/arch/arm64/include/asm/sysreg.h
 */
#define write_sysreg(v, r) do {					\
	unsigned long __val = (unsigned long)(v);					\
	asm volatile("msr " __stringify(r) ", %x0"		\
		     : : "rZ" (__val));				\
} while (0)

/*
 * Unlike read_cpuid, calls to read_sysreg are never expected to be
 * optimized away or replaced with synthetic values.
 * Note: write_sysreg is copied from linux-5.4.55/arch/arm64/include/asm/sysreg.h
 */
#define read_sysreg(r) ({					\
	unsigned long __val;						\
	asm volatile("mrs %0, " __stringify(r) : "=r" (__val));	\
	__val;							\
})

/* Which cache CCSIDR represents depends on CSSELR value. 
 * Note: get_ccsidr is copied from linux-5.4.55/arch/arm64/kvm/sys_regs.c */
static unsigned int get_ccsidr(unsigned int csselr)
{
	unsigned int ccsidr;

	write_sysreg(csselr, csselr_el1);
	isb();
	ccsidr = read_sysreg(ccsidr_el1);

	return ccsidr;
}

/*
 * type==0: 128K_L2+128K_S,  type==1: 256K_S,  type==2: 256K_L2 .
 *
 * Note: this function is called before D-cache is enabled.
 */
void l2c_sram_config(int type)
{
    unsigned int val;

    ca53_disable_flush_caches();

    val = readReg32(CSR_MP0_AXI_CONFIG);
    val &= ~(1<<4);        /* disable MP0 ACINACTM */
    writeReg32(CSR_MP0_AXI_CONFIG, val);

    val = readReg32(CSR_L2C_CFG_MP0);
    val &= ~(0xf01);        /* clear bit0,8~11 */
    val |= l2c_config[type];
    writeReg32(CSR_L2C_CFG_MP0, val);

    ca53_enable_icache();   /* dcache will be enabled later in plat_configure_mmu_el3 */
	if(!uartDisable){
    printf("L2C_type:%s\r\n", l2c_type_name[type]);
	}
    writeReg32(0x1fb00280, t_size[type]); /* use register to pass l2c_sram type to kernel */

    return;
}

#ifdef L2C_SRAM_VERIFY
void l2c_sram_verify(void)
{    
    unsigned int words;
    int i, j, k, l;
    unsigned long *longP;
    unsigned long tmp;

    
    for (i=0; i<2; i++) {

        l2c_sram_config(i);

        words = t_size[i]/sizeof(unsigned long);

        for (j=0; j<(sizeof(t_base)/sizeof(t_base[0])); j++) {
        
            longP = (unsigned long *)t_base[j];

            for (k=0; k<(sizeof(t_pat)/sizeof(t_pat[0])); k++) {
            
                for (l=0, tmp=t_pat[k]; l<words; l++, tmp++) {
                    longP[l] = tmp;
                }

                for (l=0, tmp=t_pat[k]; l<words; l++, tmp++) {
                    if (longP[l] != tmp) {
                        printf("ERROR: longP[%d]:0x%lx != tmp:0x%lx when i:%d, j:%d, k:%d\r\n", l, longP[l], tmp, i, j, k);
                        goto l2c_sram_verify_fail;
                    }
                }                    
            }
        }
    }

    printf("l2c_sram_verify done\r\n");
    return;

l2c_sram_verify_fail:
    return;
}
#endif
#endif

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	uint64_t dram_size = 0;
    unsigned int numSet, associativity, LineSize, ccsidr, cacheSize;
    int i;
	
#ifdef L2C_SRAM_CONFIG
    #ifdef L2C_SRAM_VERIFY
	l2c_sram_verify();
    #endif
    #if defined(TCSUPPORT_AUTOBENCH)
    l2c_sram_config(l2t_half_l2c_half_sram);
    #else
    if (isAN7581GT||isAN7566GT||isAN7581FG||isAN7551GT)
        l2c_sram_config(l2t_half_l2c_half_sram);
    else
        l2c_sram_config(l2t_full_l2c);
    #endif
#else
    /* bl1 set l2c as half_l2_sram+half_l2_cache, so set l2c back to full_l2_cache. */
    l2c_sram_config(l2t_full_l2c);
#endif

#ifdef TCSUPPORT_CPU_EN7581
	if (1 == efuse_check_eco())
	{
		mmio_setbits_32(RG_TOP_REV_24, (uint32_t)0x1 << 0);
	}
#endif

	/* if dram size is 2GB in this moment, it may not be the real dram size because the BL2 only support 32bit dram size calibration*/
	/* need to re-calculate dram size*/ 
	if ((GET_DRAM_SIZE() << 20) == S_2G)
	{
		dram_size = calculate_dram_size();
		SET_DRAM_SIZE(dram_size >> 20);
	}

    plat_configure_mmu_el3(TZRAM_BASE,
                   (TZRAM_SIZE + TZRAM2_SIZE),
                   BL_CODE_BASE,
                   BL_CODE_END,
                   BL_COHERENT_RAM_BASE,
                   BL_COHERENT_RAM_END);

    for (i=0; i<3; i++) {
		if(!uartDisable){
        ccsidr = get_ccsidr(cssel_config[i]);
        numSet = ((ccsidr>>13)&0x3fff)+1;
        associativity = ((ccsidr>>3)&0x2ff)+1;
        LineSize = (1<<(((ccsidr)&0x7)+4));
        cacheSize = (numSet*associativity*LineSize);
        printf("%s cache size: %d KB (with set/asso/line==%d/%d/%d)\n", 
                cache_name[i], cacheSize>>10, numSet, associativity, LineSize);
		}	
    }
    return;
}

void bl31_plat_runtime_setup(void)
{
#if TRUSTED_BOARD_BOOT
	/* Initialize authentication module */
	auth_mod_init();
#endif /* TRUSTED_BOARD_BOOT */
}

entry_point_info_t *bl31_plat_get_next_kernel64_ep_info(void)
{
	entry_point_info_t *next_image_info;
	unsigned long el_status;
	unsigned int mode;

	el_status = 0;
	mode = 0;

	/* Kernel image is always non-secured */
	next_image_info = &bl33_ep_info;

	/* Figure out what mode we enter the non-secure world in */
	el_status = read_id_aa64pfr0_el1() >> ID_AA64PFR0_EL2_SHIFT;
	el_status &= ID_AA64PFR0_ELX_MASK;

	if (el_status){
		INFO("Kernel_EL2\n");
		mode = MODE_EL2;
	} else{
		INFO("Kernel_EL1\n");
		mode = MODE_EL1;
	}

	INFO("Kernel is 64Bit\n");
	next_image_info->spsr = SPSR_64(mode, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	next_image_info->pc = get_kernel_info_pc();
	next_image_info->args.arg0=get_kernel_info_r0();
//	next_image_info->args.arg1=get_kernel_info_r1();

	INFO("pc=0x%lx, r0=0x%llx, r1=0x%llx\n",
		   next_image_info->pc,
		   next_image_info->args.arg0,
		   next_image_info->args.arg1);


	SET_SECURITY_STATE(next_image_info->h.attr, NON_SECURE);

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

entry_point_info_t *bl31_plat_get_next_kernel32_ep_info(void)
{
	entry_point_info_t *next_image_info;
	unsigned int mode;

	mode = 0;

	/* Kernel image is always non-secured */
	next_image_info = &bl33_ep_info;

	/* Figure out what mode we enter the non-secure world in */
	mode = MODE32_hyp;
	/*
	 * TODO: Consider the possibility of specifying the SPSR in
	 * the FIP ToC and allowing the platform to have a say as
	 * well.
	 */

	INFO("Kernel is 32Bit\n");
	next_image_info->spsr = SPSR_MODE32 (mode, SPSR_T_ARM, SPSR_E_LITTLE,
							(DAIF_FIQ_BIT | DAIF_IRQ_BIT | DAIF_ABT_BIT));
	next_image_info->pc = get_kernel_info_pc();
	next_image_info->args.arg0=get_kernel_info_r0();
	next_image_info->args.arg1=get_kernel_info_r1();
	next_image_info->args.arg2=get_kernel_info_r2();

	INFO("pc=0x%lx, r0=0x%llx, r1=0x%llx, r2=0x%llx\n",
		   next_image_info->pc,
		   next_image_info->args.arg0,
		   next_image_info->args.arg1,
		   next_image_info->args.arg2);


	SET_SECURITY_STATE(next_image_info->h.attr, NON_SECURE);

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

void bl31_prepare_kernel_entry(uint64_t k32_64)
{
	entry_point_info_t *next_image_info;
	uint32_t image_type;

	/* Determine which image to execute next */
	image_type = NON_SECURE; //bl31_get_next_image_type();

	/* Program EL3 registers to enable entry into the next EL */

	if (0 == k32_64) {
		next_image_info = bl31_plat_get_next_kernel32_ep_info();
	} else {

		next_image_info = bl31_plat_get_next_kernel64_ep_info();
	}
	assert(next_image_info);
	assert(image_type == GET_SECURITY_STATE(next_image_info->h.attr));

	INFO("BL3-1: Preparing for EL3 exit to %s world, Kernel\n",
		(image_type == SECURE) ? "secure" : "normal");
	INFO("BL3-1: Next image address = 0x%llx\n",
		(unsigned long long) next_image_info->pc);
	INFO("BL3-1: Next image spsr = 0x%x\n", next_image_info->spsr);
	cm_init_my_context(next_image_info);
	cm_prepare_el3_exit(image_type);
}

