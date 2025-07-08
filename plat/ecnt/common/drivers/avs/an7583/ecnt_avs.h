#ifndef _ECNT_AVS
#define _ECNT_AVS

typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;

typedef enum
{
	BGP_TEMP_SENSOR = 0,
	PAD_AVS = 1,
	CORE_POWER = 2,
	AVSDAC_OUT = 3,
	VCM = 4,
	GBE_TEMP_SENSOR = 5,
	CPU_TEMP_SENSOR = 6,
} ADC_MUX_T;

typedef enum
{
	AVS_OK = 0, // OK
	OUT_RANGE_FAIL, 	 //input out of range FAIL
	AVS_FAIL			//HW function fail
} AVS_STATUS_T; // AVS status type

typedef enum
{
	NV = 0, 
	LV, 	 
	HV,		
} V_SEL_T; // AVS status type

//RG address, base: 0x1fa20000
#define PLLRG_PROTECT		0x268
#define MUX_TADC           	0x2E4
#define DOUT_TADC          	0x2F0
#define RG_DATA_AVS_DAC 	0x2e0
#define RG_AVS_OUT_EN 		0x2a0
//PROTECT_KEY
#define PROTECT_KEY        0x80
//TADC
#define TADC_RST_OFST      0x4
#define TADC_MUX_OFST      0x1
#define TADC_SET_MASK      0x1ffff
#define TADC_MUX_MASK      0x7
//DAC
#define DAC_CHG_OFST 16
#define DAC_EN_OFST 0
//ST time
#define DAC_ST_Time 10	//unit ms
#define ADC_ST_Time 10	//unit ms

#define AVS_DAC_step		140		//unit: 0.01mV
//#define TADC_step 		31	//unit: uV
//#define TADC_32_step		180		//standard slope for non-efuse case, unit: 0.01mV, for ADC divide 32
//#define TADC_32_step_shift		5
//#define TADC_16_step		90		//standard slope for non-efuse case, unit: 0.01mV, for ADC divide 16
//#define TADC_16_step_shift		4
#define TADC_gran_step		97			//standard slope for non-efuse case, unit: 0.01mV, granularity related
#define TADC_gran_step_shift		4	//TADC code divider, granularity related


#define FW_DAC_STEP_FINAL		1
#define ST_LV_code			(24576>>TADC_gran_step_shift)	//standard EF LV code for non-efuse case
#define EF_Vdiff			1000000	//1.0V, unit: 0.001mV
#define FT_LV_code  		50000	//0.5V, unit: 0.01mV		
#define Vcore_H_thd			1150 //1.15V, unit: 0.001V
#define Vcore_L_thd			850 //0.85V, unit: 0.001V
#ifdef TCSUPPORT_AUTOBENCH
#define Vcore_H_thd_def			1070 //1.070V, unit: 0.001V
#define Vcore_L_thd_def			835 //0.835V, unit: 0.001V
#else
#define Vcore_H_thd_def			1060 //1.06V, unit: 0.001V
#define Vcore_L_thd_def			940 //0.94V, unit: 0.001V
#endif
#define MAX_AVS_LOOP		300	//300*AVS_DAC_step/2 = 300*0.7 = 210mV for approximate max. tuning range

#define	AVS_Voltage_AVS_Voltage_Cal_1_ADC_LSB /*[15:0]*/            841
#define	AVS_Voltage_AVS_Voltage_Cal_1_ADC_MSB /*[15:0]*/            856
#define	AVS_Voltage_AVS_Voltage_Cal_2_ADC_LSB /*[15:0]*/            857
#define	AVS_Voltage_AVS_Voltage_Cal_2_ADC_MSB /*[15:0]*/            872

#define	AVS_Voltage_AVS_Voltage_Cal_1_ADC_LEN /*[15:0]*/            16
#define	AVS_Voltage_AVS_Voltage_Cal_2_ADC_LEN /*[15:0]*/            16


#define avs_dbg 0

extern u32 AVS_Set_ABS(u32 V_SEL);
extern u32 AVS_Get_Vcore(void);

#endif
