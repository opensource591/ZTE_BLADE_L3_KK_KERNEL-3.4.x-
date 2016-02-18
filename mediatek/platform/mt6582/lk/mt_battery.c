#include <target/board.h>
#ifdef MTK_KERNEL_POWER_OFF_CHARGING
#define CFG_POWER_CHARGING
#endif
#ifdef CFG_POWER_CHARGING
#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <platform/mt_pmic.h>
#include <platform/boot_mode.h>
#include <platform/mt_gpt.h>
#include <platform/mt_rtc.h>
#include <platform/mt_disp_drv.h>
#include <platform/mtk_wdt.h>
#include <platform/mtk_key.h>
#include <platform/mt_logo.h>
#include <platform/mt_leds.h>
#include <printf.h>
#include <sys/types.h>
#include <target/cust_battery.h>

#undef printf


/*****************************************************************************
 *  Type define
 ****************************************************************************/
#define BATTERY_LOWVOL_THRESOLD             3450


/*****************************************************************************
 *  Global Variable
 ****************************************************************************/
bool g_boot_reason_change = false;


/*****************************************************************************
 *  Externl Variable
 ****************************************************************************/
extern bool g_boot_menu;

#ifdef MTK_FAN5405_SUPPORT
extern void fan5405_hw_init(void);
extern void fan5405_turn_on_charging(void);
extern void fan5405_dump_register(void);
#endif

#ifdef MTK_BQ24196_SUPPORT
extern void bq24196_hw_init(void);
extern void bq24196_charging_enable(UINT8 bEnable);
extern void bq24196_dump_register(void);
extern kal_uint32 bq24196_get_chrg_stat(void);
#endif

#ifdef MTK_BQ24296_SUPPORT
extern void bq24296_hw_init(void);
extern void bq24296_turn_on_charging(void);
extern void bq24296_dump_register(void);
#endif

#ifdef MTK_BQ24158_SUPPORT
extern void bq24158_hw_init(void);
extern void bq24158_turn_on_charging(void);
extern void bq24158_dump_register(void);
#endif

#ifdef MTK_NCP1851_SUPPORT
extern void ncp1851_hw_init(void);
extern void ncp1851_turn_on_charging(void);
extern void ncp1851_dump_register(void);
#endif

#ifdef MTK_NCP1854_SUPPORT
extern void ncp1854_hw_init(void);
extern void ncp1854_turn_on_charging(void);
extern void ncp1854_dump_register(void);
#endif

#ifdef MTK_IPO_POWERPATH_SUPPORT
extern kal_uint32 charging_get_charger_type(void *data);
#endif

void kick_charger_wdt(void)
{
    upmu_set_rg_chrwdt_td(0x0);           // CHRWDT_TD, 4s
    upmu_set_rg_chrwdt_wr(1); 			  // CHRWDT_WR
    upmu_set_rg_chrwdt_int_en(1);         // CHRWDT_INT_EN
    upmu_set_rg_chrwdt_en(1);             // CHRWDT_EN
    upmu_set_rg_chrwdt_flag_wr(1);        // CHRWDT_WR
}

bool is_low_battery(UINT32 val)
{
    static UINT8 g_bat_low = 0xFF;

    //low battery only justice once in lk
    if(0xFF != g_bat_low)
        return g_bat_low;
    else
        g_bat_low = FALSE;

    #if defined(MTK_BQ24196_SUPPORT) || \
        defined(MTK_BQ24296_SUPPORT) || \
        defined(MTK_NCP1851_SUPPORT) || \
        defined(MTK_NCP1854_SUPPORT)
    if(0 == val)
        val = get_i_sense_volt(5);
    #endif

    if (val < BATTERY_LOWVOL_THRESOLD)
    {
        printf("%s, TRUE\n", __func__);
        g_bat_low = 0x1;
    }
    else
    {
        #if defined(MTK_BQ24196_SUPPORT) || defined(MTK_BQ24296_SUPPORT)
        kal_uint32 bq24196_chrg_status;
        bq24196_chrg_status = bq24196_get_chrg_stat();
        printf("bq24196_chrg_status = 0x%x\n", bq24196_chrg_status);

        if(bq24196_chrg_status == 0x1) //Pre-charge
        {
            printf("%s, battery protect TRUE\n", __func__);
            g_bat_low = 0x1;
        }  
        #endif
    }

    if(FALSE == g_bat_low)
        printf("%s, FALSE\n", __func__);

    return g_bat_low;
}


void pchr_turn_on_charging (void)
{
	upmu_set_rg_usbdl_set(0);        //force leave USBDL mode
	upmu_set_rg_usbdl_rst(1);		//force leave USBDL mode
	
	kick_charger_wdt();
	
    upmu_set_rg_cs_vth(0xC);    	// CS_VTH, 450mA            
    upmu_set_rg_csdac_en(1);                // CSDAC_EN
    upmu_set_rg_chr_en(1);                  // CHR_EN  

#ifdef MTK_FAN5405_SUPPORT
    fan5405_hw_init();
    fan5405_turn_on_charging();
    fan5405_dump_register();
#endif

#ifdef MTK_BQ24296_SUPPORT
    bq24296_hw_init();
    bq24296_turn_on_charging();
    bq24296_dump_register();
#endif

#ifdef MTK_NCP1851_SUPPORT
    ncp1851_hw_init();
    ncp1851_turn_on_charging();
    ncp1851_dump_register();
#endif

#ifdef MTK_NCP1854_SUPPORT
    ncp1854_hw_init();
    ncp1854_turn_on_charging();
    ncp1854_dump_register();
#endif

#ifdef MTK_BQ24158_SUPPORT
    bq24158_hw_init();
    bq24158_turn_on_charging();
    bq24158_dump_register();
#endif

#ifdef MTK_BQ24196_SUPPORT
    bq24196_hw_init();
    bq24196_charging_enable(KAL_FALSE);
    bq24196_dump_register();
#endif
}

/* xuqian add begin: add for battery temperature detect */
#define AEON_DETECT_BAT_TEMP_IN_LK

#ifdef AEON_DETECT_BAT_TEMP_IN_LK

typedef long long       kal_int64;

#define BATTERY_HIGHTEMP_THRESOLD             59

#define BAT_NTC_10 0
#define BAT_NTC_47 1

#if (BAT_NTC_10 == 1)
#define RBAT_PULL_UP_R             16900	
#define RBAT_PULL_DOWN_R		   27000	
#endif

#if (BAT_NTC_47 == 1)
#define RBAT_PULL_UP_R             61900	
#define RBAT_PULL_DOWN_R		  100000	
#endif
#define RBAT_PULL_UP_VOLT          1800


typedef struct{
    INT32 BatteryTemp;
    INT32 TemperatureR;
}BATT_TEMPERATURE;


#if (BAT_NTC_10 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
        {-20,68237},
        {-15,53650},
        {-10,42506},
        { -5,33892},
        {  0,27219},
        {  5,22021},
        { 10,17926},
        { 15,14674},
        { 20,12081},
        { 25,10000},
        { 30,8315},
        { 35,6948},
        { 40,5834},
        { 45,4917},
        { 50,4161},
        { 55,3535},
        { 60,3014}
    };
#endif

#if (BAT_NTC_47 == 1)
    BATT_TEMPERATURE Batt_Temperature_Table[] = {
        {-20,483954},
        {-15,360850},
        {-10,271697},
        { -5,206463},
        {  0,158214},
        {  5,122259},
        { 10,95227},
        { 15,74730},
        { 20,59065},
        { 25,47000},
        { 30,37643},
        { 35,30334},
        { 40,24591},
        { 45,20048},
        { 50,16433},
        { 55,13539},
        { 60,11210}        
    };
#endif


int BattThermistorConverTemp(int Res)
{
    int i=0;
    int RES1=0,RES2=0;
    int TBatt_Value=-200,TMP1=0,TMP2=0;

    if(Res>=Batt_Temperature_Table[0].TemperatureR)
    {
        TBatt_Value = -20;
    }
    else if(Res<=Batt_Temperature_Table[16].TemperatureR)
    {
        TBatt_Value = 60;
    }
    else
    {
        RES1=Batt_Temperature_Table[0].TemperatureR;
        TMP1=Batt_Temperature_Table[0].BatteryTemp;

        for(i=0;i<=16;i++)
        {
            if(Res>=Batt_Temperature_Table[i].TemperatureR)
            {
                RES2=Batt_Temperature_Table[i].TemperatureR;
                TMP2=Batt_Temperature_Table[i].BatteryTemp;
                break;
            }
            else
            {
                RES1=Batt_Temperature_Table[i].TemperatureR;
                TMP1=Batt_Temperature_Table[i].BatteryTemp;
            }
        }
        
        TBatt_Value = (((Res-RES2)*TMP1)+((RES1-Res)*TMP2))/(RES1-RES2);
    }

    return TBatt_Value;    
}


int BattVoltToTemp_lk(int dwVolt)
{
    kal_int64 TRes_temp;
	kal_int64 TRes;
    int sBaTTMP = -100;
	
   	TRes_temp = ((kal_int64)RBAT_PULL_UP_R*(kal_int64)dwVolt) / (RBAT_PULL_UP_VOLT-dwVolt); 
	TRes = (TRes_temp * (kal_int64)RBAT_PULL_DOWN_R)/((kal_int64)RBAT_PULL_DOWN_R - TRes_temp);

    /* convert register to temperature */
    sBaTTMP = BattThermistorConverTemp((int)TRes);
      
    return sBaTTMP;
}


int force_get_tbat_lk(void)
{
#if defined(CONFIG_POWER_EXT) || defined(FIXED_TBAT_25)
	printf("[force_get_tbat_lk] fixed TBAT=25 t\n");
    return 25;
#else
    int bat_temperature_volt=0;
    int bat_temperature_val=0;
    
    /* Get V_BAT_Temperature */
    bat_temperature_volt = 2; 
    bat_temperature_volt = get_tbat_volt(5);
    
    if(bat_temperature_volt != 0)
    {       
        bat_temperature_val = BattVoltToTemp_lk(bat_temperature_volt);        
    }
    
    printf("xuqian [force_get_tbat_lk] %d,%d\n", bat_temperature_volt, bat_temperature_val);
    
    return bat_temperature_val;    
//	return 25;
#endif    
}
#endif
/* xuqian add end: add for battery temperature detect */



void mt65xx_bat_init(void)
{    
		kal_int32 bat_vol;

		/* xuqian add begin: add battery temperature in lk */
#ifdef AEON_DETECT_BAT_TEMP_IN_LK
	int bat_temp;

	bat_temp = force_get_tbat_lk();
	printf("xuqian check bat_temp=%d  with %d \n", bat_temp, BATTERY_HIGHTEMP_THRESOLD);
	
	if(bat_temp > BATTERY_HIGHTEMP_THRESOLD)
	{
		if(g_boot_mode != NORMAL_BOOT)
		{
			printf("[%s] Not in NORMAL_BOOT skip temp detect!\n", __func__);
		}
		else
		{
#ifndef NO_POWER_OFF
            mt6575_power_off();
#endif	
		}
	}
#endif
/* xuqian add end: add battery temperature in lk */


		
		#ifdef MTK_IPO_POWERPATH_SUPPORT
		CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
		#endif

		// Low Battery Safety Booting
		
		bat_vol = get_bat_sense_volt(1);
		printf("[mt65xx_bat_init] check VBAT=%d mV with %d mV\n", bat_vol, BATTERY_LOWVOL_THRESOLD);
		
		pchr_turn_on_charging();

		if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT && (upmu_get_pwrkey_deb()==0) ) {
				printf("[mt65xx_bat_init] KPOC+PWRKEY => change boot mode\n");		
		
				g_boot_reason_change = true;
		}
		rtc_boot_check(false);

	#ifndef MTK_DISABLE_POWER_ON_OFF_VOLTAGE_LIMITATION
    //if (bat_vol < BATTERY_LOWVOL_THRESOLD)
    if (is_low_battery(bat_vol))
    {
        if(g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT && upmu_is_chr_det() == KAL_TRUE)
        {
            printf("[%s] Kernel Low Battery Power Off Charging Mode\n", __func__);
            g_boot_mode = LOW_POWER_OFF_CHARGING_BOOT;
            return;
        }
        else
        {
            #ifdef MTK_IPO_POWERPATH_SUPPORT
            //boot linux kernel because of supporting powerpath and using standard AC charger
            if(upmu_is_chr_det() == KAL_TRUE)
            {
                charging_get_charger_type(&CHR_Type_num);
                if(STANDARD_CHARGER == CHR_Type_num)
                {
                    return;
                }
            }
            #endif

            printf("[BATTERY] battery voltage(%dmV) <= CLV ! Can not Boot Linux Kernel !! \n\r",bat_vol);
#ifndef NO_POWER_OFF
            mt6575_power_off();
#endif			
            while(1)
            {
                printf("If you see the log, please check with RTC power off API\n\r");
            }
        }
    }
	#endif
    return;
}

#else

#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h>
#include <printf.h>

void mt65xx_bat_init(void)
{
    printf("[BATTERY] Skip mt65xx_bat_init !!\n\r");
    printf("[BATTERY] If you want to enable power off charging, \n\r");
    printf("[BATTERY] Please #define CFG_POWER_CHARGING!!\n\r");
}

#endif
