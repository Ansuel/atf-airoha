#include <linux/version.h>
#include <asm/io.h>
#include <ecnt_avs.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#include <linux/sched.h>
#else
#include <asm/tc3162.h>
#endif
//#include "avs/avs.h"

/************************************************************************
*                  D E F I N E S   &   C O N S T A N T S
*************************************************************************
*/
#define mux_T	0
#define mux_AVSMON	1
#define mux_VCORE	2
#define mux_AVSDAC	3
	
#define AVS_DAC_step		120		//unit: 0.01mV
//#define TADC_step 		31	//unit: uV
#define TADC_32_step		96		//standard slope for non-efuse case, unit: 0.01mV
#define TADC_32_step_shift		5
#define EF_Vdiff			500000	//unit: 0.001mV
#define FT_LV_code  		75000	//0.75V, unit: 0.01mV
#define LV_drop_1				8000	//0.08V, unit: 0.01mV
#define LV_drop_2				3000	//0.03V, unit: 0.01mV
#define HV_boost				5000	//0.05V, unit: 0.01mV
#define AVSADC_MIN			0x80	//~=100mV for ADC code divide 32

#define avs_dbg 0

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
extern u32 get_chip_scu_data(u32 reg);
extern void set_chip_scu_data(u32 reg, u32 val);
extern u32 get_ptp_dummy_AVS(void);
#else
#define get_chip_scu_data(reg) readl((0x1fa20000 + reg))
#define set_chip_scu_data(reg, val) writel(val, (0x1fa20000 + reg))
#define RG_THERMAL_DUMMY_REG 0x1efbdf20
#define RG_AVS_DUMMY_REG 0x1efbdf24
#define	get_ptp_dummy_AVS()	readl(RG_AVS_DUMMY_REG)
#endif

u32 NV_out_flag=0;

/************************************************************************
*                  M A C R O S
*************************************************************************
*/
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
#else
#define printk printf
#endif

/************************************************************************
*                  D A T A   T Y P E S
*************************************************************************
*/
/* APIs */
//kernel version
extern u32 get_avs_DAC(u32 *AVS_DAC);
extern void set_avs_DAC(u32 *AVS_DAC);
extern u32 read_TADC(u32 mux_num);
extern void avs_DAC_en(u32 DAC_EN);
extern void set_TADC_temp(void);

AVS_STATUS_T DAC_approach_target(u32 target_ADC, u32 mux_sel)
{
	u32 AVS_ADC, AVS_DAC, cnt = 0, cnt_limit = 40;
	u32 init_state = 0;

	get_avs_DAC(&AVS_DAC);
	
	#if avs_dbg
	printk("AVS_DAC init = 0x%x\r\n", AVS_DAC);
	printk("AVS target ADC = %d\r\n", target_ADC);
	#endif
	
	AVS_ADC = read_TADC(mux_sel)>>TADC_32_step_shift;
	
	#if avs_dbg
	printk("AVS_ADC init = %d\r\n", AVS_ADC);
	#endif
	
	if(AVS_ADC > target_ADC)
	{
		init_state = 1;	//init_state = 1, represent initial ADC > target ADC
	}
	else
	{
		init_state = 2;	//init_state = 2, represent initial ADC <= target ADC
	}
	
	while(AVS_ADC != target_ADC)
	{
		cnt++;
		
		if(AVS_ADC > target_ADC)
		{			
			if(init_state == 2)
			{
				if(mux_sel == mux_VCORE)
				{
					//in previous loop, minus too much DAC, rollback DAC to mid value and finish searching
					AVS_DAC += 2;
				}
				else
				{
					//in previous loop, add too much DAC, rollback DAC to mid value and finish searching
					AVS_DAC -= 2;
				}
				set_avs_DAC(&AVS_DAC);
				break;
			}
			
			if(mux_sel == mux_VCORE)
			{
				//DAC+=4, make Vcore = Vupper - VDAC to decrease 
				AVS_DAC += 4;
			}
			else
			{
				//DAC-=4, make VDAC/AVSMON to decrease 
				AVS_DAC -= 4;
			}
			//set_avs_DAC(&AVS_DAC);
		}
		else
		{			
			if(init_state == 1)
			{
				if(mux_sel == mux_VCORE)
				{
					//in previous loop, add too much DAC,  rollback DAC to mid value and finish searching
					AVS_DAC -= 2;
				}
				else
				{
					//in previous loop, minus too much DAC, rollback DAC to mid value and finish searching
					AVS_DAC += 2;
				}
				set_avs_DAC(&AVS_DAC);
				break;
			}

			if(mux_sel == mux_VCORE)
			{
				//DAC-=4, make Vcore = Vupper - VDAC to increase
				AVS_DAC -= 4;
			}
			else
			{
				//DAC+=4, make VDAC/AVSMON to increase 
				AVS_DAC += 4;
			}
			//set_avs_DAC(&AVS_DAC);
		}

		if(cnt > cnt_limit || AVS_DAC > 0x3ff)
		{
			//Vcore_ADC = read_TADC(mux_AVSDAC)>>TADC_32_step_shift;
			#if avs_dbg
			printk("AVS approach target FAIL!\r\n");
			printk("AVS_ADC = %d\r\n", AVS_ADC);
			printk("AVS_DAC = 0x%x\r\n", AVS_DAC);
			printk("AVS_DAC RG = 0x%x\r\n", (u32)get_chip_scu_data(0x2e8));
			#endif
			//set TADC to temperature mode
			set_TADC_temp();
			return AVS_FAIL;
		}
		
		set_avs_DAC(&AVS_DAC);
		AVS_ADC = read_TADC(mux_sel)>>TADC_32_step_shift;
	}

	#if avs_dbg
	printk("AVS_ADC = %d\r\n", AVS_ADC);
	printk("AVS_DAC = 0x%x\r\n", AVS_DAC);
	printk("AVS_DAC RG = 0x%x\r\n", (u32)get_chip_scu_data(0x2e8));
	#endif
	
	//set TADC to temperature mode
	set_TADC_temp();
	
	return AVS_OK;
}

//get ADC slope from efuse. Unit: 0.01mV per 32 code.
u32 get_EF_slope(void)
{
	u32 EF_HV_code = 0, EF_LV_code = 0, ADC_slope = 0;

	EF_HV_code = (get_ptp_dummy_AVS() >> 16) >> TADC_32_step_shift;
	EF_LV_code = (get_ptp_dummy_AVS() & 0xffff) >> TADC_32_step_shift;
	//calculate value of 0.001mV per code & transfer from 0.001mV to 0.01mV
	ADC_slope = ((EF_HV_code > EF_LV_code) ? ((EF_Vdiff / (EF_HV_code - EF_LV_code)+5)/10) : TADC_32_step);
#if avs_dbg
	printk("ADC slope = %d\n", ADC_slope);
#endif	
	return ADC_slope;
}


/*according to V_SEL, 
if V_SEL=1(LV) & iddq>90mA, AVS set Vcore to drop 0.1V;
if V_SEL=1(LV) & iddq<=90mA, AVS set Vcore to drop 0.05V;
if V_SEL=0(NV), disable AVS, i.e. restore Vcore to NV*/

AVS_STATUS_T AVS_Set(u32 V_SEL, u32 iddq)
{
#if 1
	u32 Vcore_ADC, FB_V_ADC, target_Vcore_ADC;
	//u32 target_DAC;
	u32 RG_AVS_OUT_EN_val, AVS_DAC;
	u32 V_drop = 0;

	if(V_SEL != NV && V_SEL != LV && V_SEL != HV)
	{
		//do nothing & return OK.
		return AVS_OK;	
	}

	Vcore_ADC = read_TADC(mux_VCORE)>>TADC_32_step_shift;
	FB_V_ADC = read_TADC(mux_AVSMON)>>TADC_32_step_shift;
	//set TADC to temperature mode
	set_TADC_temp();

	if(FB_V_ADC < AVSADC_MIN)
	{
		printk("AVS FAIL: function fail\r\n");
		printk("AVSMON_ADC = 0x%x\r\n", FB_V_ADC);
		printk("Vcore_ADC = %d\r\n", Vcore_ADC);
		avs_DAC_en(0x0);
		
		return AVS_FAIL;
	}

	RG_AVS_OUT_EN_val = get_avs_DAC(&AVS_DAC);

	printk("AVS_Set\r\n");

	if(V_SEL == NV)
	{
		if(RG_AVS_OUT_EN_val == 0)
		{
			//calculate initial DAC
			AVS_DAC = (FB_V_ADC*TADC_32_step)/AVS_DAC_step;

			set_avs_DAC(&AVS_DAC);
			
			//make AVS output enable to avoid power ripple by LAN LED toggle.
			if(DAC_approach_target(FB_V_ADC, mux_AVSDAC) == AVS_FAIL)
			{
				printk("AVS FAIL: DAC track to AVSMON fail\r\n");
				return AVS_FAIL;
			}
			
			avs_DAC_en(0x1);
			
			if(DAC_approach_target(Vcore_ADC, mux_VCORE) == AVS_FAIL)
			{
				avs_DAC_en(0x0);
				printk("AVS FAIL: DAC track to target Vcore fail\r\n");
				return AVS_FAIL;
			}

			NV_out_flag = 1;
		}
	}
	else	//LV or HV
	{
		if(RG_AVS_OUT_EN_val == 1 && NV_out_flag == 0)
		{
			//do nothing & return OK.
			printk("AVS no change.\r\n");
			return AVS_OK;
		}
	}

	if(iddq>90)
	{
		V_drop = LV_drop_1;
	}
	else
	{
		V_drop = LV_drop_2;
	}

	if(V_SEL == NV)
	{
		//RG_AVS_OUT_EN_val = 1, but NV_out_flag = 0
		if(NV_out_flag == 0)
		{
			if(iddq >= 30)
			{
				target_Vcore_ADC = Vcore_ADC + (V_drop / get_EF_slope());
			}
			else
			{
				target_Vcore_ADC = Vcore_ADC - (HV_boost/ get_EF_slope());
			}
			
			if(DAC_approach_target(target_Vcore_ADC, mux_VCORE) == AVS_FAIL)
			{
				avs_DAC_en(0x0);
				printk("AVS FAIL: DAC track to target Vcore fail\r\n");
				return AVS_FAIL;
			}
			
			avs_DAC_en(0x0);

			Vcore_ADC = read_TADC(mux_VCORE)>>TADC_32_step_shift;
			FB_V_ADC = read_TADC(mux_AVSMON)>>TADC_32_step_shift;
			
			//make AVS output enable to avoid power ripple by LAN LED toggle.
			if(DAC_approach_target(FB_V_ADC, mux_AVSDAC) == AVS_FAIL)
			{
				printk("AVS FAIL: DAC track to AVSMON fail\r\n");
				return AVS_FAIL;
			}
			
			avs_DAC_en(0x1);
			
			if(DAC_approach_target(Vcore_ADC, mux_VCORE) == AVS_FAIL)
			{
				avs_DAC_en(0x0);
				printk("AVS FAIL: DAC track to target Vcore fail\r\n");
				return AVS_FAIL;
			}
			
			NV_out_flag = 1;
		}
	}
	else	//V_SEL = LV/HV
	{
		//calculate target Vcore ADC
		if(V_SEL == LV)
		{
			target_Vcore_ADC = Vcore_ADC - (V_drop / get_EF_slope());
		}
		else	//V_SEL = HV
		{
			target_Vcore_ADC = Vcore_ADC + (HV_boost/ get_EF_slope());
		}
		
		
		if(RG_AVS_OUT_EN_val == 0)	//RG_AVS_OUT_EN_val = 0 & V_SEL = LV/HV
		{
			//calculate initial DAC
			AVS_DAC = (FB_V_ADC*TADC_32_step)/AVS_DAC_step;
			
			set_avs_DAC(&AVS_DAC);
			
			if(DAC_approach_target(FB_V_ADC, mux_AVSDAC) == AVS_FAIL)
			{
				printk("AVS FAIL: DAC track to AVSMON fail\r\n");
				return AVS_FAIL;
			}
			
			avs_DAC_en(0x1);
			
			if(DAC_approach_target(target_Vcore_ADC, mux_VCORE) == AVS_FAIL)
			{
				avs_DAC_en(0x0);
				printk("AVS FAIL: DAC track to target Vcore fail\r\n");
				return AVS_FAIL;
			}
			
		}
		else if(NV_out_flag == 1)	//NV_out_flag = 0 & V_SEL = LV/HV
		{
			if(DAC_approach_target(target_Vcore_ADC, mux_VCORE) == AVS_FAIL)
			{
				avs_DAC_en(0x0);
				printk("AVS FAIL: DAC track to target Vcore fail\r\n");
				return AVS_FAIL;
			}
		}
		NV_out_flag = 0;
	}
	
	//set TADC to temperature mode
	set_TADC_temp();
	
	printk("AVS set done.\r\n");	
#endif
	return AVS_OK;
}
EXPORT_SYMBOL(AVS_Set);

//return value unit = 0.01V 
u32 AVS_Get_Vcore(void)
{

//AVS ADC can't get specific Vcore value.
#if 0
	u32 EF_K_value, EF_LV_code, ADC_slope, Vcore_ADC, Vcore;

	EF_K_value = get_avs_EF_K();

	ADC_slope = (EF_K_value>>12) & 0x7F;

	EF_LV_code = EF_K_value & 0x7FF; 

	Vcore_ADC = read_TADC(mux_VCORE)>>TADC_32_step_shift;

	#if avs_dbg
	printk("Vcore_ADC = 0x%x\n", Vcore_ADC);
	#endif

	if(ADC_slope > 0 && EF_LV_code > 0)
	{
		
		if(Vcore_ADC > EF_LV_code)
		{	
			//calculate value of 0.01mV & transfer from 0.01mV to 0.01V
			Vcore = ((Vcore_ADC - EF_LV_code)*ADC_slope + FT_LV_code + 500)/1000;
		}
		else
		{	
			//calculate value of 0.01mV & transfer from 0.01mV to 0.01V
			//Vcore = (FT_LV_code - (EF_LV_code - Vcore_ADC)*ADC_slope + 500)/1000;
			//Vcore below 0.75V, show error message.
			printk("AVS FAIL: detect Vcore < 0.75V\r\n");
			Vcore = 0;
		}
		return Vcore;
	}
	
	//calculate value of 0.01mV & transfer from 0.01mV to 0.01V for non-efuse
	//return (((Vcore_ADC>>TADC_32_step_shift)*TADC_32_step + 500)/1000);

	printk("AVS FAIL: AVS ADC efuse K data error. Value = 0x%x\r\n", EF_K_value);
	
#endif

	return 0;
}
//EXPORT_SYMBOL(AVS_Get_Vcore);
// if TCSUPPORT_KERNEL_API export from _scu_phy
u32 AVS_Get_Affinity(u32 pid, u32 *mask)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	return sched_getaffinity( pid, mask);
#else
	printk("AVS_Get_Affinity only support when linux version bigger than 2.6.30\r\n");
	return 0;
#endif
}
EXPORT_SYMBOL(AVS_Get_Affinity);

u32 AVS_Set_Affinity(u32 pid, u32 *mask)
{
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,30)
	return sched_setaffinity( pid, mask);
#else
	printk("AVS_Get_Affinity only support when linux version bigger than 2.6.30\r\n");
	return 0;
#endif
}
EXPORT_SYMBOL(AVS_Set_Affinity);
