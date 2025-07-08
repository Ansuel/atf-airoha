
//#define TEST_BLOCK_CNT_IN_SRAM
//#define TEST_RBUS_TIMEOUT
//#define TEST_RBUS_DRAM_CNT
//#define IS_FPGA_STAGE
//#define TEST_SW_RESET
//#define TEST_DRAM_RBUS_RESET

#include <stdio.h>
#include <string.h>
#include <lib/mmio.h>
//#include <drivers/delay_timer.h>

#ifdef IS_FPGA_STAGE
#define EMI_CLK_DOMAIN      (0x4000000) /*64 MHz*/
#ifdef TCSUPPORT_CPU_AN7583
#define APB_TMIER_CLK       (50)/*MHz*/
#else
#define APB_TMIER_CLK       (32)/*MHz*/
#endif
#define APB_CPUTMR_CLK      (50)/*MHz*/
#else
#ifdef TCSUPPORT_CPU_EN7581
#define EMI_CLK_DOMAIN      (0x21C00000) /*540 MHz*/
#define APB_TMIER_CLK       (150)/*MHz*/
#define CR_CHIP_SCU_EMI_CLK (0x1fa201b8)
#define CR_CHIP_SCU_BUS_CLK (0x1fa201bc)
#else
#define EMI_CLK_DOMAIN      (0x14D00000) /*333 MHz*/
#define APB_TMIER_CLK       (125)/*MHz*/
#endif

#define APB_CPUTMR_CLK      (200)/*MHz*/
#endif

#if defined(TEST_BLOCK_CNT_IN_SRAM) || defined(TEST_RBUS_TIMEOUT)
#define CR_TIMER_BASE  		0x1fbf0100
#define CR_TIMER_CTL    	(CR_TIMER_BASE + 0x00)
#define CR_TIMER0_LVR   	(CR_TIMER_BASE + 0x04)
#define CR_TIMER0_CVR    	(CR_TIMER_BASE + 0x08)
#define CR_TIMER2_LVR       (CR_TIMER_BASE + 0x14)
#define CR_TIMER2_CVR       (CR_TIMER_BASE + 0x18)

#define DELAY_US_MODE	(0)
#define DELAY_MS_MODE	(1)

static unsigned int get_1us_timeTick_by_timeClk(unsigned int clk /*MHz*/)
{
    /* if timer_clk is 1 MHz, timeTick should be 1 to stand for 1us */
    return clk;
}

#if defined(TCSUPPORT_CPU_EN7581) && !defined(IS_FPGA_STAGE)
static unsigned int bus_clk_get(void)
{
    unsigned int bus_clksrc[] = {600, 540};             /*0x1fa201bc[8], no div1*/
    unsigned int regVal, clksrc, clkdiv, bus_clk;

    regVal = mmio_read_32(CR_CHIP_SCU_BUS_CLK);
    clksrc = (regVal>>8)&0x1;
    clkdiv = regVal&0x7;

    if (clkdiv==0) {
        printf("Error(%s): rbus_clk's div can't be 0 (CR_CHIP_SCU_BUS_CLK:0x%x)\n", __func__, regVal);
        return 0;
    }

    bus_clk = bus_clksrc[clksrc]/(clkdiv+1);
    //printf("bus_clk: %d MHz\n", (int)bus_clk);
    return bus_clk;
}
#endif

static unsigned int get_tmrTick_by_usTime (unsigned int usTime)
{
    unsigned int apb_bus_clk=APB_TMIER_CLK;

    #if defined(TCSUPPORT_CPU_EN7581) && !defined(IS_FPGA_STAGE)
    apb_bus_clk = (bus_clk_get()>>1);
    #endif

    return (get_1us_timeTick_by_timeClk(apb_bus_clk)*usTime);
}

static unsigned int get_1ms_timeTick_by_timeClk(unsigned int clk /*MHz*/)
{
    /* if timer_clk is 1 MHz, timeTick should be 1000 to stand for 1ms */
    return clk*1000;
}

static unsigned int get_tmrTick_by_msTime (unsigned int msTime)
{
    unsigned int apb_bus_clk=APB_TMIER_CLK;

    #if defined(TCSUPPORT_CPU_EN7581) && !defined(IS_FPGA_STAGE)
    apb_bus_clk = (bus_clk_get()>>1);
    #endif

    return (get_1ms_timeTick_by_timeClk(apb_bus_clk)*msTime);
}

static int apb_timer_delay(unsigned int mode, unsigned int time)
{
	volatile unsigned int timer_now, timer_last;
	volatile unsigned int tick_acc;
	unsigned int tick_per_unit;
	volatile unsigned int tick_wait; 
	volatile unsigned int timer2_ldv = mmio_read_32(CR_TIMER2_LVR);
	unsigned int same_count = 0;


	if(mode > DELAY_MS_MODE) {
    	printf("%s: Delay mode:%d error.\r\n", __func__, mode);
    	return -1;
	}

	if((mmio_read_32(CR_TIMER_CTL) & 0x4) == 0) {
		printf("%s: Error, APB Timer2 does not be enabled.\r\n", __func__);
		return -1;
	}

    if (mode==DELAY_US_MODE)
	    tick_per_unit = get_tmrTick_by_usTime(1);
    else if (mode==DELAY_MS_MODE)
        tick_per_unit = get_tmrTick_by_msTime(1);
    else {}

	tick_wait = time * tick_per_unit;
	
	tick_acc = 0;
	timer_last = mmio_read_32(CR_TIMER2_CVR);
	do {
		timer_now = mmio_read_32(CR_TIMER2_CVR);
		if(timer_last == timer_now) {
			same_count++;
		}
		if(same_count >= 10) {
			printf("%s: dead loop, break;\r\n", __func__);
			return -1;
		}
	  	if (timer_last >= timer_now) {
	  		tick_acc += timer_last - timer_now;
		} else {
			tick_acc += timer2_ldv - timer_now + timer_last;
		}
		timer_last = timer_now;
	} while (tick_acc < tick_wait);

	return 0;
}

void delay1us(int us)
{
	apb_timer_delay(DELAY_US_MODE, us);
}

void delay1ms(int ms)
{
	apb_timer_delay(DELAY_MS_MODE, ms);
}

void 
apb_timer_init(
	unsigned int timer_no,  /*0, 1, 2, or 3*/
	unsigned int enable,    /*0: disable, 1: enable*/
	unsigned int loadTime   /* ms */
)
{   
    unsigned int word;
	
    if (timer_no>3)
        printf("%s timer_no:%d is wrong, should be 0,1,2,or 3\r\n", __func__, timer_no);

    if (timer_no==3) timer_no=5;

    if (enable) {
        mmio_write_32(CR_TIMER0_LVR+(timer_no<<3), get_tmrTick_by_msTime(loadTime));
        word = mmio_read_32(CR_TIMER_CTL);
        word |= (1 << timer_no);
        mmio_write_32(CR_TIMER_CTL, word);
    }
    else {
        word = mmio_read_32(CR_TIMER_CTL);
        word &= (~(1 << timer_no));
        mmio_write_32(CR_TIMER_CTL, word);
    }
        
    return;
}
#endif

#ifdef TEST_BLOCK_CNT_IN_SRAM
#define DISABLE     0
#define ENABLE      1

#define RBUS_CORE_BASE  (0x1FA00000)
#define CR_BLOCK_EN     (RBUS_CORE_BASE+0x0ec)
#define CR_BLOCK_MASK   (RBUS_CORE_BASE+0x0f0)
#define CR_BLOCK_CNT0   (RBUS_CORE_BASE+0x0f4)  /* gdma,hsdma, woe,wdma,crypto,... */
#define CR_BLOCK_CNT1   (RBUS_CORE_BASE+0x0f8)  /* ppe */
#define CR_BLOCK_CNT2   (RBUS_CORE_BASE+0x0fc)  /* qdma_lan */
#define CR_BLOCK_CNT3   (RBUS_CORE_BASE+0x100)  /* qdma_wan */
#define CR_BLOCK_CNT4   (RBUS_CORE_BASE+0x104)  /* tdma */
#define CR_BLOCK_CNT5   (RBUS_CORE_BASE+0x108)  /* npu */

#define CR_AXI2RBUS_CFG         (0x1EFBC800)
#define CR_RBUS_PENDING_CNT     (CR_AXI2RBUS_CFG+0x10)

#define RBUS_PENDING_ADDR_SHIFT (24)
#define RBUS_PENDING_ADDR_BITS  (0xff)

#define CR_GDMA_BASE    (0x1FB30000)
#define CR_GDMA_SA0     (CR_GDMA_BASE+0x000)
#define CR_GDMA_DA0     (CR_GDMA_BASE+0x004)
#define CR_GDMA_CT00    (CR_GDMA_BASE+0x008)
#define CR_GDMA_CT10    (CR_GDMA_BASE+0x00C)
#define CR_GDMA_DONE    (CR_GDMA_BASE+0x204)

#define CR_NPU_MIB8 (0x1ec0c160)
#define CR_NPU_MIB9 (0x1ec0c164)
#define CR_NPU_MIB10 (0x1ec0c168)
#define CR_NPU_MIB11 (0x1ec0c16c)
#define CR_NPU_MIB12 (0x1ec0c170)

#define NPU_BLK_CNT_LOCK (0x1)
#ifdef TCSUPPORT_NPU_V2
#define REG_CORE_BOOT_CONFIG    (0x1ec06004)
#define REG_CORE_BOOT_TRIGGER   (0x1ec06000)
#define REG_CORE0_BOOT_BASE     (0x1ec06020)
#ifdef TCSUPPORT_CPU_AN7583
#define NPU_SRAM_BASE	        (0x3E840000)
#else
#define NPU_SRAM_BASE	        (0x3E820000)
#endif
#define NPU_16K_SRAM_BASE	    (0x3E900000)
#else
#define REG_CORE_BOOT_CONFIG    (0x1ec08004)
#define REG_CORE_BOOT_TRIGGER   (0x1ec08000)
#define REG_CORE0_BOOT_BASE     (0x1ec08020)
#define NPU_SRAM_BASE	        (0x1E800000)
#define NPU_16K_SRAM_BASE	    (0x1E900000)
#endif

#define DRAM_TEST_BASE  (0x84000000) /* 4000000: 64MB */
#define DMA_BLK_CASE    (2)
#define CPU_WR_CASE     (3)
#define DELAY_ITEM      (2)

#define CR_ARMCPU_CFG       (0x1FBE2E08)

#define FE_BASE         (0x1fb50000)
#define QDMA_LAN_BASE   (FE_BASE+0x4000)
#define QDMA_REG_OFFSET (0x2000)
#define QDMA_BUF_OFFSET (0x02000000)

#define QDMA_GLB_CFG    (0x004+QDMA_LAN_BASE)
#define FDSCP_BASE_CFG  (0x010+QDMA_LAN_BASE)
#define FBUF_BASE_CFG   (0x014+QDMA_LAN_BASE)
#define INT_STATUS      (0x020+QDMA_LAN_BASE)
#define INT_STATUS2     (0x024+QDMA_LAN_BASE)
#define INT_ENABLE      (0x028+QDMA_LAN_BASE)
#define INT_ENABLE2     (0x02c+QDMA_LAN_BASE)
#define IRQ_BASE        (0x050+QDMA_LAN_BASE)
#define IRQ_CFG         (0x054+QDMA_LAN_BASE)
#define IRQ_CLRLEN      (0x058+QDMA_LAN_BASE)
#define IRQ_STATUS      (0x05c+QDMA_LAN_BASE)
#define IRQ_PTIME       (0x060+QDMA_LAN_BASE)
#define TXQ_CFG_BASE    (0x100+QDMA_LAN_BASE)
#define RXQ_CFG_BASE    (0x200+QDMA_LAN_BASE)
#define DBGCNT_CFG      (0x400+QDMA_LAN_BASE)
#define LMGR_INIT_CFG   (0x1000+QDMA_LAN_BASE)
#define DSCP_AVAI_NUM   (0x10f0+QDMA_LAN_BASE)

#define INTRQ_BASE      (0x01000000+DRAM_TEST_BASE)
#define RDSCP_BASE      (0x01020000+DRAM_TEST_BASE)
#define TDSCP_BASE      (0x01080000+DRAM_TEST_BASE)
#define FDSCP_BASE      (0x010c0000+DRAM_TEST_BASE)
#define RXPKT_BASE      (0x01100000+DRAM_TEST_BASE)
#define TXPKT_BASE      (0x01500000+DRAM_TEST_BASE)
#define FWBUF_BASE      (0x02000000+DRAM_TEST_BASE)

#define rxDSCP_NUM      (2048)
#define rxDSCP_HOOK_NUM (32)
#define RX_PKT_MAX_SIZE (2048)
#ifdef TCSUPPORT_CPU_AN7583
#define NUM_AVAI_DSCP   (0xffc)
#else
#define NUM_AVAI_DSCP   (0xff8)
#endif

#define LITTLE_ENDIAN   (1)
//#define QDMA_DEBUG      (1)

#define SZ_128          0x80

#define XMODEM_BUFFER_SIZE	(SZ_128)

enum blockCntTestCase
{
    #ifdef TCSUPPORT_CPU_AN7583
    bctc_gdma=0,    /*and hsdma*/
    bctc_tdma,      /*and ppe*/
    bctc_qdma_lan,  /*i.e. qdma1_tx*/
    bctc_qdma1_rx,  /*=3*/
    bctc_qdma_wan,  /*i.e. qdma2_tx*/
    bctc_npu,       /*=5*/
    bctc_qdma2_rx,
    bctc_gdma_qdmalan,/*gdma+qdma1_tx test*/
    bctc_max_case   /*=8*/
    
    #else
    bctc_gdma=0,
    bctc_ppe,
    bctc_qdma_lan,
    bctc_qdma_wan,
    bctc_tdma,
    bctc_npu,
    bctc_gdma_qdmalan,
    bctc_max_case
    #endif
};

#ifdef TCSUPPORT_CPU_AN7583
char *dmaNames[8] = {"gdma_hsdma", "ppe_tdma", "qdma1_tx", "qdma1_rx", "qdma2_tx", "npu", "qdma2_rx", "gdma+qdma1_tx"};
#else
char *dmaNames[7] = {"gdma", "ppe", "qdma_lan", "qdma_wan", "tdma", "npu", "gdma_qdmalan"};
#endif
unsigned int g_reg_off=0, g_buf_off=0;
int isQdmaLanInit=0;
#if defined(CONFIG_TPL_BUILD)
extern int getcxmodem(void);
#endif


int qdma_loopback_init(int isWan)
{
    unsigned int dscp_idx;
    unsigned int dscp0=0, dscp1, dscp2, dscp3;
    unsigned int rdscp_addr;
    unsigned int reg, val;

    if (isWan) {
        g_reg_off=QDMA_REG_OFFSET;
        g_buf_off=QDMA_BUF_OFFSET;
    }
    else { /* qdma_lan */
        g_reg_off=0;
        g_buf_off=0;
        if (isQdmaLanInit==1)
            return 0;
    }

    /* 0x1FB00958[0] == 0 for QDMA registers, == 1 for FE_SRAM,
       So, test code must NOT run on FE_SRAM for qdma-related blockCnt tests */
    if ((mmio_read_32(0x1FB00958)&0x1)!=0) {
        printf("Warning: 0x1FB00958[0]:%d !=0\r\n", mmio_read_32(0x1FB00958)&0x1);
        printf("\t->Make sure test code and data do NOT run on FE_SRAM, otherwise setting 0x1FB00958[0]=0 will fail !!\r\n");
        val = mmio_read_32(0x1FB00958);
        val &= (~0x1);
        mmio_write_32(0x1FB00958, val);
        printf("Set 0x1FB00958[0]=0 OK\r\n");
    }

    /* qdma_init */

    mmio_write_32(TXQ_CFG_BASE+0x0+g_reg_off, TDSCP_BASE+g_buf_off);
    mmio_write_32(TXQ_CFG_BASE+0x4+g_reg_off, 1);
    mmio_write_32(TXQ_CFG_BASE+0x8+g_reg_off, 0); /* CTX */
    mmio_write_32(TXQ_CFG_BASE+0xC+g_reg_off, 0); /* DTX */

    mmio_write_32(RXQ_CFG_BASE+0x0+g_reg_off, RDSCP_BASE+g_buf_off);
    mmio_write_32(RXQ_CFG_BASE+0x4+g_reg_off, rxDSCP_NUM);
    mmio_write_32(RXQ_CFG_BASE+0x8+g_reg_off, 0); /* CRX */
    mmio_write_32(RXQ_CFG_BASE+0xC+g_reg_off, 0); /* DRX */
    mmio_write_32(RXQ_CFG_BASE+0x10+g_reg_off, 0x410);

    mmio_write_32(FDSCP_BASE_CFG+g_reg_off, FDSCP_BASE+g_buf_off);
    mmio_write_32(FBUF_BASE_CFG+g_reg_off, FWBUF_BASE+g_buf_off);
    mmio_write_32(INT_ENABLE+g_reg_off, 0);
    mmio_write_32(INT_ENABLE2+g_reg_off, 0);
    mmio_write_32(IRQ_BASE+g_reg_off, INTRQ_BASE+g_buf_off);
    mmio_write_32(IRQ_CFG+g_reg_off, 0x00100400);
    mmio_write_32(IRQ_PTIME+g_reg_off, 0x00000022);
    mmio_write_32(LMGR_INIT_CFG+g_reg_off, 0x91801000);
    delay1ms(10);

    mmio_write_32(QDMA_GLB_CFG+g_reg_off, 0x840100c5); /* QDMA_INIT */

    #ifdef QDMA_DEBUG
    /* enable debug counters */
    mmio_write_32(DBGCNT_CFG+g_reg_off, 0xf0000000); /* set dbg_ctx_done */
    mmio_write_32(DBGCNT_CFG+0x8+g_reg_off, 0xf1000000); /* set dbg_ftx_done */
    mmio_write_32(DBGCNT_CFG+0x10+g_reg_off, 0xf2000000); /* set dbg_crx_done */
    #endif

    reg = DSCP_AVAI_NUM+g_reg_off;
    if (mmio_read_32(reg)!=NUM_AVAI_DSCP) {
        printf("ERROR: qdma_init failed due to mmio_read_32(0x%x):0x%x !=0x%x\r\n", reg, mmio_read_32(reg), NUM_AVAI_DSCP);
        return -1;
    }

    /* qdma rx_dscp hook */

    for (dscp_idx=0, dscp1=RX_PKT_MAX_SIZE, dscp3=0; dscp_idx<=(rxDSCP_HOOK_NUM+1); dscp_idx++) { /* workaround: rx needs to hook one more dscp */

        rdscp_addr = RDSCP_BASE + (dscp_idx<<5) +g_buf_off;
        dscp2 = RXPKT_BASE + (dscp_idx*RX_PKT_MAX_SIZE) +g_buf_off;
        
        mmio_write_32(rdscp_addr+0x0, dscp0); /* write dscp0 */
        mmio_write_32(rdscp_addr+0x4, dscp1); /* write dscp1 (max_rx_receive_len. For loopback, it's about 100 bytes) */
        mmio_write_32(rdscp_addr+0x8, dscp2); /* write dscp2 */
        mmio_write_32(rdscp_addr+0xc, dscp3); /* write dscp3 */
        asm volatile ("dsb sy");
        mmio_write_32(RXQ_CFG_BASE+0x8+g_reg_off, dscp_idx+1); /* update CRX */
    }

    if (isWan==0)
        isQdmaLanInit=1;

    return 0;
}

void qdma_tx_dscp_hook(unsigned int dscp_idx, unsigned int src_addr, unsigned int test_len)
{
    unsigned int dscp0=0, dscp1, dscp2, dscp3;
    unsigned int tdscp_addr;
    unsigned int dma_chnl=0, dma_queue=0;
    unsigned int tmsg;

    tdscp_addr = TDSCP_BASE + (dscp_idx<<5) +g_buf_off;
    tmsg = (dma_chnl<<3) + dma_queue;

    dscp1 = test_len;
    dscp2 = src_addr;
    dscp3 = dscp_idx + 1;

    mmio_write_32(tdscp_addr+0x0, dscp0); /* write dscp0 */
    mmio_write_32(tdscp_addr+0x4, dscp1); /* write dscp1 */
    mmio_write_32(tdscp_addr+0x8, dscp2); /* write dscp2 */
    mmio_write_32(tdscp_addr+0xc, dscp3); /* write dscp3 */
    mmio_write_32(tdscp_addr+0x10, tmsg); /* write tmsg */
    asm volatile ("dsb sy");

}

void trigger_qdma(unsigned int dscp_idx)
{
    mmio_write_32(TXQ_CFG_BASE+0x8+g_reg_off, dscp_idx + 1); /* update CTX */
    #ifdef QDMA_DEBUG
    printf("dscp_idx:%d\r\n", dscp_idx);
    /* read debug counters */
    printf("\tqdma fetches  0x%x desp\\rn", mmio_read_32(DBGCNT_CFG+0x4+g_reg_off)); /* read dbg_ctx_done */
    printf("\tqdma fetches  0x%x pkt\r\n", mmio_read_32(DBGCNT_CFG+0xC+g_reg_off)); /* read dbg_ftx_done */
    printf("\tqdma receives 0x%x pkt\r\n", mmio_read_32(DBGCNT_CFG+0x14+g_reg_off)); /* read dbg_crx_done */
    printf("\tmmio_read_32(0x%x):0x%x\r\n", RXQ_CFG_BASE+0xC+g_reg_off, mmio_read_32(RXQ_CFG_BASE+0xC+g_reg_off));
    #endif
    return;
}

int qdma_blkCntTest_config (int d, unsigned int *dscp_idx_p, unsigned int dscp_idx_backup, int masks)
{
    int res=0;

    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)||(d==bctc_gdma_qdmalan)) {
    
        /* qdma_lan needs to handle 2 test cases, so make sure it hooks enough rx dscps */
        if (rxDSCP_HOOK_NUM < (((DMA_BLK_CASE*masks*CPU_WR_CASE)<<1)+2)) {
            printf("ERROR: %s need to hook more qdma rx dscps\r\n", dmaNames[d]);
            return -1;
        }

        *dscp_idx_p=0;
        if ((dscp_idx_backup!=0) && ((d==bctc_qdma_lan)||(d==bctc_gdma_qdmalan))) {
            /* if using qdma_lan for the 2nd time, uses next dscp_idx_p */
            *dscp_idx_p = dscp_idx_backup;
        }

        if (d==bctc_qdma_wan)
            res = qdma_loopback_init(1);
        else { /* bctc_qdma_lan and bctc_gdma_qdmalan */
            res = qdma_loopback_init(0);
        }
    }

    return res;
}

void enable_block_dma_mechanism(unsigned int enable)
{
    unsigned int reg = CR_BLOCK_EN;
    unsigned int val = mmio_read_32(reg);

    if(enable)
        val |= 0x1;
    else
        val &= (~0x1);

    mmio_write_32(reg, val);
}

void set_block_dma_mask(unsigned int mask)
{
    unsigned int reg = CR_BLOCK_MASK;
    unsigned int old_mask = mmio_read_32(reg);
    unsigned int val= 0xffffffff;

    val &= (~mask);
    mmio_write_32(reg, val);

    printf("old_blk_mask:0x%x, new_blk_mask:0x%x\r\n", old_mask, mmio_read_32(reg));
}

unsigned int get_block_dma_counter(int cnt_no)
{
    if ((cnt_no>=bctc_gdma_qdmalan) || (cnt_no<0)) {
        printf("cnt_no:%d is wrong! Correct value: 0~5\r\n", cnt_no);
        return 0;
    }

    return mmio_read_32(CR_BLOCK_CNT0+(cnt_no<<2));
}

void show_all_block_dma_cnt (void) 
{
    int i;

    printf("%s:\r\n", __func__);
    for (i=0; i<bctc_gdma_qdmalan; i++)
        printf("\t%s_blk_cnt: %d\r\n", dmaNames[i], get_block_dma_counter(i));
    return;
}

void enable_rbus_pending(int enable)
{
    unsigned int reg = CR_AXI2RBUS_CFG;
    unsigned int val = mmio_read_32(reg);

    if (enable)
        val |= (1<<1);
    else
        val &= (~(1<<1));
    
    mmio_write_32(reg, val);
    return;
}

void set_rbus_pending_addr(unsigned int addr)
{
    unsigned int val, val2;
    unsigned int reg = CR_AXI2RBUS_CFG;
    
    val = mmio_read_32(reg);
    val2 = val;
    
    val &= (~(RBUS_PENDING_ADDR_BITS<<2));
    val |= (((addr>>RBUS_PENDING_ADDR_SHIFT)&RBUS_PENDING_ADDR_BITS)<<2);

    mmio_write_32(reg, val);
    
    printf("old rbus_pending_addr:0x%x  new rbus_pending_addr:0x%x\r\n",
                ((val2>>2)&RBUS_PENDING_ADDR_BITS)<<RBUS_PENDING_ADDR_SHIFT, 
                ((mmio_read_32(reg)>>2)&RBUS_PENDING_ADDR_BITS)<<RBUS_PENDING_ADDR_SHIFT);
    return;
}

/* set pending time for matched address, 0 means forever */
void set_rbus_pending_cnt(unsigned int cnt_val)
{        
    mmio_write_32(CR_RBUS_PENDING_CNT, cnt_val);   
    return;
}

void set_gdma_config(unsigned int channel, unsigned int sa, unsigned int da, unsigned int ct0, unsigned int ct1)
{
    mmio_write_32(CR_GDMA_SA0+(channel<<4), sa);
    mmio_write_32(CR_GDMA_DA0+(channel<<4), da);
    mmio_write_32(CR_GDMA_CT10+(channel<<4), ct1);
    if (ct0&(1<<1)) /* enable bit */
        asm volatile ("dsb sy");
    mmio_write_32(CR_GDMA_CT00+(channel<<4), ct0);
    
    #if 0
    printf("CR_GDMA_SA0(0x%x):0x%x\n", CR_GDMA_SA0, mmio_read_32(CR_GDMA_SA0));
    printf("CR_GDMA_DA0(0x%x):0x%x\n", CR_GDMA_DA0, mmio_read_32(CR_GDMA_DA0));
    printf("CR_GDMA_CT10(0x%x):0x%x\n", CR_GDMA_CT10, mmio_read_32(CR_GDMA_CT10));
    printf("CR_GDMA_CT00(0x%x):0x%x\n", CR_GDMA_CT00, mmio_read_32(CR_GDMA_CT00));
    #endif
    
    return;
}

void trigger_gdma(unsigned int src, unsigned int dst, unsigned int len, unsigned int ch, unsigned int burst)
{
    int coherent_en = 0;

    //printf("GDMA moves data from src:0x%x to dst:0x%x (len:0x%x)\n", src, dst, len);

    set_gdma_config(ch, src, dst, ((len&0xffff)<<16)|(burst<<3)|(1<<1)|(1<<0), (coherent_en<<2));
   
    return;
}

void trigger_npu(unsigned int src, unsigned int dst, unsigned int len)
{
    int cnt=0;

	while ((mmio_read_32(CR_NPU_MIB8)!=2))
	{
		printf("wait npu ready ! CR_NPU_MIB8 is %d\r\n",mmio_read_32(CR_NPU_MIB8));
		delay1ms(1000);
        cnt++;
        if (cnt>5) {
            printf("ERROR: NPU is not ready\r\n");
            return;
        }
	}
	mmio_write_32(CR_NPU_MIB9, src);
	mmio_write_32(CR_NPU_MIB10, dst);
	mmio_write_32(CR_NPU_MIB11, len);
	mmio_write_32(CR_NPU_MIB8, 3);
	delay1ms(100);
    return;
}

void wait_gdma_done(unsigned int channel)
{
    /* wait until GDMA is done */
    while((mmio_read_32(CR_GDMA_DONE) & (1<<channel))== 0) ;
    /* clear done bit */
    mmio_write_32(CR_GDMA_DONE, (1<<channel)); 
    return;
}

void wait_npu_done (void)
{
    int cnt=0;

	/* wait until NPU is done*/
	while ((mmio_read_32(CR_NPU_MIB8)!=2))
	{
		delay1ms(10);
        cnt++;
        if (cnt>5) {
            printf("ERROR: wait for too long for NPU done\r\n");
            return;
        }
	}
	return;
}

void wakeup_npu_core0(void)
{
    #ifdef IS_FPGA_STAGE
    mmio_write_32(CR_NPU_MIB12, 1);
    #endif

	mmio_write_32(REG_CORE0_BOOT_BASE, NPU_SRAM_BASE);
	mmio_write_32(REG_CORE_BOOT_CONFIG,1);
	mmio_write_32(REG_CORE_BOOT_TRIGGER,1);
	
	/* wait NPU up*/
	delay1ms(100);

	/* set npu test case*/
	mmio_write_32(CR_NPU_MIB8,45);

}

void show_block_dbg_regs(void)
{
    unsigned int offset;

    printf("%s:\r\n", __func__);
    for (offset=0x110; offset<=0x134; ) {
        printf("\t0x%x: 0x%x\r\n", RBUS_CORE_BASE+offset, mmio_read_32(RBUS_CORE_BASE+offset));
        offset+=4;
    }
    
    return;
}

/* phase==1: before gdmq data moving,  phase==2: after gdmq data moving*/
int check_dma_block_cnt(int d, int e, int i, int phase)
{
    unsigned int cur_block_cnt=0, pre_block_cnt=0, block_cnt_diff;
    unsigned int cur_block_cnt2=0, pre_block_cnt2=0, block_cnt_diff2;
    int count_down=3;
    int dma;

    if (d==bctc_gdma_qdmalan)
        dma = bctc_gdma;
    else
        dma = d;

    if ((phase==1) && (e==1) && (i<2)) {

        /* make sure that dma_block_cnt is counting */
        cur_block_cnt = get_block_dma_counter(dma);
        if (d==bctc_gdma_qdmalan)
            cur_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
         
        while(1) {
            delay1us(1);
            pre_block_cnt = cur_block_cnt;
            cur_block_cnt = get_block_dma_counter(dma);
            block_cnt_diff = cur_block_cnt-pre_block_cnt;
            block_cnt_diff2 = 1; /* for easier comparison later */
            if (d==bctc_gdma_qdmalan) {
                pre_block_cnt2 = cur_block_cnt2;
                cur_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
                block_cnt_diff2 = cur_block_cnt2-pre_block_cnt2;
            }
            if ((block_cnt_diff>0) && (block_cnt_diff2>0))
                break;
            else {
                if (count_down>0) {
                    count_down--;
                }
                else {
                    printf("ERROR1: %s_block_cnt is not counting\r\n", dmaNames[d]);
                    printf("  --pre_block_cnt:0x%x  cur_block_cnt:0x%x\r\n", pre_block_cnt, cur_block_cnt);
                    if (d==bctc_gdma_qdmalan)
                        printf("  --pre_block_cnt2:0x%x  cur_block_cnt2:0x%x\r\n", pre_block_cnt2, cur_block_cnt2);
                    return -1;
                }
            }
        }

        printf("%s block cnt:0x%x in 1 us\r\n", dmaNames[dma], block_cnt_diff);
        if (d==bctc_gdma_qdmalan)
            printf("qdma_lan block cnt:0x%x in 1 us\r\n", block_cnt_diff2);
    }
    else { /* make sure that dma_block_cnt won't count */

        pre_block_cnt = get_block_dma_counter(dma);
        if (d==bctc_gdma_qdmalan) {
            pre_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
        }
        delay1us(100);
        cur_block_cnt = get_block_dma_counter(dma);
        if (d==bctc_gdma_qdmalan) {
            cur_block_cnt2 = get_block_dma_counter(bctc_qdma_lan);
        }
        
        if ((pre_block_cnt != cur_block_cnt) || (pre_block_cnt2 != cur_block_cnt2)) {
            printf("\r\nERROR2: [%s] pre_block_cnt:0x%x != cur_block_cnt:0x%x\r\n", 
                    dmaNames[dma], pre_block_cnt, cur_block_cnt);
            if (d==bctc_gdma_qdmalan)
                printf("\tqdma_lan pre_block_cnt2:0x%x != cur_block_cnt2:0x%x\r\n", pre_block_cnt2, cur_block_cnt2);
            return -1;
        }
        
        //printf("dma block cnt is not counting which is correct\n");
    }


    return 0;
}

int read_compare_data(int d, int e, int i, unsigned int cmp_addr, unsigned int cmp_val, int flag)
{
    unsigned dst_data;

    if (d==bctc_gdma_qdmalan) {
        if (flag==0)
            d=bctc_gdma;
        else
           d=bctc_qdma_lan; 
    }

    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) { /* qdma will move data to rxBuf+2 */
        #ifdef LITTLE_ENDIAN
        dst_data = ((mmio_read_32(cmp_addr+4)&0xffff)<<16) | ((mmio_read_32(cmp_addr)>>16)&0xffff);
        #else
        dst_data = ((mmio_read_32(cmp_addr)&0xffff)<<16) | ((mmio_read_32(cmp_addr+4)>>16)&0xffff);
        #endif
    }
    else
        dst_data = mmio_read_32(cmp_addr);

    if ((e==1) && (i<2)) { /* when block is enabled, dma-read should happen "after" cpu-write, so comparison should succeed */
        if (dst_data != cmp_val) {
            printf("ERROR3: mmio_read_32(0x%x):0x%x != cmp_val:0x%x\r\n", cmp_addr, dst_data, cmp_val);
            if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) {
                printf("\tNote: mmio_read_32(0x%x):0x%x  mmio_read_32(0x%x):0x%x\r\n", 
                        cmp_addr, mmio_read_32(cmp_addr), cmp_addr+4, mmio_read_32(cmp_addr+4));
            }
            return -1;
        }
    }
    if ((e==0) && (i<2)) { /* when block is disabled, dma-read should happen "before" cpu-write, so comparison should fail */
        if (dst_data == cmp_val) {
            printf("ERROR4: mmio_read_32(0x%x):0x%x == cmp_val:0x%x\r\n", cmp_addr, dst_data, cmp_val);
            if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) {
                printf("\tNote: mmio_read_32(0x%x):0x%x  mmio_read_32(0x%x):0x%x\r\n", 
                        cmp_addr, mmio_read_32(cmp_addr), cmp_addr+4, mmio_read_32(cmp_addr+4));
            }
            return -1;
        }
    }
    return 0;
}
#if 0
void transXmodem(unsigned int dst_addr)
{
	int size = 0;
	int err;
	int res;
	connection_info_t info;
	char ymodemBuf[XMODEM_BUFFER_SIZE];
	ulong store_addr = ~0;
	ulong addr = 0;
	
	info.mode = xyzModem_xmodem;
	res = xyzModem_stream_open(&info, &err);
	if (!res)
	{
	
		while ((res = xyzModem_stream_read(ymodemBuf, XMODEM_BUFFER_SIZE, &err)) > 0)
		{
			store_addr = addr + dst_addr;
			size += res;
			addr += res;
	
			memcpy((char *)(store_addr), ymodemBuf, res);
		}
	}
	else
	{
		printf("%s\n", xyzModem_error(err));
	}
	
	xyzModem_stream_close(&err);
    #if defined(CONFIG_TPL_BUILD)
	xyzModem_stream_terminate(false, &getcxmodem);
    #endif
}

void prepare_npu_block_test(void)
{
	transXmodem(NPU_SRAM_BASE);
	delay1us(100);
	transXmodem(NPU_16K_SRAM_BASE);
	delay1us(100);
}
#endif
void dma_block_cnt_test(void)
{
    int i, m, e, d;
    unsigned int test_len, test_offset;
    unsigned int block_mask[] = {0x3f, 0x7f};
    int masks = sizeof(block_mask)/sizeof(block_mask[0]);
    unsigned int wtVal = 0x12345678;
    unsigned int wtVal2 = 0x23456789;
    unsigned int dma_src_addr, dma_dst_addr, cpu_wt_addr;
    unsigned int dma_src_addr2=0, dma_dst_addr2=0, cpu_wt_addr2=0;
    unsigned int dscp_idx, dscp_idx_backup=0;


	/* set NPU SRAM to non secure*/
    unsigned int val = 0;
	val = mmio_read_32(0x1FBE2E04);
	val |= ((1<<12));
	mmio_write_32(0x1FBE2E04, val);
	asm volatile ("dsb sy");
    
    #if 1
    printf("Use CodeViser to load npu_rv32.bin to 0x%x and npu_data.bin to 0x1e900000, then set 0x1fb00280 as 1 to start the test\r\n",
            (NPU_SRAM_BASE&0x1fffffff));
    while (mmio_read_32(0x1fb00280)==0);
    #else
	/*put npu bin to npu_sram by XMODEM*/
	prepare_npu_block_test();
    #endif
    /* set pending (high-8bits) address */
    set_rbus_pending_addr(DRAM_TEST_BASE);
    /* set pending time for matched address, 0 means forever */
    set_rbus_pending_cnt(0);
    /* init apb timer 2 */
    apb_timer_init(2, 1, 10);

    /*
     * d==(0,1,2,3,4,5,6) for (gdma,ppe,qdma_lan,qdma_wan,tdma,npu,gdma+qdmaLan) respectively
     * e==1 for enabling, e==0 for disabling block mechanism.
     * m==0 for 64B, m==1 for 128B block range.
     * i==0 or 1 for block counting case, i==2 for block not counting case.
     *
     * in case (e,m,i)==(1,0,0), cpu writes to addr:0x00 and dma reads addr:0x0~0x3f,  so block should heppen. 
     * in case (e,m,i)==(1,0,1), cpu writes to addr:0x20 and dma reads addr:0x0~0x3f, so block should heppen. 
     * in case (e,m,i)==(1,0,2), cpu writes to addr:0x40 and dma reads addr:0x0~0x3f, so block should not heppen. 
     */
    for (d=0; d<bctc_max_case; d++) { 

        #ifdef TCSUPPORT_CPU_AN7583
		/* tdma is tested in kernel. qdma_rx only write DRAM, so no need to test. */
        if ((d==bctc_tdma)||(d==bctc_qdma1_rx)||(d==bctc_qdma2_rx)) continue;
        #else
        if ((d==bctc_ppe)||(d==bctc_tdma)) continue; /* not supported yet */
		#endif

        if (qdma_blkCntTest_config (d, &dscp_idx, dscp_idx_backup, masks))
            return;

        if (d == bctc_npu) { /* wake up NPU core0 */
			wakeup_npu_core0();
        }

        for (e=(DMA_BLK_CASE-1); e>=0; e--) {
            for (m=0; m<masks; m++) {
                for (i=0; i<CPU_WR_CASE; i++) {
                    
                    printf("%s at (d=%d, e=%d,m=%d,i=%d)\r\n", dmaNames[d],d,e,m,i);
            
                    test_len = block_mask[m]+1;
                    test_offset = (test_len>>1);
                    cpu_wt_addr = DRAM_TEST_BASE+(test_offset*i);
                    dma_src_addr = DRAM_TEST_BASE;
                    dma_dst_addr = DRAM_TEST_BASE+test_len;
                    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)) {
                        /* replace dma_dst_addr */
                        dma_dst_addr = RXPKT_BASE + (dscp_idx*RX_PKT_MAX_SIZE) +g_buf_off;
                    }
                    else if (d==bctc_gdma_qdmalan) {
                        /* xxx_xxx_addr is for gdma, xxx_xxx_addr2 is for qdmalan */
                        cpu_wt_addr2 = DRAM_TEST_BASE+(test_offset*i)+(test_offset<<2);
                        dma_src_addr2 = DRAM_TEST_BASE+(test_offset<<2);
                        dma_dst_addr2 = RXPKT_BASE + (dscp_idx*RX_PKT_MAX_SIZE) +g_buf_off;
                    }
                    else {}
                    

                    /* qdma will read 16 more bytes from source area, so cpu should write to the 3rd test_len area 
                     * in case (i==2) that qdma blcok counters don't want to be triggered */
                    if ((i==2) && ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)||(d==bctc_gdma_qdmalan))) {
                        if (d==bctc_gdma_qdmalan)
                            cpu_wt_addr2 += test_len;
                        else
                            cpu_wt_addr += test_len;
                    }
                    
                    /* reset dma src & dst areas */
                    memset((void*)dma_src_addr, 0, test_len);
                    memset((void*)dma_dst_addr, 0, test_len);
                    if (d==bctc_gdma_qdmalan) {
                        memset((void*)dma_src_addr2, 0, test_len);
                        memset((void*)dma_dst_addr2, 0, test_len);
                    }
                    asm volatile ("dsb sy");

                    enable_block_dma_mechanism(DISABLE);
                    
                    if (e) {
                        set_block_dma_mask(block_mask[m]);
                        enable_block_dma_mechanism(ENABLE);
                    }
                    else {
                        printf("block disable case for m=%d,i=%d\r\n", m,i);
                    }

                    printf("test_len:0x%x  cpu_wt_addr:0x%x  dma_dst_addr:0x%x\r\n", test_len, cpu_wt_addr, dma_dst_addr);
                    if (d==bctc_gdma_qdmalan)
                        printf("\t\tcpu_wt_addr2:0x%x  dma_dst_addr2:0x%x\r\n", cpu_wt_addr2, dma_dst_addr2);

                    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan))
                        qdma_tx_dscp_hook(dscp_idx, dma_src_addr, test_len);
                    else if (d==bctc_gdma_qdmalan)
                        qdma_tx_dscp_hook(dscp_idx, dma_src_addr2, test_len);
                    else {}

					memset((void*)dma_src_addr, 0, test_len); //workaround: qdma's behavior may affect rbus_pending's circuit
                    /* enable pending to make sure that cpu-write can stay in Wbuff when dma-read happens */ 
                    enable_rbus_pending(ENABLE);
                    asm volatile ("dsb sy");

                    /* cpu writes */
                    mmio_write_32(cpu_wt_addr, wtVal);
                    if (d==bctc_gdma_qdmalan)
                        mmio_write_32(cpu_wt_addr2, wtVal2);

                    /* trigger DMA */
                    
                    switch (d) {
                        case bctc_gdma:
                        case bctc_gdma_qdmalan:
                            /* gdma-coherent is disabled here, otherwise "i==2" case will fail, because 
                             * if gdma-coherent is enabled, after gdma finishes writing to dst area, 
                             * it will uncached read back last byte of dst area which matches cpu_wt_addr's
                             * 64-byte-masked value. */
                            #ifdef TCSUPPORT_CPU_AN7583
                            /* 7583 qdma uses p6 to access Dscp (if DScp is on DRAM). 7583 gdma uses p1 to
                             * move data on DRAM. p1 and p6 share the same port to access DRAM. 
                             * if enabling gdma then qdma, under the condition that rbus pending is enabled,
                             * qdma's read on Dscp on DRAM will be blocked by gdma's data read on DRAM,
                             * because gdma's data read is currectly blocked by CPU's write on WriteBuffer.
                             * As results, qdma has no chance to use p17 to read data on DRAM, which causes
                             * qdma1_tx block_cnt not counting in check_dma_block_cnt(). 
                             * Therefore, need to enable qdma then gdma to prevent this case. */
                            if (d==bctc_gdma_qdmalan)
                                trigger_qdma(dscp_idx);
                            trigger_gdma(dma_src_addr, dma_dst_addr, test_len, 0, 0);
                            break;
                            
                            #else
                            trigger_gdma(dma_src_addr, dma_dst_addr, test_len, 0, 0);
                            if (d!=bctc_gdma_qdmalan) /* bctc_gdma_qdmalan go head */
                                break;
                            #endif

                        case bctc_qdma_lan:
                        case bctc_qdma_wan:
                            trigger_qdma(dscp_idx);
                            break;
                            
                        case bctc_npu:
							trigger_npu(dma_src_addr, dma_dst_addr, test_len);
							break;

                        default:
                            printf("unknown dma_type:%d\r\n", d);
                            return;
                    }

                    #if 0 /* more dbg info */
                    show_block_dbg_regs();
                    show_all_block_dma_cnt();
                    #endif

                    if (check_dma_block_cnt(d, e, i, 1))
                        goto dma_block_cnt_test_end;

                    /* disable rbus_cmd_pending to let the cpu-wt-cmd out, otherwise, 
                     * dma-read will be blocked by cpu-wt-cmd forever. */
                    enable_rbus_pending(DISABLE);

                    /* wait until DMA done */

                    switch (d) {
                        case bctc_gdma:
                        case bctc_gdma_qdmalan:
                            wait_gdma_done(0);
                            if (d!=bctc_gdma_qdmalan) /* bctc_gdma_qdmalan go head */
                                break;

                        case bctc_qdma_lan:
                        case bctc_qdma_wan:
                            delay1ms(10);
                            break;

                        case bctc_npu:
							wait_npu_done();
							break;

                        default:
                            return;
                    }

                    if (check_dma_block_cnt(d, e, i, 2))
                        goto dma_block_cnt_test_end;

                    if (read_compare_data(d, e, i, dma_dst_addr+(test_offset*i), wtVal, 0))
                        goto dma_block_cnt_test_end;

                    if (d==bctc_gdma_qdmalan) {
                        if (read_compare_data(d, e, i, dma_dst_addr2+(test_offset*i), wtVal2, 1))
                            goto dma_block_cnt_test_end;
                    }

                    if ((d==bctc_qdma_lan)||(d==bctc_qdma_wan)||(d==bctc_gdma_qdmalan))
                        dscp_idx++;

                    printf("\n");
            
                } /* i loop */
            } /* m loop */
        } /* e loop */

        if ((dscp_idx_backup==0) && ((d==bctc_qdma_lan)||(d==bctc_gdma_qdmalan))) /* save dscp_idx for using qdma_lan again */
            dscp_idx_backup = dscp_idx;
        
        printf("%s block_cnt_test done\r\n\r\n", dmaNames[d]);
    
    } /* d loop */

    printf("\r\nALL dma_block_cnt_tests PASS !!\r\n");
    return;

dma_block_cnt_test_end:
    printf("\r\nERROR for %s at (e=%d,m=%d,i=%d)\r\n", dmaNames[d], e , m, i);
    
    return;
}

/* 
 * if dma_burst==64 bytes and dma_block_mask_bytes==32, when cpu writes a 64-byte region,
 * the block mechanism can only block dma for 1st half of the region, and dma will read
 * 2nd half of the region before cpu writes to it, which makes final result wrong! 
 * Therefore, dma_block_mask_bytes must equal to or be larger than dma_burst 
 */
void dma_block_mask_test(void)
{
    int i, m;
    unsigned int test_len, test_words;
    unsigned int block_mask[] = {0x1f, 0x3f};
    int masks = sizeof(block_mask)/sizeof(block_mask[0]);
    unsigned int wtVal = 0x12345678, rdVal, cmpVal;
    unsigned int dma_src_addr, dma_dst_addr;
    unsigned int *cpu_wt_ptr, *cmp_rd_ptr;
    unsigned int block_cnt0, block_cnt1;

    printf("\r\n%s start ... \r\n", __func__);

    test_len = block_mask[1]+1; /* 64 bytes */
    dma_src_addr = DRAM_TEST_BASE;
    dma_dst_addr = DRAM_TEST_BASE+test_len;
    test_words = test_len/sizeof(unsigned int);

    /* set pending (high-8bits) address */
    set_rbus_pending_addr(DRAM_TEST_BASE);
    /* set pending time for matched address, 0 means forever */
    set_rbus_pending_cnt(100000);

    for (m=0; m<masks; m++) {

        /* reset dma src & dst areas */
        memset((void*)dma_src_addr, 0, test_len);
        memset((void*)dma_dst_addr, 0, test_len);
        asm volatile ("dsb sy");

        /* set dma block mask */
        enable_block_dma_mechanism(DISABLE);
        set_block_dma_mask(block_mask[m]);
        enable_block_dma_mechanism(ENABLE);

        /* enable pending to make sure that cpu-write can stay in Wbuff when dma-read happens */ 
        enable_rbus_pending(ENABLE);
        asm volatile ("dsb sy");

        /* cpu writes to src region */
        cpu_wt_ptr = (unsigned int *)dma_src_addr;
        for (i=0; i<test_words; i++, cpu_wt_ptr++) {
            if (i&0x1) continue; /* writeBuff can only queue 8 packets, so skip some. */
            mmio_write_32((unsigned int)cpu_wt_ptr, wtVal+i);
        }

        /* trigger GDMA, burst:0/1/2/3/4 means 1/2/4/8/16 DW which is 4/8/16/32/64 bytes */
        trigger_gdma(dma_src_addr, dma_dst_addr, test_len, 0, 4);

        /* make sure that gdma is blocked by cpu_write */
        block_cnt0 = get_block_dma_counter(bctc_gdma);
        delay1us(100);
        block_cnt1 = get_block_dma_counter(bctc_gdma);
        if (block_cnt0==block_cnt1) {
            printf("\r\nError: gdma block cnt is not counting! (cnt0:0x%x, cnt1:0x%x, gdma_cnt0x%x)\r\n", 
                    block_cnt0, block_cnt1, get_block_dma_counter(bctc_gdma));
            return;
        }

        /* disable rbus_cmd_pending to let the cpu-wt-cmd out, otherwise, 
         * dma-read will be blocked by cpu-wt-cmd forever. */
        enable_rbus_pending(DISABLE);

        /* wait until DMA done */
        wait_gdma_done(0);

        /* read and compare */
        cmp_rd_ptr = (unsigned int *)dma_dst_addr;
        for (i=0; i<test_words; i++, cmp_rd_ptr++) {
            if (i&0x1) continue; /* writeBuff can only queue 8 packets, so skip some. */
            rdVal = mmio_read_32((unsigned int)cmp_rd_ptr);
            if ((m==0) && (i>=(test_words>>1))) {
                /* when m==0, that is, dma_burst(64 bytes) > dma_block_mask_bytes(32 bytes), 
                 * the block mechanism can only block dma for 1st half of the region(byte0~31), 
                 * and dma will read 2nd half of the region(byte32~63) before cpu writes to it,
                 * so the expected result is 0 */
                cmpVal = 0;
            }
            else {
                cmpVal = wtVal+i;
            }
            if (rdVal != cmpVal) {
                printf("\r\nError: rdVal:0x%x != cmpVal:0x%x at (i:%d, m:%d, cmp_addr:0x%x)\r\n", 
                                    rdVal, cmpVal, i, m, (unsigned int)cmp_rd_ptr);
                return;
            }
        }
    }

    printf("\r\n%s Done !\r\n", __func__);
    return;
}
#endif

#ifdef TEST_RBUS_TIMEOUT
#define DRAM_TEST_ADDR		(0x84000000)

#define RBUS_TIMEOUT_BASE	(0x1fa00000)
#define FORCE_TIMEOUT		(RBUS_TIMEOUT_BASE + 0xbc)
#define TIMEOUT_STS0		(RBUS_TIMEOUT_BASE + 0xd0)
#define TIMEOUT_STS1		(RBUS_TIMEOUT_BASE + 0xd4)
#define TIMEOUT_CFG0		(RBUS_TIMEOUT_BASE + 0xd8)
#define TIMEOUT_CFG1		(RBUS_TIMEOUT_BASE + 0xdc)
#define TIMEOUT_CFG2		(RBUS_TIMEOUT_BASE + 0xe0)

#define CR_SCREG_WF0        (0x1FB00240)

#define CR_TIMER1_LVR       (CR_TIMER_BASE + 0x0C)
#define CR_TIMER1_CVR       (CR_TIMER_BASE + 0x10)
#define CR_TIMER3_LVR       (CR_TIMER_BASE + 0x2C)
#define CR_TIMER3_CVR       (CR_TIMER_BASE + 0x30)
#define NPU_WDOG_THLD       (CR_TIMER_BASE + 0x34)
#define NPU_WDOG_RLD        (CR_TIMER_BASE + 0x38)

#define CPU_TIMER_BASE      (0x1fbf0400)
#define CPU_TIMER_CTRL      (CPU_TIMER_BASE + 0x00)
#define CPU_TIMER0_CMP      (CPU_TIMER_BASE + 0x04)
#define CPU_TIMER0_CVR      (CPU_TIMER_BASE + 0x08)
#define CPU_TIMER1_CMP      (CPU_TIMER_BASE + 0x0c)
#define CPU_TIMER1_CVR      (CPU_TIMER_BASE + 0x10)


unsigned int get_cpuTmr0_cnt(void)
{
    return mmio_read_32(CPU_TIMER0_CVR);
}

unsigned int get_msTime_by_cpuTmrCnt(unsigned int cpuTmrCnt)
{
    return (cpuTmrCnt/(APB_CPUTMR_CLK*1000));
}

unsigned int get_cpuTmrTime_by_msTime (unsigned int msTime)
{
    return (get_1ms_timeTick_by_timeClk(APB_CPUTMR_CLK)*msTime);
}

void cpu_timer_init(unsigned int tmr, unsigned int enable, unsigned int cmpVal/*ms*/)
{
    unsigned int regVal;

    if (tmr>2)
        printf("%s cpu_tmr:%d is wrong, should be 0 or 1\r\n", __func__, tmr);

    if (enable) {
        
        /*set compare value and reset current value */
        mmio_write_32(CPU_TIMER0_CMP+(tmr<<3), get_cpuTmrTime_by_msTime(cmpVal));
        mmio_write_32(CPU_TIMER0_CVR+(tmr<<3), 0);
        
        /* enable tmr */
        regVal = mmio_read_32(CPU_TIMER_CTRL);
        regVal |= (1<<tmr);
        mmio_write_32(CPU_TIMER_CTRL, regVal);
    }
    else {

        /* disable tmr */
        regVal = mmio_read_32(CPU_TIMER_CTRL);
        regVal &= (~(1<<tmr));
        mmio_write_32(CPU_TIMER_CTRL, regVal);
    }

    return;
}

void wdog_tmr3_init (unsigned int enable, unsigned int timeOut/*ms*/, unsigned int wdogThld/*ms*/)
{
    unsigned int tmr=3;
    unsigned int regVal;

    apb_timer_init(tmr, enable, timeOut);

    if (enable) {
        
        /* set wdog threshold. when the threshold is reached, wdog will issue interrupt. */
        mmio_write_32(NPU_WDOG_THLD, get_tmrTick_by_msTime(wdogThld));

        /* clear tmr3 first due to being enabled in timer_init eariler */
        regVal = mmio_read_32(CR_TIMER_CTL);
        regVal &= (~(1<<5)); 
        mmio_write_32(CR_TIMER_CTL, regVal);
        /* enable wdog on tmr3 */
        regVal = mmio_read_32(CR_TIMER_CTL);
        regVal |= ((1<<5)|(1<<25)); 
        mmio_write_32(CR_TIMER_CTL, regVal);
    }

    return;
}

void rbus_timeout_test(int test_index)
{
	unsigned int read_data =0 ;
	unsigned int addr = 0;
	unsigned int time_config = 0;
    unsigned int timeout_config = 0;
	int i = 0;
	unsigned int time_start = 0;
	unsigned int time_end = 0;
    unsigned int emi_clk=EMI_CLK_DOMAIN;
    #if defined(TCSUPPORT_CPU_EN7581) && !defined(IS_FPGA_STAGE)
    unsigned int emi_clksrc[] = {540, 480, 400, 300};   /*0x1fa201b8[9:8]*/
    unsigned int regVal, clksrc, clkdiv;
    #endif

	printf ("test index = %d\r\n", test_index);
    
	/* In case of EN7523 timer reg would not be reset when reboot, set the regs to default before tests*/
	mmio_write_32(TIMEOUT_STS0, 0);
	mmio_write_32(TIMEOUT_STS1, 0);

    #if defined(TCSUPPORT_CPU_EN7581) && !defined(IS_FPGA_STAGE)
    if ((test_index == 4)||(test_index == 5)) {
        regVal = mmio_read_32(CR_CHIP_SCU_EMI_CLK);
        clksrc = (regVal>>8)&0x3;
        clkdiv = regVal&0x7;
        emi_clk = (emi_clksrc[clksrc]/(clkdiv+1))<<20;
        //printf("\emi_clk: %d MHz\r\n", (emi_clk>>20));
    }
    #endif

	if (test_index == 1)
	{
		printf("Rbus timeout disable Test\r\n");
		printf("0x1fa000d0 = %x\r\n", mmio_read_32(TIMEOUT_STS0));
		printf("Force bus timeout when rbus timeout is disabled!\r\n");
		mmio_write_32(FORCE_TIMEOUT, 0);
		printf("Trying to read data from DRAM\r\n\r\n");
		/* System will hang here */
		read_data = mmio_read_32(DRAM_TEST_ADDR);
		/* this should not be excuted*/
		printf("Error!\r\n");
		
	}
	else if (test_index == 2)
	{
		printf("Rbus timeout trigger by read command Test\r\n");
		mmio_write_32(TIMEOUT_STS0, 0x80000000);
		printf("0x1fa000d0 = %x\r\n", mmio_read_32(TIMEOUT_STS0));
		printf("Force bus timeout\r\n");
		mmio_write_32(FORCE_TIMEOUT, 0);
		printf("Read data from DRAM 0x84000000\r\n");
		read_data = mmio_read_32(DRAM_TEST_ADDR);

		printf("CPU BUS Test: action: Read\r\n");
		printf("read_data = %x; rbus timeout status = %x ; errAddr = %x \r\n",read_data, mmio_read_32(TIMEOUT_STS0), mmio_read_32(TIMEOUT_STS1));
	}
	else if (test_index == 3)
	{
		printf("Rbus timeout trigger by write command Test\r\n");
		mmio_write_32(TIMEOUT_STS0, 0x80000000);
		mmio_write_32(TIMEOUT_CFG0, 0x400);
		printf("0x1fa000d0 = %x\r\n", mmio_read_32(TIMEOUT_STS0));
		printf("Force bus timeout\r\n");
		mmio_write_32(FORCE_TIMEOUT, 0);
		printf("Write data to DRAM 0x84000000\r\n");
		mmio_write_32(DRAM_TEST_ADDR, 0x12345678); //write data to dram
		/* wait write cmd into bus*/
		i = 0;
		time_start = get_cpuTmr0_cnt();
		while(mmio_read_32(TIMEOUT_STS1)!= DRAM_TEST_ADDR)
		{
			i++;
		}
		time_end = get_cpuTmr0_cnt();
		printf("CPU BUS Test: action: Write\r\n");
		printf("loop %d times; wait %dms; rbus timeout status = %x ; errAddr = %x ; TIMEOUT_CFG0 = %x \r\n"
                ,i, get_msTime_by_cpuTmrCnt(time_end - time_start),mmio_read_32(TIMEOUT_STS0), mmio_read_32(TIMEOUT_STS1),mmio_read_32(TIMEOUT_CFG0));

	}
	else if ( test_index == 4)
	{
		printf("Rbus timeout cmd cnt Read Test\r\n");
		time_config = 1;
		mmio_write_32(0x1EFBC800, 0); //set to non bufferable
		for (time_config = 1 ; time_config < 6; time_config ++)
		{
		    timeout_config = emi_clk * time_config;
			mmio_write_32(TIMEOUT_CFG0, timeout_config);
            mmio_write_32(TIMEOUT_CFG1, timeout_config);
            mmio_write_32(TIMEOUT_CFG2, timeout_config);
			mmio_write_32(TIMEOUT_STS0, 0x80000000);
			addr = DRAM_TEST_ADDR + (0x100 * time_config);
			
			printf("0x1fa000d8 = %x , read_addr = %x, timer config = %d sec(s)\r\n",mmio_read_32(TIMEOUT_CFG0), addr, time_config);
			mmio_write_32(FORCE_TIMEOUT, 0);
			printf("force timeout\r\n");
			time_start = get_cpuTmr0_cnt();
			addr = mmio_read_32(addr);
			time_end = get_cpuTmr0_cnt();
			printf("CPU BUS Test: \r\n");
			printf("rbus timeout status = %x ; errAddr = %x ; wait time = %dms\r\n", 
                mmio_read_32(TIMEOUT_STS0), mmio_read_32(TIMEOUT_STS1), get_msTime_by_cpuTmrCnt(time_end - time_start));
		}
	}
	else if (test_index == 5)
	{
		printf("Rbus timeout cmd cnt Write Test\r\n");

		mmio_write_32(0x1EFBC800, 0); //set to non bufferable
		for (time_config = 1 ; time_config < 6; time_config ++)
		{
		    timeout_config = emi_clk * time_config;
			mmio_write_32(TIMEOUT_CFG0, timeout_config);
            mmio_write_32(TIMEOUT_CFG1, timeout_config);
            mmio_write_32(TIMEOUT_CFG2, timeout_config);
			mmio_write_32(TIMEOUT_STS0, 0x80000000);
			addr = DRAM_TEST_ADDR + (0x100 * time_config);			
		
			printf("0x1fa000d8 = %x , write_addr = %x, timer config = %d sec(s)\r\n",mmio_read_32(TIMEOUT_CFG0), addr, time_config);
			mmio_write_32(FORCE_TIMEOUT, 0);
			printf("force timeout\r\n");
			time_start = get_cpuTmr0_cnt();
			mmio_write_32(addr, 0x12345678);
			i = 0;
			/* wait write cmd into bus*/
			while(mmio_read_32(TIMEOUT_STS1)!= addr)
			{
				i++;
			}
			time_end = get_cpuTmr0_cnt();
			printf("CPU BUS Test: \r\n");
			printf("loop %d times; rbus timeout status = %x ; errAddr = %x ; wait time = %dms\r\n", 
                i,mmio_read_32(TIMEOUT_STS0), mmio_read_32(TIMEOUT_STS1), get_msTime_by_cpuTmrCnt(time_end - time_start));
			
		}
	}
    else {
        printf("\r\nERROR: wrong test_index:0x%d, should be 1~5\r\n", test_index);
    }

    return;
}

void rbus_timeout_tests_all(void)
{
    unsigned int val;

    
    val=mmio_read_32(CR_SCREG_WF0);
    val++;
    mmio_write_32(CR_SCREG_WF0, val);

    if (val<=5) {

        if (val<=3)
            wdog_tmr3_init(1, 3000, 3000);
        else
            wdog_tmr3_init(1, 18000, 18000);
        
        cpu_timer_init(0, 1, 10000);
        
        rbus_timeout_test(val);
        
        printf("wait for wdog reboot\r\n\r\n");
        while(1);
    }

    printf("%s Done\r\n", __func__);

    return;
}
#endif

#ifdef TEST_RBUS_DRAM_CNT
#define REG_DRAM_RBUS_WCMD_CNT      (0x1fb00960)
#define REG_DRAM_RBUS_WCMD_CNT_CLR  (REG_DRAM_RBUS_WCMD_CNT+0x4)
#define REG_DRAM_RBUS_RCMD_CNT      (REG_DRAM_RBUS_WCMD_CNT+0x8)
#define REG_DRAM_RBUS_RCMD_CNT_CLR  (REG_DRAM_RBUS_WCMD_CNT+0xc)
#define NUM_DRAM_RBUS_REG           (4)
#define NUM_DRAM_RBASE              (3)
#define DRAM_TSIZE                  (0x100000)
#define DRAM_BASE0                  (0x80000000)
#define DRAM_BASE1                  (0x90000000)
#define DRAM_BASE2                  (0xA0000000-DRAM_TSIZE)

void rbus_dram_access_cnt_test(void)
{
    unsigned int base_addr[NUM_DRAM_RBASE] = {DRAM_BASE0, DRAM_BASE1, DRAM_BASE2};
    unsigned int defaultVal[NUM_DRAM_RBUS_REG] = {0,1,0,1};
    int i, j;
    unsigned int words = DRAM_TSIZE/sizeof(unsigned int);
    unsigned int addr, val, cnt, val_base;
    unsigned char bVal;

    printf("%s start ...\r\n", __func__);


    /* clear counters */
    mmio_write_32(REG_DRAM_RBUS_WCMD_CNT_CLR, 1);
    mmio_write_32(REG_DRAM_RBUS_RCMD_CNT_CLR, 1);

    /*check default values */
    for (i=0; i<NUM_DRAM_RBUS_REG; i++) {
        if (mmio_read_32(REG_DRAM_RBUS_WCMD_CNT+(i<<2)) != defaultVal[i]) {
            printf("%s FAILED1 due to read(0x%x):0x%x != defaultVal[%d]:0x%x \r\n", 
                __func__, (REG_DRAM_RBUS_WCMD_CNT+(i<<2)), mmio_read_32(REG_DRAM_RBUS_WCMD_CNT+(i<<2)), i, defaultVal[i]);
            return;
        }
    }

    /* write/read DRAM and check counters every word */
    for (j=0; j<NUM_DRAM_RBASE; j++) {
        val_base = j*words;
        for (i=0; i<words; i++) {
            addr = base_addr[j]+(i<<2);
            val = 0x12345678+i+val_base;
            cnt = i+1+val_base;
            mmio_write_32(addr, val);
            if (mmio_read_32(REG_DRAM_RBUS_WCMD_CNT) != cnt) {
                printf("%s FAILED2 due to read(WCMD_CNT):0x%x != cnt:0x%x at i:%d,j:%d\r\n", 
                    __func__, mmio_read_32(REG_DRAM_RBUS_WCMD_CNT), cnt, i, j);
                return;
            }
            if (mmio_read_32(addr) != val) {
                printf("%s FAILED3 due to read(0x%x):0x%x != val:0x%x at i:%d,j:%d\r\n", 
                    __func__, addr, mmio_read_32(addr), val, i, j);
                return;
            }
            if (mmio_read_32(REG_DRAM_RBUS_RCMD_CNT) != cnt) {
                printf("%s FAILED4 due to read(RCMD_CNT):0x%x != cnt:0x%x at i:%d,j:%d\r\n", 
                    __func__, mmio_read_32(REG_DRAM_RBUS_WCMD_CNT), cnt, i, j);
                return;
            }
        }
    }

    /* clear counters */
    mmio_write_32(REG_DRAM_RBUS_WCMD_CNT_CLR, 1);
    if (mmio_read_32(REG_DRAM_RBUS_WCMD_CNT) != 0) {
        printf("%s FAILED5 due to read(WCMD_CNT):0x%x != 0 after clear\r\n", __func__, mmio_read_32(REG_DRAM_RBUS_WCMD_CNT));
        return;
    }
    mmio_write_32(REG_DRAM_RBUS_RCMD_CNT_CLR, 1);
    if (mmio_read_32(REG_DRAM_RBUS_RCMD_CNT) != 0) {
        printf("%s FAILED6 due to read(RCMD_CNT):0x%x != 0 after clear\r\n", __func__, mmio_read_32(REG_DRAM_RBUS_RCMD_CNT));
        return;
    }

    /* write DRAM_TSIZE and check counter */
    for (j=0; j<NUM_DRAM_RBASE; j++) {
        val_base = j*words;
        for (i=0; i<words; i++) {
            addr = base_addr[j]+(i<<2);
            val = 0x2345679+i+val_base;
            mmio_write_32(addr, val);
        }
    }
    if (mmio_read_32(REG_DRAM_RBUS_WCMD_CNT) != (NUM_DRAM_RBASE*words)) {
        printf("%s FAILED7 due to read(WCMD_CNT):0x%x != words:0x%x\r\n", __func__, mmio_read_32(REG_DRAM_RBUS_WCMD_CNT), (NUM_DRAM_RBASE*words));
        return;
    }

    /* read DRAM_TSIZE and check counter */
    for (j=0; j<NUM_DRAM_RBASE; j++) {
        val_base = j*words;
        for (i=0; i<words; i++) {
            addr = base_addr[j]+(i<<2);
            val = 0x2345679+i+val_base;
            cnt = i+1+val_base;
            if (mmio_read_32(addr) != val) {
                printf("%s FAILED8 due to read(0x%x):0x%x != val:0x%x at i:%d,j:%d\r\n", 
                    __func__, addr, mmio_read_32(addr), val, i, j);
                return;
            }
        }
    }
    if (mmio_read_32(REG_DRAM_RBUS_RCMD_CNT) != (NUM_DRAM_RBASE*words)) {
        printf("%s FAILED9 due to read(RCMD_CNT):0x%x != words:0x%x\r\n", __func__, mmio_read_32(REG_DRAM_RBUS_WCMD_CNT), (NUM_DRAM_RBASE*words));
        return;
    }

    /* clear counters */
    mmio_write_32(REG_DRAM_RBUS_WCMD_CNT_CLR, 1);
    mmio_write_32(REG_DRAM_RBUS_RCMD_CNT_CLR, 1);

    /* write/read DRAM and check counters every byte */
    for (j=0; j<NUM_DRAM_RBASE; j++) {
        val_base = j*DRAM_TSIZE;
        for (i=0; i<DRAM_TSIZE; i++) {
            addr = base_addr[j]+i;
            bVal = i&0xff;
            cnt = i+1+val_base;
            mmio_write_8(addr, bVal);
            if (mmio_read_32(REG_DRAM_RBUS_WCMD_CNT) != cnt) {
                printf("%s FAILED10 due to read(WCMD_CNT):0x%x != cnt:0x%x at i:%d,j:%d\r\n", 
                    __func__, mmio_read_32(REG_DRAM_RBUS_WCMD_CNT), cnt, i, j);
                return;
            }
            if (mmio_read_8(addr) != bVal) {
                printf("%s FAILED11 due to read(0x%x):0x%x != bVal:0x%x at i:%d,j:%d\r\n", 
                    __func__, addr, mmio_read_8(addr), bVal, i, j);
                return;
            }
            if (mmio_read_32(REG_DRAM_RBUS_RCMD_CNT) != cnt) {
                printf("%s FAILED12 due to read(RCMD_CNT):0x%x != cnt:0x%x at i:%d,j:%d\r\n", 
                    __func__, mmio_read_32(REG_DRAM_RBUS_WCMD_CNT), cnt, i, j);
                return;
            }
        }
    }

    /* clear counters */
    mmio_write_32(REG_DRAM_RBUS_WCMD_CNT_CLR, 1);
    mmio_write_32(REG_DRAM_RBUS_RCMD_CNT_CLR, 1);

    printf("%s Done !!!\r\n", __func__);
    return;
}
#endif

#ifdef TEST_SW_RESET
#define RG_EJTAG_ENABLE     (0x1FBE2E18)

void sw_reset_test(void)
{
	unsigned int reg_val;

    /* enable EJTAG */
	reg_val = mmio_read_32(RG_EJTAG_ENABLE);
	reg_val |= 0x3;
	mmio_write_32(RG_EJTAG_ENABLE, reg_val);

    printf("Use CodeViser to check if DRAM's content is un-changed after SW Reset, then set 0x1fb00280 as 1 to continue\r\n");
    while (mmio_read_32(0x1fb00280)==0);
}
#endif

#ifdef TEST_DRAM_RBUS_RESET
#define RG_DRAM_ILLADDR_START2  (0x1fa00024)
#define RG_RESET_CTRL           (0x1fb00040)

void dram_rbus_reset_test(void)
{
    printf("%s Start\r\n", __func__);
    
    mmio_write_32(RG_DRAM_ILLADDR_START2, 0x12345678);
    printf("Reset DRAM\r\n");
    mmio_write_32(RG_RESET_CTRL, 0x1);
    mmio_write_32(RG_RESET_CTRL, 0x0);
    if (mmio_read_32(RG_DRAM_ILLADDR_START2) == 0x12345678)
        printf("[Pass] Reset DRAM\r\n");
    else
        printf("[Fail] Reset DRAM\r\n");

    mmio_write_32(RG_DRAM_ILLADDR_START2, 0x23456789);
    printf("Reset RBus\r\n");
    mmio_write_32(RG_RESET_CTRL, 0x100);
    mmio_write_32(RG_RESET_CTRL, 0x0);
    if (mmio_read_32(RG_DRAM_ILLADDR_START2) == 0xffffffff)
        printf("[Pass] Reset RBus\r\n");
    else
        printf("[Fail] Reset RBus\r\n");

    printf("%s Done\r\n", __func__);
}
#endif

void tests_on_l2c_sram(void)
{
    #ifdef TEST_BLOCK_CNT_IN_SRAM
    dma_block_cnt_test();
    dma_block_mask_test();
    #endif
    #ifdef TEST_RBUS_TIMEOUT
    rbus_timeout_tests_all();
    #endif
    #ifdef TEST_RBUS_DRAM_CNT
    rbus_dram_access_cnt_test();
    #endif
    #ifdef TEST_SW_RESET
    sw_reset_test();
    #endif
    #ifdef TEST_DRAM_RBUS_RESET
    dram_rbus_reset_test();
    #endif

    return;
}

