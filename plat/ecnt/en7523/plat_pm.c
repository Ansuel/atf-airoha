/*
 * Copyright (c) 2013-2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/gicv3.h>
#include <drivers/console.h>
#include <drivers/delay_timer.h>
#include <lib/bakery_lock.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <plat/arm/common/plat_arm.h>
#include <plat/common/platform.h>
#include <cortex_a53.h>

#include <en7523_def.h>
#include <plat_private.h>
#include <asm/tc3162.h>

#define AUTO_POWER_OFF	1
#define POLLING_TIMES	10000

/* Macros to set up EcoNet watchdog timer for system restart */			
#define ENABLE                (1)
#define TIMER_HALTDISABLE     (0)
#define SYS_HCLK              (0x0a0)
#define TIMERTICKS_10MS       (10) // set timer ticks as 10 ms
#define CR_TIMER_CTL          (0x1FBF0100)
#define CR_TIMER3_LDV         (0x1FBF012C)			
#define CR_WDOG_RLD           (0x1FBF0138)	

static inline uint64_t read_cntpctl(void)
{
	uint64_t cntpctl;

	__asm__ volatile("mrs	%x0, cntp_ctl_el0"
			 : "=r" (cntpctl) : : "memory");

	return cntpctl;
}

static inline void write_cntpctl(uint64_t cntpctl)
{
	__asm__ volatile("msr	cntp_ctl_el0, %x0" : : "r"(cntpctl));
}

static void plat_cpu_standby(plat_local_state_t cpu_state)
{
	unsigned int scr;

	scr = read_scr_el3();
	write_scr_el3(scr | SCR_IRQ_BIT);
	isb();
	dsb();
	wfi();
	write_scr_el3(scr);
}

static inline unsigned long read_cpuectlr(void)
{
	unsigned long cpuectlr;

	__asm__ volatile("mrs	%x0, S3_1_C15_C2_1"
			 : "=r" (cpuectlr) : : "memory");

	return cpuectlr;
}

static inline void write_cpuectlr(uint64_t cpuectlr)
{
	__asm__ volatile("msr	S3_1_C15_C2_1, %x0" : : "r"(cpuectlr));
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be turned
 * on. The level and mpidr determine the affinity instance.
 ******************************************************************************/
static uintptr_t ecnt_sec_entrypoint;

static uint32_t check_flag[PLATFORM_CORE_COUNT] = {0};

static void plat_power_domain_wait_reg(uintptr_t addr, uint32_t vlaue, bool state)
{
	unsigned long timeout = POLLING_TIMES;


	while(timeout)
	{
        if ((state==true) && ((mmio_read_32(addr) & (vlaue))!=0))
            break;
        if ((state==false) && ((mmio_read_32(addr) & (vlaue))==0))
            break;
		timeout--;
		udelay(1);
	}

	if (!timeout)
		INFO("Wait addr %lx value %x timeout\n", addr, vlaue);
}

static int plat_power_domain_on(unsigned long mpidr)
{
	int rc = PSCI_E_SUCCESS;
	unsigned long cpu_id = 0;
	unsigned long cluster_id = 0;
	uintptr_t rv = 0;
	cpu_id = mpidr & MPIDR_CPU_MASK;
	cluster_id = mpidr & MPIDR_CLUSTER_MASK;
	rv = (MCUCFG_BASE + 0x38 + (cpu_id * 8));

	/* Set RVBAR for core */
	mmio_write_32(rv, ecnt_sec_entrypoint);
	INFO("mt_on[%ld:%ld], entry %x\n",
		cluster_id, cpu_id, mmio_read_32(rv));

	/* Set spmc_pwr_rst_cpu[m] = 0 */
	mmio_clrbits_32((SPM_BASE + 0x1C), 1 << (13 - cpu_id));

	/* Set spmc_pwr_on_cpu[m] = 1 */
	mmio_setbits_32((SPM_BASE + 0x1C), 1 << (25 - (cpu_id * 2)));

	/* Wait spmc_pwr_ack_cpu[m] = 1 */
	plat_power_domain_wait_reg((SPM_BASE + 0x1C), (1 << (24 - (cpu_id * 2))), true);

	return rc;
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be turned
 * off. The level and mpidr determine the affinity instance. The 'state' arg.
 * allows the platform to decide whether the cluster is being turned off and
 * take apt actions.
 *
 * CAUTION: This function is called with coherent stacks so that caches can be
 * turned off, flushed and coherency disabled. There is no guarantee that caches
 * will remain turned on across calls to this function as each affinity level is
 * dealt with. So do not write & read global variables across calls. It will be
 * wise to do flush a write to the global to prevent unpredictable results.
 ******************************************************************************/
void plat_power_domain_off_check(unsigned long cpu_id)
{
	if (check_flag[cpu_id] == 1)
	{
#if AUTO_POWER_OFF
		/* Wait spmc_pwr_ack_cpu[m] = 0 */
		plat_power_domain_wait_reg((SPM_BASE + 0x1C), (1 << (24 - (cpu_id * 2))), false);
#else
		/* Wait STANDBYWIFI[m] = 1 */
		plat_power_domain_wait_reg((SPM_BASE + 0x20), ((1 << cpu_id) << 1), true);

		/* set spmc_resetpwron_config_cpu[m] = 0 */
		mmio_clrbits_32((SPM_BASE + 0x1C), 1 << (8 - cpu_id));

		/* Set spmc_pwr_on_cpu[m] = 0 */
		mmio_clrbits_32((SPM_BASE + 0x1C), 1 << (25 - (cpu_id * 2)));

		/* Wait spmc_pwr_ack_cpu[m] = 0 */
		plat_power_domain_wait_reg((SPM_BASE + 0x1C), (1 << (24 - (cpu_id * 2))), false);

		/* Set spmc_pwr_rst_cpu[m] = 1 */
		mmio_setbits_32((SPM_BASE + 0x1C), 1 << (13 - cpu_id));
#endif

		check_flag[cpu_id] = 0;
	}
}

static void __dead2 plat_power_domain_down_wfi(const psci_power_state_t *state)
{
	unsigned long mpidr = read_mpidr_el1();
	unsigned long cpu_id = 0;

	cpu_id = mpidr & MPIDR_CPU_MASK;

#if AUTO_POWER_OFF
	/* set spmc_resetpwron_config_cpu[m] = 0 */
	mmio_clrbits_32((SPM_BASE + 0x1C), 1 << (8 - cpu_id));

	/* Set spmc_pwr_on_cpu[m] = 0 */
	mmio_clrbits_32((SPM_BASE + 0x1C), 1 << (25 - (cpu_id * 2)));
#endif

	check_flag[cpu_id] = 1;

	psci_power_down_wfi();
}

static void plat_power_domain_off(const psci_power_state_t *state)
{
	unsigned long mpidr = read_mpidr_el1();
	unsigned long cpu_id = mpidr & MPIDR_CPU_MASK;
#if AUTO_POWER_OFF
	unsigned long cluster_id = mpidr & MPIDR_CLUSTER_MASK;
	unsigned long cpuectlr = 0;

	/* Set sw_no_wait_for_q_channel_cpu[m] = 0 */
	mmio_clrbits_32((MCUCFG_BASE + 0x1C30+ (cpu_id * 4)), (1 << 1));

	/* Set retention chk = 2 */
	cpuectlr = ((read_cpuectlr() & ~(CORTEX_A53_ECTLR_CPU_RET_CTRL_MASK)) |
				(RETENTION_ENTRY_TICKS_2 << CORTEX_A53_ECTLR_CPU_RET_CTRL_SHIFT));
	write_cpuectlr(cpuectlr);
	INFO("mt_off[%ld:%ld], cpuectlr %lx\n",
		cluster_id, cpu_id, cpuectlr);
#else
	/* Set sw_no_wait_for_q_channel_cpu[m] = 1 */
	mmio_setbits_32((MCUCFG_BASE + 0x1C30+ (cpu_id * 4)), (1 << 1));
#endif

	/* Prevent interrupts from spuriously waking up this cpu */
	plat_arm_gic_cpuif_disable();

	/* Turn redistributor off */
	plat_arm_gic_redistif_off();
}

static void plat_power_domain_on_finish(const psci_power_state_t *state)
{
	/* Program GIC per-cpu distributor or re-distributor interface */
	plat_arm_gic_pcpu_init();

	/* Enable GIC CPU interface */
	plat_arm_gic_cpuif_enable();
}

/*******************************************************************************
 * EcoNet handlers to shutdown/reboot the system
 ******************************************************************************/
static void __dead2 plat_system_off(void)
{
	INFO("EcoNet System Off\n");

	wfi();
	ERROR("EcoNet System Off: operation not handled.\n");
	panic();
}

/*
 * CR_TIMER_CTL -- timer control register
 * timer3 enable: bit 5
 * timer3 interrupt enable: bit 21
 * watchdog_timer enable: bit25
 * 
 */

static void timer_Configure(uint8_t timer_no, uint8_t timer_enable, uint8_t timer_halt)
{
	uint32_t word, offset;

	if(timer_no > 3) {
		printf("Error, timer_no:%u is more than 3.\n", timer_no);
		return;
	}

	if(timer_no == 3) {
		offset = 5;
	} else {
		offset = timer_no;
	}

	word = mmio_read_32(CR_TIMER_CTL);

	/* Set enable bit */
	if(timer_enable) {
		word |= (1 << offset);
	} else {
		word &= ~(1 << offset);
	}

	/* Set interrupt bit */
	if(timer_halt) {
		word |= (1 << (offset + 16));
	} else {
		word &= ~(1 << (offset + 16));
	}

	mmio_write_32(CR_TIMER_CTL, word);

} 

 static void timerLdvSet(uint8_t timer_no, uint32_t val)
{
	mmio_write_32(CR_TIMER3_LDV,val);
}

static void timerCtlSet(uint8_t timer_no, uint8_t timer_enable, uint8_t timer_halt)
{
	timer_Configure(timer_no, timer_enable, timer_halt);	
}

void timer_WatchDogConfigure(uint8_t tick_enable, uint8_t watchdog_enable)
{

	uint32_t word;

	word = mmio_read_32(CR_TIMER_CTL);
	word &= 0xfdffffdf;
	word |= ( tick_enable << 5)|(watchdog_enable<<25);
	mmio_write_32(CR_TIMER_CTL, word);
}

void timerSet(uint32_t timer_no, uint32_t timerTime, uint32_t enable, uint8_t timer_halt)
{   
    uint32_t word, offset;
	
	if(timer_no > 3) {
		printf("Error, timer_no:%u is more than 3.\n", timer_no);
		return;
	}

	if(timer_no == 3) {
		offset = 5;
	} else {
		offset = timer_no;
	}
	
	if((mmio_read_32(CR_TIMER_CTL) & (1 << offset)) == (1 << offset)) {
		if(timer_no != 3){
			printf("%s: Error, APB Timer%d has been enabled.\r\n", __func__, timer_no);
			return;
		}
	}

	/* when SYS_HCLK is large, it will cause overflow. The calculation will be wrong */
    /* word = (timerTime * SYS_HCLK) * 1000 / 2; */
    word = (timerTime * SYS_HCLK) * 500; 
	/* set timer3 countdown value */
    timerLdvSet(timer_no, word);
	/* enable or disable timer3 and its interrupt */
    timerCtlSet(timer_no, enable, timer_halt);
}

 void __dead2 plat_system_reset(void)
{
	/* Write the System Configuration Control Register */
	INFO("EcoNet System Reset\n");
	uint32_t tmp;

	timerSet(3, 10 * TIMERTICKS_10MS, ENABLE, TIMER_HALTDISABLE);
	mmio_write_32(CR_WDOG_RLD, 0x1);
	timer_WatchDogConfigure(ENABLE, ENABLE);
	if(isAN7583)
	{
		/* make sure DDR data are all done before reset*/
		tmp = mmio_read_32(DRAMC_CONF);
		tmp &= ~(1<<2);
		mmio_write_32(DRAMC_CONF, tmp);
		/*7583 boot flag*/
		mmio_write_32(EN7523_SCREG_WF0, (mmio_read_32(EN7523_SCREG_WF0)|(DBG_7583_BOOT_MAGIC)));
	}
	wfi();
	ERROR("EcoNet System Reset: operation not handled.\n");
	panic();
}


static const plat_psci_ops_t plat_plat_pm_ops = {
	.cpu_standby					= plat_cpu_standby,
	.pwr_domain_on					= plat_power_domain_on,
	.pwr_domain_on_finish			= plat_power_domain_on_finish,
	.pwr_domain_off					= plat_power_domain_off,
	.pwr_domain_pwr_down_wfi		= plat_power_domain_down_wfi,
	.system_off						= plat_system_off,
	.system_reset					= plat_system_reset,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	ecnt_sec_entrypoint = sec_entrypoint;
    flush_dcache_range((uint64_t)&ecnt_sec_entrypoint,
               sizeof(ecnt_sec_entrypoint));

	*psci_ops = &plat_plat_pm_ops;

	return 0;
}
