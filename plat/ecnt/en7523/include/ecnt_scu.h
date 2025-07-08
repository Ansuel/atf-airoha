#ifndef _ECNT_SCU_H_
#define _ECNT_SCU_H_

#include <lib/mmio.h>
#ifdef IMAGE_BL31
#define readl(reg)						mmio_read_64(reg)
#define writel(wdata, reg)				mmio_write_64(reg,wdata)
#else
#define readl(reg)						mmio_read_32(reg)
#define writel(wdata, reg)				mmio_write_32(reg,wdata)
#endif

extern unsigned int GET_DRAM_SIZE(void);
extern void SET_DRAM_SIZE(unsigned int val);

extern unsigned int GET_SYS_CLK(void);
extern void SET_SYS_CLK(unsigned int val);

extern unsigned int GET_IS_FPGA(void);
extern void SET_IS_FPGA(unsigned int val);

extern unsigned int GET_IS_SPI_CONTROLLER_ECC(void);
extern void SET_IS_SPI_CONTROLLER_ECC(unsigned int val);

extern unsigned int GET_IS_SECURE_MODE(void);
extern void SET_IS_SECURE_MODE(unsigned int val);

extern unsigned int GET_IS_SECURE_HWTRAP(void);
extern void SET_IS_SECURE_HWTRAP(unsigned int val);

extern unsigned int GET_HIR(void);
extern unsigned int GET_PDIDR(void);
extern unsigned int GET_NP_SCU_EMMC(void);
extern void SET_IS_DDR4(unsigned int val);
extern void SET_PACKAGE_ID(unsigned int val);
extern void set_boot_from_spi_ejtag_enable(unsigned int isEnabled);
#if defined(TCSUPPORT_CPU_EN7581) && defined(IMAGE_BL2)
extern void SET_R2C_MODE(int mode);
#endif

#endif
