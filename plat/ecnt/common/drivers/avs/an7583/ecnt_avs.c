#include <linux/version.h>
#include <asm/io.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#else
#include <drivers/delay_timer.h>
#include <asm/tc3162.h>
#endif
#include "ecnt_avs.h"

/************************************************************************
*                  D E F I N E S   &   C O N S T A N T S
*************************************************************************
*/


/************************************************************************
*                  M A C R O S
*************************************************************************
*/
//#define get_chip_scu_data(reg) readl((0x1fa20000 + reg))
//#define set_chip_scu_data(reg, val) writel(val, (0x1fa20000 + reg))
#define printk printf
//#define TCSUPPORT_AUTOBENCH


/************************************************************************
*                  D A T A   T Y P E S
*************************************************************************
*/

/* APIs */
//kernel version
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
extern u32 get_avs_DAC(u32 *AVS_DAC);
extern void set_avs_DAC(u32 *AVS_DAC, u8 delay_cnt);
extern u32 read_TADC(ADC_MUX_T mux_num, u8 delay_cnt);
extern void avs_DAC_en(u32 DAC_EN);
#endif
extern void ef_read_parse(unsigned int start, unsigned int len, unsigned char *data);

/*global variables*/
static u32 g_EF_LV_code = 0, g_EF_HV_code = 0, g_ADC_slope = 0;
static u8 g_EF_OK_flag = 0, g_EF_LOAD_done = 0;
static u32 FW_DAC_STEP = 10, FW_DAC_ST_TIME = DAC_ST_Time;


/************************************************************************
*                 AVS API
*************************************************************************
*/
static u32 get_chip_scu_data(u32 reg)
{
	u32 offset=0;
	u64 reg_val=0;

	reg += 0x1fa20000;

	//printf("R reg = 0x%x\n", reg);
	#ifdef IMAGE_BL31
	if((reg & 0x7) !=0)
	{
		offset = (reg & 0x7)<<3;
		reg = (reg>>3)<<3;
	}	
	#endif
	//printf("R reg = 0x%x\n", reg);
	//printf("R offset = %d\n", offset);
	
	reg_val = readl(reg) >> offset;
	//printf("R reg_val 1 = 0x%llx\n", reg_val);
	//printf("R reg_val 2 = 0x%x\n", (u32)reg_val);
	
	return (u32)reg_val;
}

static void set_chip_scu_data(u32 reg, u32 val)
{
	u32 offset=0;
	u64 reg_val=0, mask=0xFFFFFFFF;

	reg += 0x1fa20000;

	//printf("reg = 0x%x\n", reg);
	#ifdef IMAGE_BL31
	if((reg & 0x7) !=0)
	{
		offset = (reg & 0x7)<<3;
		reg = (reg>>3)<<3;
	}
	#endif
	//printf("reg = 0x%x\n", reg);
	//printf("offset = %d\n", offset);
	//printf("mask = 0x%llx\n", mask);
	reg_val = readl(reg);
	//printf("reg_val 1 = 0x%llx\n", reg_val);
	reg_val &= ~(mask<<offset);
	//printf("reg_val 2 = 0x%llx\n", reg_val);
	reg_val |= ((u64)(val)<<offset);
	//printf("reg_val 3 = 0x%llx\n", reg_val);

	writel(reg_val, reg);
	
}

u32 rw_chip_scu_data(u32 addr,u32 value,u32 mask,u16 ofst)
{
	u32 newValue=(get_chip_scu_data(addr) &(~(mask <<ofst)) ) | ((value&mask)<<ofst);
	set_chip_scu_data(addr, newValue);
	return newValue;
}

static void AVS_DAC_CHG(void)
{
	u32 tmp_val;
	
	//toggle CHG bit
	tmp_val = get_chip_scu_data(RG_DATA_AVS_DAC) | (0x1<<DAC_CHG_OFST);
	set_chip_scu_data(RG_DATA_AVS_DAC, tmp_val);
	tmp_val = get_chip_scu_data(RG_DATA_AVS_DAC) & (~(0x1<<DAC_CHG_OFST));
	set_chip_scu_data(RG_DATA_AVS_DAC, tmp_val);
}

/*get AVS_DAC value & return AVS_OUT_EN value
*/ 
static u32 get_avs_DAC(u32 *AVS_DAC)
{
	*AVS_DAC = get_chip_scu_data(RG_DATA_AVS_DAC) & 0x3ff;

	return (get_chip_scu_data(RG_AVS_OUT_EN)&0x1);
}
//EXPORT_SYMBOL(get_avs_DAC);

/*set AVS_DAC value & enable AVS_DAC output
*/ 
static void set_avs_DAC(u32 *AVS_DAC, u8 delay_cnt)
{
	set_chip_scu_data(PLLRG_PROTECT, PROTECT_KEY);
	set_chip_scu_data(RG_DATA_AVS_DAC, *AVS_DAC);
	AVS_DAC_CHG();
	udelay(delay_cnt*1000);
	set_chip_scu_data(PLLRG_PROTECT, 0x0);
}
//EXPORT_SYMBOL(set_avs_DAC);

/*DAC_EN = 1 to enable output; DAC_EN = 0 to disable output*/
static void avs_DAC_en(u8 DAC_EN)
{
	set_chip_scu_data(PLLRG_PROTECT, PROTECT_KEY);
	set_chip_scu_data(RG_AVS_OUT_EN, (get_chip_scu_data(RG_AVS_OUT_EN) & (~(0x1<<DAC_EN_OFST)))|((DAC_EN & 0x1)<<DAC_EN_OFST));
	set_chip_scu_data(PLLRG_PROTECT, 0x0);
}

/*return TADC value by mux_num: 0=thermal, 1=AVSMON, 2=Vcore, 3=AVS_DAC
*/
static u32 read_TADC(ADC_MUX_T mux_num, u8 delay_cnt)
{
	u32 val=0;
	
	if(mux_num != ((get_chip_scu_data(MUX_TADC)>>TADC_MUX_OFST)&TADC_MUX_MASK))
	{
		set_chip_scu_data(PLLRG_PROTECT,  PROTECT_KEY);
		rw_chip_scu_data(MUX_TADC, mux_num, TADC_MUX_MASK, TADC_MUX_OFST);
		udelay(delay_cnt*1000);
		set_chip_scu_data(PLLRG_PROTECT,  0x0);
	}
	
	val = get_chip_scu_data(DOUT_TADC);
	

	#if avs_dbg
	printk("MUX_SEL RG(0x1fa202e4) = 0x%x\r\n", get_chip_scu_data(0x2e4));
	printk("ADC_VAL1 RG(0x1fa202f0) = 0x%x\r\n", val);
	printk("ADC_VAL2 RG(0x1fa202f0) = 0x%x\r\n", get_chip_scu_data(0x2f0));
	#endif
	
    return val;
}

static void TADC_init(void)
{
  set_chip_scu_data(PLLRG_PROTECT,  PROTECT_KEY);
  //set TADC related registers
  //set TADC mode
  rw_chip_scu_data(MUX_TADC,0x33B0,TADC_SET_MASK,0);
  //reset TADC, 0x1fa202ec = [4] = 0, 0x1fa202ec = [4] = 1
  rw_chip_scu_data(MUX_TADC,0,1,TADC_RST_OFST);
  rw_chip_scu_data(MUX_TADC,1,1,TADC_RST_OFST);
  set_chip_scu_data(PLLRG_PROTECT,  0x0);
}

#ifdef TCSUPPORT_AUTOBENCH
static void STL_Error_Report(void)
{
	printf("SLT_BIN2\n");
	printf("SLT_BIN2\n");
	printf("SLT_BIN2\n");
    while (1);
}
#endif

//input: 1. target_ADC: target ADC code; 2. mux_sel: select ADC = Vcore/DAC_OUT
static AVS_STATUS_T DAC_approach_target(u32 target_ADC, ADC_MUX_T mux_sel)
{
	u32 AVS_ADC, AVS_DAC = 0, DAC_step = FW_DAC_STEP, cnt = 0;
	u8 init_state = 0, search_done = 0;

	if(mux_sel != CORE_POWER && mux_sel != AVSDAC_OUT)
	{
		printk("[AVS][DAC_approach_target] un-supported mux_sel: %d\n", mux_sel);
		return OUT_RANGE_FAIL;
	}

	get_avs_DAC(&AVS_DAC);
	
	if(mux_sel == AVSDAC_OUT)
	{
	#if avs_dbg
		printk("AVS_DAC init.(1) = 0x%x\r\n", AVS_DAC);
	#endif
	}
	
	AVS_ADC = read_TADC(mux_sel, ADC_ST_Time)>>TADC_gran_step_shift;
	
	#if avs_dbg
	printk("AVS_ADC init(>>%d) = 0x%x\r\n", TADC_gran_step_shift, AVS_ADC);
	#endif
	
	if(AVS_ADC > target_ADC)
	{
		init_state = 1; //init_state = 1, represent initial ADC > target ADC
		//decide init. step size
		for(DAC_step = FW_DAC_STEP; DAC_step > 2; DAC_step >>= 1)
		{
			if((AVS_ADC - target_ADC)*g_ADC_slope >= DAC_step*AVS_DAC_step)
				break;
		}
	}
	else
	{
		init_state = 0; //init_state = 0, represent initial ADC <= target ADC
		//decide init. step size
		for(DAC_step = FW_DAC_STEP; DAC_step > 2; DAC_step >>= 1)
		{
			if((target_ADC - AVS_ADC)*g_ADC_slope >= DAC_step*AVS_DAC_step)
				break;
		}
	}
	
	while(AVS_ADC != target_ADC && search_done == 0)
	{
		//error handler
		if(cnt > (MAX_AVS_LOOP>>1) || AVS_DAC > 0x350 || AVS_DAC < 0x80)
		{
		#if 1//avs_dbg
			printk("AVS approach target FAIL!\r\n");
			printk("AVS target ADC(>>%d) = 0x%x\r\n", TADC_gran_step_shift, target_ADC);
			printk("AVS_ADC(>>%d) = 0x%x\r\n", TADC_gran_step_shift, AVS_ADC);
			printk("AVS_DAC = 0x%x\r\n", AVS_DAC);
			printk("AVS_DAC RG = 0x%x\r\n", (u32)get_chip_scu_data(RG_DATA_AVS_DAC));
		#endif
			
			return AVS_FAIL;
		}
		
		cnt++;
		
		if(AVS_ADC > target_ADC)
		{			
			if(init_state == 0)
			{
				//DAC tuning too much setp, decrease step size, and go reverse direction.
				DAC_step >>= 1;

				init_state = 1;
				
				if(DAC_step <= FW_DAC_STEP_FINAL)
				{
					search_done = 1;
				}
			}
			
			if(mux_sel == CORE_POWER)
			{
				//DAC+=x, make Vcore = Vupper - VDAC to decrease 
				AVS_DAC += DAC_step;
			}
			else
			{
				//DAC-=x, make VDAC/AVSMON to decrease 
				AVS_DAC -= DAC_step;
			}
		}
		else	//AVS_ADC < target_ADC
		{			
			if(init_state == 1)
			{
				//DAC tuning too much setp, decrease step size, and go reverse direction.
				DAC_step >>= 1;

				init_state = 0;
				
				if(DAC_step <= FW_DAC_STEP_FINAL)
				{
					search_done = 1;
				}
			}

			if(mux_sel == CORE_POWER)
			{
				//DAC-=x, make Vcore = Vupper - VDAC to increase
				AVS_DAC -= DAC_step;
			}
			else
			{
				//DAC+=x, make VDAC/AVSMON to increase 
				AVS_DAC += DAC_step;
			}
		}
		
		set_avs_DAC(&AVS_DAC, FW_DAC_ST_TIME);
		AVS_ADC = read_TADC(mux_sel, ADC_ST_Time)>>TADC_gran_step_shift;
	}

	if(mux_sel == AVSDAC_OUT)
	{
	#if avs_dbg
		printk("AVS_DAC init.(2) = 0x%x\r\n", AVS_DAC);
	#endif
	}

	#if avs_dbg
	printk("AVS_ADC(>>%d) = 0x%x\r\n", TADC_gran_step_shift, AVS_ADC);
	printk("AVS_DAC = 0x%x\r\n", AVS_DAC);
	printk("AVS_DAC RG = 0x%x\r\n", (u32)get_chip_scu_data(RG_DATA_AVS_DAC));
	#endif
	
	return AVS_OK;
}


/*get phy efuse*/
u32 get_phy_efuse(u32 start_bit, u32 len)
{
	u32 data_ef=0;
	
	ef_read_parse(start_bit,len,(unsigned char *)&data_ef);

	return data_ef;
}

//get ADC slope from efuse. Unit: 0.01mV per 32 code.
static u32 get_EF_slope(void)
{
	u32 ADC_slope = 0;
	u32 AVS_efuse_val = 0;

	AVS_efuse_val = get_phy_efuse(AVS_Voltage_AVS_Voltage_Cal_1_ADC_LSB, 32);

	if(AVS_efuse_val != -1 && AVS_efuse_val != 0)
	{
		g_EF_HV_code = (AVS_efuse_val >> 16) >> TADC_gran_step_shift;
		g_EF_LV_code = (AVS_efuse_val & 0xffff) >> TADC_gran_step_shift;
		//calculate value of 0.001mV per 32 code & roundup from 0.001mV to 0.01mV per 32 code
		ADC_slope = ((g_EF_HV_code > g_EF_LV_code) ? ((EF_Vdiff / (g_EF_HV_code - g_EF_LV_code)+5)/10) : 0);
	}

#if avs_dbg
	printk("ADC slope = %d\n", ADC_slope);
#endif	
	return ADC_slope;
}

static void get_avs_EF_K(void)
{	
	g_ADC_slope = get_EF_slope();

	if(g_ADC_slope == 0 || g_EF_LV_code == 0)
	{

		g_ADC_slope = TADC_gran_step;

		g_EF_LV_code = ST_LV_code;

		#if avs_dbg
			printk("No EF, ADC_slope = %d\nEF_LV_code = 0x%x\n", g_ADC_slope, g_EF_LV_code);
		#endif	

		g_EF_OK_flag = 0;
	}
	else
	{
		#if avs_dbg
			printk("With EF, ADC_slope = %d\nEF_LV_code = 0x%x\nEF_HV_code = 0x%x\n", g_ADC_slope, g_EF_LV_code, g_EF_HV_code);
		#endif	
		
		g_EF_OK_flag = 1;
		
	}
	
	g_EF_LOAD_done = 1;
}

//return value unit = 0.001V 
u32 AVS_Get_Vcore(void)
{

//AVS ADC get Vcore value.(accuracy: +/-10mV)
#if 1
	u32 Vcore_ADC, Vcore;

	Vcore_ADC = read_TADC(CORE_POWER, ADC_ST_Time)>>TADC_gran_step_shift;

	#if avs_dbg
	//printk("ADC_slope = %d\n", g_ADC_slope);
	//printk("EF_LV_code = 0x%x\n", g_EF_LV_code);
	printk("Vcore_ADC(>>%d) = 0x%x\n", TADC_gran_step_shift, Vcore_ADC);
	#endif
		
	if(Vcore_ADC > g_EF_LV_code)
	{	
		//calculate value of 0.01mV & roundup from 0.01mV to 0.001V
		Vcore = ((Vcore_ADC - g_EF_LV_code)*g_ADC_slope + FT_LV_code + 50)/100;
		#if avs_dbg
		printk("Vcore = %d mV\r\n", Vcore);
		#endif
	}
	else
	{	
		//calculate value of 0.01mV & transfer from 0.01mV to 0.01V
		//Vcore = (FT_LV_code - (EF_LV_code - Vcore_ADC)*ADC_slope + 500)/1000;
		//Vcore below 0.45V, show error message.
		printk("AVS_Get_Vcore error: Vcore < 0.45V\r\n");
		Vcore = 0;
	}
	return Vcore;

	
#endif

}
//EXPORT_SYMBOL(AVS_Get_Vcore);

//input parameter Vcore_tar unit = 0.001V
static AVS_STATUS_T DAC_approach_Vcore(u32 Vcore_tar)
{
	u32 Vcore, AVS_DAC, DAC_step = FW_DAC_STEP;
	int DAC_cnt = 0;
	u8 init_state = 0, search_done = 0, close_tar_flag = 0;
#if avs_dbg
	u32 cnt = 0
#endif
	
	Vcore = AVS_Get_Vcore();

	get_avs_DAC(&AVS_DAC);
	
#if avs_dbg
	printk("AVS_DAC init = 0x%x\r\n", AVS_DAC);
	printk("Vcore target = %d mV\r\n", Vcore_tar);
	printk("Vcore start = %d mV\r\n", Vcore);
#endif

	if(Vcore > Vcore_tar)
	{
		init_state = 1; //init_state = 1, represent initial Vcore > target Vcore
		//decide init. step size
		for(DAC_step = FW_DAC_STEP; DAC_step > 2; DAC_step >>= 1)
		{
			if((Vcore - Vcore_tar)*200 >= DAC_step*AVS_DAC_step)	//Vavs = Vref - Vdac*0.5, unit 0.01mV
				break;
		}
	}
	else
	{
		init_state = 0; //init_state = 0, represent initial Vcore <= target Vcore
		//decide init. step size
		for(DAC_step = FW_DAC_STEP; DAC_step > 2; DAC_step >>= 1)
		{
			if((Vcore_tar - Vcore)*200 >= DAC_step*AVS_DAC_step)	//Vavs = Vref - Vdac*0.5, unit 0.01mV
				break;
		}
	}
#if avs_dbg
	printk("DAC_step init = 0x%x\r\n", DAC_step);
#endif
	
	while(Vcore != Vcore_tar && search_done == 0)
	{
		//error handling
		if(DAC_cnt > MAX_AVS_LOOP || DAC_cnt < (-1)*MAX_AVS_LOOP || Vcore > Vcore_H_thd || Vcore < Vcore_L_thd)
		{
		#if 1//avs_dbg
			printk("AVS approach target FAIL!\r\n");
			printk("Vcore_tar = %dmV\r\n", Vcore_tar);
			printk("Vcore = %dmV\r\n", Vcore);
			printk("AVS_DAC RG = 0x%x\r\n", (u32)get_chip_scu_data(RG_DATA_AVS_DAC));
		#endif
			
			//disable DAC output
			avs_DAC_en(0x0);
			return AVS_FAIL;
		}

	#if avs_dbg
		cnt++;
	#endif
		
		if(Vcore > Vcore_tar)
		{	
		#if 1
			if(DAC_step > 2 && init_state == 1)
			{
				if(close_tar_flag == 1)
				{
					DAC_step = 2;
					
				#if avs_dbg
					printk("close_tar_flag = %d\r\n", close_tar_flag);
					printk("DAC_step change to %d\r\n", DAC_step);
				#endif
				}
				else if((Vcore - Vcore_tar)*200 < (DAC_step*AVS_DAC_step)) //Vdiff < DAC_step*AVS_DAC_step/2 (0.01mV)
				{
					//calculate precise DAC step
					DAC_step = (Vcore - Vcore_tar)*200/AVS_DAC_step;

					DAC_step = (DAC_step > 2) ? DAC_step : 2;

				#if avs_dbg
					printk("close_tar_flag = %d\r\n", close_tar_flag);
					printk("DAC_step change to %d\r\n", DAC_step);
				#endif
					
					close_tar_flag = 1;
				}
				
			}
			else	//DAC_step <= 2 or init_state = 0(Vcore < V_tar)
		#endif
			{
				if(init_state == 0)
				{
					//in previous loop, minus too much DAC,  rollback DAC to mid value
					if(close_tar_flag == 1 && DAC_step > 2)
					{
						DAC_step = 2;
					}
					else
					{
						DAC_step >>= 1;
					}

					init_state = 1;
					
					if(DAC_step <= FW_DAC_STEP_FINAL)
					{
						search_done = 1;
					}
				}
			}
			
			//DAC+=x, make Vcore = Vupper - VDAC to decrease
			if(AVS_DAC == 0x3ff)
			{
				printk("AVS approach target FAIL!\r\n");
				printk("AVS_DAC = %d, can not add more DAC_step = %d!\n",AVS_DAC, DAC_step);
				printk("Vcore_tar = %dmV\r\n", Vcore_tar);
				printk("Vcore = %dmV\r\n", Vcore);
				//disable DAC output
				avs_DAC_en(0x0);
				return AVS_FAIL;
			}
			else
			{
				AVS_DAC += DAC_step;
				DAC_cnt += DAC_step;
			}
		}
		else //Vcore < Vcore_tar
		{
		#if 1
			if(DAC_step > 2 && init_state == 0)
			{
				if(close_tar_flag == 1)
				{
					DAC_step = 2;
				}
				else if((Vcore_tar - Vcore)*200 < (DAC_step*AVS_DAC_step)) //Vdiff < DAC_step*AVS_DAC_step/2 (0.01mV)
				{
					//calculate precise DAC step
					DAC_step = (Vcore_tar - Vcore)*200/AVS_DAC_step;

					DAC_step = (DAC_step > 2) ? DAC_step : 2;
					
					close_tar_flag = 1;
				}
				
			}
			else	//DAC_step <= 2 or init_state = 1(Vcore > V_tar)
		#endif
			{
				if(init_state == 1)
				{
					//in previous loop, add too much DAC,  rollback DAC to mid value
					if(close_tar_flag == 1 && DAC_step > 2)
					{
						DAC_step = 2;
					}
					else
					{
						DAC_step >>= 1;
					}

					init_state = 0;
					
					if(DAC_step <= FW_DAC_STEP_FINAL)
					{
						search_done = 1;
					}
				}
			}

			//DAC-=x, make Vcore = Vupper - VDAC to increase
			if(AVS_DAC == 0)
			{
				printk("AVS approach target FAIL!\r\n");
				printk("AVS_DAC = %d, can not minus more DAC_step = %d!\n",AVS_DAC, DAC_step);
				printk("Vcore_tar = %dmV\r\n", Vcore_tar);
				printk("Vcore = %dmV\r\n", Vcore);
				//disable DAC output
				avs_DAC_en(0x0);
				return AVS_FAIL;
			}
			else
			{
				AVS_DAC = (AVS_DAC <= DAC_step) ? 0 : (AVS_DAC-DAC_step);
				DAC_cnt -= DAC_step;
			}
		}

		set_avs_DAC(&AVS_DAC, FW_DAC_ST_TIME);
		Vcore = AVS_Get_Vcore();

	}

#if avs_dbg
	printk("Final DAC_step = %d\n",DAC_step);
	printk("DAC_approach_Vcore loop cnt = %d\n",cnt);
	printk("Vcore end = %d mV\r\n", Vcore);
#endif
	
	return AVS_OK;
}

//input parameter Vcore_tar unit = 0.001V
u32 AVS_Set_ABS(u32 Vcore_tar)
{
	u32 RG_AVS_OUT_EN_val, AVS_DAC;
	u32 FB_V_ADC, Vcore;
	AVS_STATUS_T status = AVS_FAIL;

	printk("AVS set point: %d.%03dV\r\n", Vcore_tar/1000, Vcore_tar%1000);
	
	if(g_EF_LOAD_done == 0)
	{
		TADC_init();
		get_avs_EF_K();
	}

	#ifndef USE_DEF_K
	if(g_EF_OK_flag == 0)
	{
		printk("AVS ERROR: efuse invalid\r\n");
		#ifdef TCSUPPORT_AUTOBENCH
		STL_Error_Report();
		#endif
		return AVS_FAIL;
	}
	#endif

	//check default Vcore
	if(get_avs_DAC(&AVS_DAC) == 0)
	{
		Vcore = AVS_Get_Vcore();
		if(Vcore > Vcore_H_thd_def || Vcore < Vcore_L_thd_def)
		{
			printk("AVS ERROR: Detect HW default Volt. out of range.\r\n");
			printk("AVS ERROR: HW default Volt. = %d.%03dV\r\n", Vcore/1000, Vcore%1000);
			#ifdef TCSUPPORT_AUTOBENCH
			STL_Error_Report();
			#endif
			return AVS_FAIL;
		}
	}

	RG_AVS_OUT_EN_val = get_avs_DAC(&AVS_DAC);

	if(RG_AVS_OUT_EN_val == 0)
	{
		//approach to AVSMON of buck & enable DAC
		//calculate initial DAC
		FB_V_ADC = read_TADC(PAD_AVS, ADC_ST_Time)>>TADC_gran_step_shift;

		AVS_DAC = ((FB_V_ADC - g_EF_LV_code)*g_ADC_slope + FT_LV_code)/AVS_DAC_step;
		
		set_avs_DAC(&AVS_DAC, FW_DAC_ST_TIME);
		
		if(DAC_approach_target(FB_V_ADC, AVSDAC_OUT) != AVS_OK)
		{
			printk("AVS FAIL: DAC track to AVSMON fail\r\n");
			#ifdef TCSUPPORT_AUTOBENCH
			STL_Error_Report();
			#endif
			return status;
		}
		else
		{
			//enable DAC output
			avs_DAC_en(0x1);
			#if avs_dbg
			printk("DAC_EN RG(0x1fa202a0) = 0x%x\r\n", get_chip_scu_data(0x2a0));
			#endif
		}
	}
	
#if 1
	//approach to target Vcore
	if(DAC_approach_Vcore(Vcore_tar) != AVS_OK)
	{
		printk("AVS FAIL: track to Vcore fail\r\n");
		#ifdef TCSUPPORT_AUTOBENCH
		STL_Error_Report();
		#endif
	}
	else
	{
		status = AVS_OK;
		printk("AVS set done.\r\n");
	}
#endif
	
	return status;
}

