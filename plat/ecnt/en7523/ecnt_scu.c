

#include <asm/tc3162.h>
#include <en7523_def.h>

#define GET_REG_SAVE_INFO				((SYS_GLOBAL_PARM_T){ .word = get_np_scu_data(EN7523_SCREG_WR1)})
#define SET_REG_SAVE_INFO(x)			(set_np_scu_data(EN7523_SCREG_WR1, x))

unsigned int get_np_scu_data(unsigned int reg)
{
	return readl(reg);
}

/* don't EXPORT this function. Create API for your purpose instead. */
void set_np_scu_data(unsigned int reg, unsigned int val)
{
	writel(val, reg);
}

unsigned int GET_IS_DDR4(void)
{
	return GET_REG_SAVE_INFO.raw.isDDR4;
}

void SET_IS_DDR4(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.isDDR4 = val;
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_DRAM_SIZE(void)
{
	return (GET_REG_SAVE_INFO.raw.dram_size << 4);
}

void SET_DRAM_SIZE(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.dram_size = (val >> 4);
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_SYS_CLK(void)
{
	return GET_REG_SAVE_INFO.raw.sys_clk;
}

void SET_SYS_CLK(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.sys_clk = val;
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_IS_FPGA(void)
{
	return GET_REG_SAVE_INFO.raw.isFpga;
}

void SET_IS_FPGA(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.isFpga = val;
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_IS_SPI_CONTROLLER_ECC(void)
{
	return GET_REG_SAVE_INFO.raw.isCtrlEcc;
}

void SET_IS_SPI_CONTROLLER_ECC(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.isCtrlEcc = val;
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_PACKAGE_ID(void)
{
	unsigned int id;

	id = (GET_REG_SAVE_INFO.raw.packageID | (GET_REG_SAVE_INFO.raw.packageID_ext << 4));
	return id;
}

void SET_PACKAGE_ID(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.packageID = (val & 0xF);
	z.raw.packageID_ext = ((val >> 4) & 0x1);
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_IS_SECURE_MODE(void)
{
	return GET_REG_SAVE_INFO.raw.isSecureModeEn;
}

void SET_IS_SECURE_MODE(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.isSecureModeEn = val;
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_IS_SECURE_HWTRAP(void)
{
	return GET_REG_SAVE_INFO.raw.isSecureHwTrapEn;
}

void SET_IS_SECURE_HWTRAP(unsigned int val)
{
	SYS_GLOBAL_PARM_T z = GET_REG_SAVE_INFO;
	z.raw.isSecureHwTrapEn = val;
	SET_REG_SAVE_INFO(z.word);
}

unsigned int GET_HIR(void)
{
    return ((get_np_scu_data(EN7523_SCREG_HIR) >> 16) & (0xFFFF));
}

unsigned int GET_PDIDR(void)
{
    return (get_np_scu_data(CR_NP_SCU_PDIDR) & (0xFFFF));
}

unsigned int GET_NP_SCU_EMMC(void)
{
	return ((get_np_scu_data(CR_NP_SCU_BOOT_TRAP_CONF_DEC) & BOOT_TRAP_CONF_DEC_EMMC_MASK) >> BOOT_TRAP_CONF_DEC_EMMC_OFFSET);
}

#if defined(TCSUPPORT_CPU_EN7581) && defined(IMAGE_BL2)
void SET_R2C_MODE(int mode)
{
	unsigned int data = 0;

	data = get_np_scu_data(EN7581_R2C_SEL);
	switch(mode)
	{
		case R2C_OLD:
			data &= ~((R2C_NEW_EN)| (R2C_WRAP_EN) | (R2C_256_EN));
			break;
		case R2C_NEW128:
			data |= R2C_NEW_EN;
			data &= ~((R2C_WRAP_EN) | (R2C_256_EN));
			break;
		case R2C_NEW256_NO_WRAP:
			data |= ((R2C_NEW_EN) | (R2C_256_EN));
			data &= ~(R2C_WRAP_EN);
			break;
		case R2C_NEW256:
			data |= (R2C_NEW_EN| R2C_WRAP_EN | R2C_256_EN);
			break;
		default:
			printf("ERROR: unknown mode in SET_R2C_MODE\n");
			break;
	}
	set_np_scu_data(EN7581_R2C_SEL, data);
}
#endif

void set_boot_from_spi_ejtag_enable(unsigned int isEnabled)
{
	unsigned int reg_val;

	reg_val = readl(EN7523_EJTAG_ENABLE);
	reg_val &= ~BOOT_FROM_SPI_EJTAG;
	
	if(isEnabled) {
		reg_val |= BOOT_FROM_SPI_EJTAG;
	}

	writel(reg_val, EN7523_EJTAG_ENABLE);
}

