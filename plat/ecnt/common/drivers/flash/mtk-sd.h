#ifndef __MTK_MMC_H__
#define __MTK_MMC_H__

#define MSDC0_BASE			0x1fa0e000
#define MSDC_SRC_CK			(200 * 1000 * 1000)	//ASIC Clock
#define MSDC_BUS_WIDTH		(MMC_BUS_WIDTH_4)

struct msdc_compatible {
	uint8_t clk_div_bits;
	bool pad_tune0;
	bool async_fifo;
	bool data_tune;
	bool busy_check;
	bool stop_clk_fix;
	bool enhance_rx;
	bool r_smpl;
	uint32_t latch_ck;
};

int mtk_mmc_init(void);

#endif
