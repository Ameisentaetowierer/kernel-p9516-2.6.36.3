/*
 * kernel/drivers/media/video/tegra
 *
 * Aptina MT9D115 sensor driver
 *
 * Copyright (C) 2010 NVIDIA Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <media/yuv_sensor_ap2031.h>

#define SENSOR_WIDTH_REG      0x2703
#define SENSOR_640_WIDTH_VAL  0x280
#define SENSOR_720_WIDTH_VAL  0x500
#define SENSOR_1600_WIDTH_VAL 0x640

struct sensor_reg {
	u16 addr;
	u16 val;
};

struct sensor_info {
	int mode;
	struct i2c_client *i2c_client;
	struct yuv_sensor_platform_data *pdata;
};

static struct sensor_info *info;


static struct sensor_reg mode_1600x1200[] = {
{0x098C, 0xA115},     // MCU_ADDRESS [SEQ_CAP_MODE]
{0x0990, 0x0000},     // MCU_DATA_0
{0x098C, 0xA116},     // MCU_ADDRESS [SEQ_CAP_NUMFRAMES]
{0x0990, 0x000A},     // MCU_DATA_0
{0x098C, 0xA103},     // MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0002},     // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

/*  CTS testing will set the largest resolution mode only during testZoom.
 *  Fast switchig function will break the testing and fail.
 *  To workaround this, we need to create a 1600x1200 table 
 */
static struct sensor_reg CTS_ZoomTest_mode_1600x1200[] = {
{0x001A, 0x0001},
{SENSOR_WAIT_MS, 10}, 	
{0x001A, 0x0000},
{SENSOR_WAIT_MS, 50}, 	//delay=50

// select output interface
{0x001A, 0x0050},       // BITFIELD=0x001A, 0x0200, 0 // disable Parallel
{0x001A, 0x0058},       // BITFIELD=0x001A, 0x0008, 1 // MIPI
{0x0014, 0x21F9},	// PLL Control: BYPASS PLL = 8697
{0x0010, 0x0115},       // PLL Dividers = 277
{0x0012, 0x00F5},       // wcd = 8
{0x0014, 0x2545},	//PLL Control: TEST_BYPASS on = 9541
{0x0014, 0x2547},	//PLL Control: PLL_ENABLE on = 9543
{0x0014, 0x2447},	//PLL Control: SEL_LOCK_DET on
{SENSOR_WAIT_MS, 100}, 	//DELAY = 1               //Delay 1ms to allow PLL to loc
{0x0014, 0x2047},	//PLL Control: PLL_BYPASS off
{0x0014, 0x2046},	//PLL Control: TEST_BYPASS off
{0x0018, 0x402D},       //LOAD=MCU Powerup Stop Enable
{SENSOR_WAIT_MS, 100}, 	//delay=10
{0x0018, 0x402c},    	// LOAD=GO
{SENSOR_WAIT_MS, 100}, 	//delay=10

{0x098C, 0x2703},	//Output Width (A)
{0x0990, 0x0640},       //      = 1600
{0x098C, 0x2705},	//Output Height (A)
{0x0990, 0x04B0},       //      = 1200
{0x098C, 0x2707},       //Output Width (B)
{0x0990, 0x0640},       //      = 1600
{0x098C, 0x2709},       //Output Height (B)
{0x0990, 0x04B0},       //      = 1200
{0x098C, 0x270D},       //Row Start (A)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x270F},       //Column Start (A)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x2711},       //Row End (A)
{0x0990, 0x04BB},       //      = 1211
{0x098C, 0x2713},	//Column End (A)
{0x0990, 0x064B},       //      = 1611
{0x098C, 0x2715},	//Row Speed (A)
{0x0990, 0x0111},       //      = 273
{0x098C, 0x2717},	//Read Mode (A)
{0x0990, 0x0024},       //      = 36
{0x098C, 0x2719},	//sensor_fine_correction (A)
{0x0990, 0x003A},	//      = 58
{0x098C, 0x271B},	//sensor_fine_IT_min (A)
{0x0990, 0x00F6},	//      = 246
{0x098C, 0x271D},	//sensor_fine_IT_max_margin (A)
{0x0990, 0x008B},	//      = 139
{0x098C, 0x271F},	//Frame Lines (A)
{0x0990, 0x0521},       //      = 1313
{0x098C, 0x2721},	//Line Length (A)
{0x0990, 0x08EC},       //      = 2284
{0x098C, 0x2723},       //Row Start (B)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x2725},       //Column Start (B)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x2727},       //Row End (B)
{0x0990, 0x04BB},       //      = 1211
{0x098C, 0x2729},       //Column End (B)
{0x0990, 0x064B},       //      = 1611
{0x098C, 0x272B},       //Row Speed (B)
{0x0990, 0x0111},       //      = 273
{0x098C, 0x272D},       //Read Mode (B)
{0x0990, 0x0024},       //      = 36
{0x098C, 0x272F},       //sensor_fine_correction (B)
{0x0990, 0x003A},       //      = 58
{0x098C, 0x2731},       //sensor_fine_IT_min (B)
{0x0990, 0x00F6},       //      = 246
{0x098C, 0x2733},       //sensor_fine_IT_max_margin (B)
{0x0990, 0x008B},       //      = 139
{0x098C, 0x2735},       //Frame Lines (B)
{0x0990, 0x0521},       //      = 1313
{0x098C, 0x2737},       //Line Length (B)
{0x0990, 0x08EC},       //      = 2284
{0x098C, 0x2739},	//Crop_X0 (A)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x273B},	//Crop_X1 (A)
{0x0990, 0x063F},       //      = 1599
{0x098C, 0x273D},	//Crop_Y0 (A)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x273F},	//Crop_Y1 (A)
{0x0990, 0x04AF},       //      = 1199
{0x098C, 0x2747},       //Crop_X0 (B)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x2749},       //Crop_X1 (B)
{0x0990, 0x063F},       //      = 1599
{0x098C, 0x274B},       //Crop_Y0 (B)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x274D},       //Crop_Y1 (B)
{0x0990, 0x04AF},       //      = 1199
{0x098C, 0x222D},	//R9 Step
{0x0990, 0x00C5},	//      = 197
{0x098C, 0xA408},	//search_f1_50
{0x0990, 0x0030},	//      = 48
{0x098C, 0xA409},	//search_f2_50
{0x0990, 0x0032},	//      = 50
{0x098C, 0xA40A},	//search_f1_60
{0x0990, 0x003A},	//      = 58
{0x098C, 0xA40B},	//search_f2_60
{0x0990, 0x003C},	//      = 60
{0x098C, 0x2411},	//R9_Step_60 (A)
{0x0990, 0x0099},       //      = 153
{0x098C, 0x2413},	//R9_Step_50 (A)
{0x0990, 0x00B8},       //      = 184
{0x098C, 0x2415},       //R9_Step_60 (B)
{0x0990, 0x0099},       //      = 153
{0x098C, 0x2417},       //R9_Step_50 (B)
{0x0990, 0x00B8},       //      = 184
{0x098C, 0xA404},	//FD Mode
{0x0990, 0x0010},	//      = 16
{0x098C, 0xA40D},	//Stat_min
{0x0990, 0x0002},	//      = 2
{0x098C, 0xA40E},	//Stat_max
{0x0990, 0x0003},	//      = 3
{0x098C, 0xA410},	//Min_amplitude
{0x0990, 0x000A},	//      = 10

// AE settings
{0x098C, 0xA117}, 	// MCU_ADDRESS [SEQ_PREVIEW_0_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA11D}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA129}, 	// MCU_ADDRESS [SEQ_PREVIEW_3_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA24F}, 	// MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x0032}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0xA216},       // MCU_ADDRESS [AE_MAXGAIN23]
{0x0990, 0x0080},       // MCU_DATA_0
{0x098C, 0xA20E}, 	// MCU_ADDRESS [AE_MAX_VIRTGAIN]
{0x0990, 0x0080}, 	// MCU_DATA_0
{0x098C, 0x2212}, 	// MCU_ADDRESS [AE_MAX_DGAIN_AE1]
{0x0990, 0x00A4}, 	// MCU_DATA_02703

// Lens register settings for A-2031SOC (MT9D115) REV1
{0x364E, 0x0650}, 	// P_GR_P0Q0	
{0x3650, 0x3209}, 	// P_GR_P0Q1
{0x3652, 0x5612}, 	// P_GR_P0Q2
{0x3654, 0xB86F}, 	// P_GR_P0Q3
{0x3656, 0xA834}, 	// P_GR_P0Q4
{0x3658, 0x00F0}, 	// P_RD_P0Q0
{0x365A, 0x74AB}, 	// P_RD_P0Q1
{0x365C, 0x4E72}, 	// P_RD_P0Q2
{0x365E, 0x948F}, 	// P_RD_P0Q3
{0x3660, 0xE4B3}, 	// P_RD_P0Q4
{0x3662, 0x01D0}, 	// P_BL_P0Q0
{0x3664, 0x7F6A}, 	// P_BL_P0Q1
{0x3666, 0x1EF2}, 	// P_BL_P0Q2
{0x3668, 0x8FEF}, 	// P_BL_P0Q3
{0x366A, 0xB7B3}, 	// P_BL_P0Q4
{0x366C, 0x0130}, 	// P_GB_P0Q0
{0x366E, 0x1B6B}, 	// P_GB_P0Q1
{0x3670, 0x5FD2}, 	// P_GB_P0Q2
{0x3672, 0xCFEF}, 	// P_GB_P0Q3
{0x3674, 0xB734}, 	// P_GB_P0Q4
{0x3676, 0x256D}, 	// P_GR_P1Q0
{0x3678, 0xB3EF}, 	// P_GR_P1Q1
{0x367A, 0xB5B0}, 	// P_GR_P1Q2
{0x367C, 0x61F2}, 	// P_GR_P1Q3
{0x367E, 0x5193}, 	// P_GR_P1Q4
{0x3680, 0x070E}, 	// P_RD_P1Q0
{0x3682, 0x8F6E}, 	// P_RD_P1Q1
{0x3684, 0x4B8F}, 	// P_RD_P1Q2
{0x3686, 0x16D2}, 	// P_RD_P1Q3
{0x3688, 0xC6F2}, 	// P_RD_P1Q4
{0x368A, 0xA28C}, 	// P_BL_P1Q0
{0x368C, 0xE50D}, 	// P_BL_P1Q1
{0x368E, 0xDE50}, 	// P_BL_P1Q2
{0x3690, 0x5551}, 	// P_BL_P1Q3
{0x3692, 0x7373}, 	// P_BL_P1Q4
{0x3694, 0xEC2B}, 	// P_GB_P1Q0
{0x3696, 0x4C8B}, 	// P_GB_P1Q1
{0x3698, 0xF430}, 	// P_GB_P1Q2
{0x369A, 0x3A92}, 	// P_GB_P1Q3
{0x369C, 0x0054}, 	// P_GB_P1Q4
{0x369E, 0x2E53}, 	// P_GR_P2Q0
{0x36A0, 0x8291}, 	// P_GR_P2Q1
{0x36A2, 0xC936}, 	// P_GR_P2Q2
{0x36A4, 0x1154}, 	// P_GR_P2Q3
{0x36A6, 0x0A79}, 	// P_GR_P2Q4
{0x36A8, 0x14B3}, 	// P_RD_P2Q0
{0x36AA, 0xDEF0}, 	// P_RD_P2Q1
{0x36AC, 0xB2F5}, 	// P_RD_P2Q2
{0x36AE, 0x7D72}, 	// P_RD_P2Q3
{0x36B0, 0x4257}, 	// P_RD_P2Q4
{0x36B2, 0x0A73}, 	// P_BL_P2Q0
{0x36B4, 0x8071}, 	// P_BL_P2Q1
{0x36B6, 0x8EB6}, 	// P_BL_P2Q2
{0x36B8, 0x7BB3}, 	// P_BL_P2Q3
{0x36BA, 0x55B8}, 	// P_BL_P2Q4
{0x36BC, 0x29F3}, 	// P_GB_P2Q0
{0x36BE, 0xF1F0}, 	// P_GB_P2Q1
{0x36C0, 0xD036}, 	// P_GB_P2Q2
{0x36C2, 0x08B4}, 	// P_GB_P2Q3
{0x36C4, 0x0F99}, 	// P_GB_P2Q4
{0x36C6, 0x8B11}, 	// P_GR_P3Q0
{0x36C8, 0x3EF2}, 	// P_GR_P3Q1
{0x36CA, 0x5FD4}, 	// P_GR_P3Q2
{0x36CC, 0x8C76}, 	// P_GR_P3Q3
{0x36CE, 0xDCF6}, 	// P_GR_P3Q4
{0x36D0, 0x9410}, 	// P_RD_P3Q0
{0x36D2, 0x5972}, 	// P_RD_P3Q1
{0x36D4, 0x9853}, 	// P_RD_P3Q2
{0x36D6, 0xA036}, 	// P_RD_P3Q3
{0x36D8, 0x59D6}, 	// P_RD_P3Q4
{0x36DA, 0x86B1}, 	// P_BL_P3Q0
{0x36DC, 0x70D1}, 	// P_BL_P3Q1
{0x36DE, 0x2835}, 	// P_BL_P3Q2
{0x36E0, 0xCC55}, 	// P_BL_P3Q3
{0x36E2, 0xE697}, 	// P_BL_P3Q4
{0x36E4, 0x9B10}, 	// P_GB_P3Q0
{0x36E6, 0x2F12}, 	// P_GB_P3Q1
{0x36E8, 0x7B34}, 	// P_GB_P3Q2
{0x36EA, 0xA976}, 	// P_GB_P3Q3
{0x36EC, 0xAD17}, 	// P_GB_P3Q4
{0x36EE, 0xE635}, 	// P_GR_P4Q0
{0x36F0, 0x0313}, 	// P_GR_P4Q1
{0x36F2, 0x0F59}, 	// P_GR_P4Q2
{0x36F4, 0x7755}, 	// P_GR_P4Q3
{0x36F6, 0xD23A}, 	// P_GR_P4Q4
{0x36F8, 0xF5F4}, 	// P_RD_P4Q0
{0x36FA, 0x0973}, 	// P_RD_P4Q1
{0x36FC, 0x4154}, 	// P_RD_P4Q2
{0x36FE, 0x10F6}, 	// P_RD_P4Q3
{0x3700, 0x18BA}, 	// P_RD_P4Q4
{0x3702, 0x9F35}, 	// P_BL_P4Q0
{0x3704, 0x6773}, 	// P_BL_P4Q1
{0x3706, 0x6DD8}, 	// P_BL_P4Q2
{0x3708, 0xFB53}, 	// P_BL_P4Q3
{0x370A, 0xDCBA}, 	// P_BL_P4Q4
{0x370C, 0xE0F5}, 	// P_GB_P4Q0
{0x370E, 0x3352}, 	// P_GB_P4Q1
{0x3710, 0x17B9}, 	// P_GB_P4Q2
{0x3712, 0x00F6}, 	// P_GB_P4Q3
{0x3714, 0xF9DA}, 	// P_GB_P4Q4
{0x3644, 0x0344}, 	// POLY_ORIGIN_C
{0x3642, 0x0270}, 	// POLY_ORIGIN_R
{0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL

// GAMMA Setting
{0x098C, 0xAB37}, 	// MCU_ADDRESS [HG_GAMMA_MORPH_CTRL]
{0x0990, 0x0003}, 	// MCU_DATA_0
{0x098C, 0x2B38}, 	// MCU_ADDRESS [HG_GAMMASTARTMORPH]
{0x0990, 0x2968}, 	// MCU_DATA_0
{0x098C, 0x2B3A}, 	// MCU_ADDRESS [HG_GAMMASTOPMORPH]
{0x0990, 0x2D50}, 	// MCU_DATA_0
{0x098C, 0x2B62}, 	// MCU_ADDRESS [HG_FTB_START_BM]
{0x0990, 0xFFFE}, 	// MCU_DATA_0
{0x098C, 0x2B64}, 	// MCU_ADDRESS [HG_FTB_STOP_BM]
{0x0990, 0xFFFF}, 	// MCU_DATA_0

{0x098C, 0xAB4F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB50}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
{0x0990, 0x0013}, 	// MCU_DATA_0
{0x098C, 0xAB51}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
{0x0990, 0x0027}, 	// MCU_DATA_0
{0x098C, 0xAB52}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
{0x0990, 0x0043}, 	// MCU_DATA_0
{0x098C, 0xAB53}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
{0x0990, 0x0068}, 	// MCU_DATA_0
{0x098C, 0xAB54}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
{0x0990, 0x0081}, 	// MCU_DATA_0
{0x098C, 0xAB55}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
{0x0990, 0x0093}, 	// MCU_DATA_0
{0x098C, 0xAB56}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
{0x0990, 0x00A3}, 	// MCU_DATA_0
{0x098C, 0xAB57}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
{0x0990, 0x00B0}, 	// MCU_DATA_0
{0x098C, 0xAB58}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
{0x0990, 0x00BC}, 	// MCU_DATA_0
{0x098C, 0xAB59}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
{0x0990, 0x00C7}, 	// MCU_DATA_0
{0x098C, 0xAB5A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
{0x0990, 0x00D1}, 	// MCU_DATA_0
{0x098C, 0xAB5B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
{0x0990, 0x00DA}, 	// MCU_DATA_0
{0x098C, 0xAB5C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
{0x0990, 0x00E2}, 	// MCU_DATA_0
{0x098C, 0xAB5D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
{0x0990, 0x00E9}, 	// MCU_DATA_0
{0x098C, 0xAB5E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
{0x0990, 0x00EF}, 	// MCU_DATA_0
{0x098C, 0xAB5F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
{0x0990, 0x00F4}, 	// MCU_DATA_0
{0x098C, 0xAB60}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
{0x0990, 0x00FA}, 	// MCU_DATA_0
{0x098C, 0xAB61}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
{0x0990, 0x00FF}, 	// MCU_DATA_0

// CCM
{0x098C, 0x2306}, 	// MCU_ADDRESS [AWB_CCM_L_0]
{0x0990, 0x01D6}, 	// MCU_DATA_0
{0x098C, 0x2308}, 	// MCU_ADDRESS [AWB_CCM_L_1]
{0x0990, 0xFF89}, 	// MCU_DATA_0
{0x098C, 0x230A}, 	// MCU_ADDRESS [AWB_CCM_L_2]
{0x0990, 0xFFA1}, 	// MCU_DATA_0
{0x098C, 0x230C}, 	// MCU_ADDRESS [AWB_CCM_L_3]
{0x0990, 0xFF73}, 	// MCU_DATA_0
{0x098C, 0x230E}, 	// MCU_ADDRESS [AWB_CCM_L_4]
{0x0990, 0x019C}, 	// MCU_DATA_0
{0x098C, 0x2310}, 	// MCU_ADDRESS [AWB_CCM_L_5]
{0x0990, 0xFFF1}, 	// MCU_DATA_0
{0x098C, 0x2312}, 	// MCU_ADDRESS [AWB_CCM_L_6]
{0x0990, 0xFFB0}, 	// MCU_DATA_0
{0x098C, 0x2314}, 	// MCU_ADDRESS [AWB_CCM_L_7]
{0x0990, 0xFF2D}, 	// MCU_DATA_0
{0x098C, 0x2316}, 	// MCU_ADDRESS [AWB_CCM_L_8]
{0x0990, 0x0223}, 	// MCU_DATA_0
{0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
{0x0990, 0x001C}, 	// MCU_DATA_0
{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
{0x0990, 0x0054}, 	// MCU_DATA_0
{0x098C, 0x231C}, 	// MCU_ADDRESS [AWB_CCM_RL_0]
{0x0990, 0xFFCD}, 	// MCU_DATA_0
{0x098C, 0x231E}, 	// MCU_ADDRESS [AWB_CCM_RL_1]
{0x0990, 0x0023}, 	// MCU_DATA_0
{0x098C, 0x2320}, 	// MCU_ADDRESS [AWB_CCM_RL_2]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0x2322}, 	// MCU_ADDRESS [AWB_CCM_RL_3]
{0x0990, 0x0026}, 	// MCU_DATA_0
{0x098C, 0x2324}, 	// MCU_ADDRESS [AWB_CCM_RL_4]
{0x0990, 0xFFE9}, 	// MCU_DATA_0
{0x098C, 0x2326}, 	// MCU_ADDRESS [AWB_CCM_RL_5]
{0x0990, 0xFFF1}, 	// MCU_DATA_0
{0x098C, 0x2328}, 	// MCU_ADDRESS [AWB_CCM_RL_6]
{0x0990, 0x003A}, 	// MCU_DATA_0
{0x098C, 0x232A}, 	// MCU_ADDRESS [AWB_CCM_RL_7]
{0x0990, 0x005D}, 	// MCU_DATA_0
{0x098C, 0x232C}, 	// MCU_ADDRESS [AWB_CCM_RL_8]
{0x0990, 0xFF69}, 	// MCU_DATA_0
{0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
{0x0990, 0x000C}, 	// MCU_DATA_0
{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
{0x0990, 0xFFD8}, 	// MCU_DATA_0

// SOC2031 patch
{0x098C, 0x0415},
{0x0990, 0xF601},
{0x0992, 0x42C1},
{0x0994, 0x0326},
{0x0996, 0x11F6},
{0x0998, 0x0143},
{0x099A, 0xC104},
{0x099C, 0x260A},
{0x099E, 0xCC04},
{0x098C, 0x0425},
{0x0990, 0x33BD},
{0x0992, 0xA362},
{0x0994, 0xBD04},
{0x0996, 0x3339},
{0x0998, 0xC6FF},
{0x099A, 0xF701},
{0x099C, 0x6439},
{0x099E, 0xFE01},
{0x098C, 0x0435},
{0x0990, 0x6918},
{0x0992, 0xCE03},
{0x0994, 0x25CC},
{0x0996, 0x0013},
{0x0998, 0xBDC2},
{0x099A, 0xB8CC},
{0x099C, 0x0489},
{0x099E, 0xFD03},
{0x098C, 0x0445},
{0x0990, 0x27CC},
{0x0992, 0x0325},
{0x0994, 0xFD01},
{0x0996, 0x69FE},
{0x0998, 0x02BD},
{0x099A, 0x18CE},
{0x099C, 0x0339},
{0x099E, 0xCC00},
{0x098C, 0x0455},
{0x0990, 0x11BD},
{0x0992, 0xC2B8},
{0x0994, 0xCC04},
{0x0996, 0xC8FD},
{0x0998, 0x0347},
{0x099A, 0xCC03},
{0x099C, 0x39FD},
{0x099E, 0x02BD},
{0x098C, 0x0465},
{0x0990, 0xDE00},
{0x0992, 0x18CE},
{0x0994, 0x00C2},
{0x0996, 0xCC00},
{0x0998, 0x37BD},
{0x099A, 0xC2B8},
{0x099C, 0xCC04},
{0x099E, 0xEFDD},
{0x098C, 0x0475},
{0x0990, 0xE6CC},
{0x0992, 0x00C2},
{0x0994, 0xDD00},
{0x0996, 0xC601},
{0x0998, 0xF701},
{0x099A, 0x64C6},
{0x099C, 0x03F7},
{0x099E, 0x0165},
{0x098C, 0x0485},
{0x0990, 0x7F01},
{0x0992, 0x6639},
{0x0994, 0x3C3C},
{0x0996, 0x3C34},
{0x0998, 0xCC32},
{0x099A, 0x3EBD},
{0x099C, 0xA558},
{0x099E, 0x30ED},
{0x098C, 0x0495},
{0x0990, 0x04BD},
{0x0992, 0xB2D7},
{0x0994, 0x30E7},
{0x0996, 0x06CC},
{0x0998, 0x323E},
{0x099A, 0xED00},
{0x099C, 0xEC04},
{0x099E, 0xBDA5},
{0x098C, 0x04A5},
{0x0990, 0x44CC},
{0x0992, 0x3244},
{0x0994, 0xBDA5},
{0x0996, 0x585F},
{0x0998, 0x30ED},
{0x099A, 0x02CC},
{0x099C, 0x3244},
{0x099E, 0xED00},
{0x098C, 0x04B5},
{0x0990, 0xF601},
{0x0992, 0xD54F},
{0x0994, 0xEA03},
{0x0996, 0xAA02},
{0x0998, 0xBDA5},
{0x099A, 0x4430},
{0x099C, 0xE606},
{0x099E, 0x3838},
{0x098C, 0x04C5},
{0x0990, 0x3831},
{0x0992, 0x39BD},
{0x0994, 0xD661},
{0x0996, 0xF602},
{0x0998, 0xF4C1},
{0x099A, 0x0126},
{0x099C, 0x0BFE},
{0x099E, 0x02BD},
{0x098C, 0x04D5},
{0x0990, 0xEE10},
{0x0992, 0xFC02},
{0x0994, 0xF5AD},
{0x0996, 0x0039},
{0x0998, 0xF602},
{0x099A, 0xF4C1},
{0x099C, 0x0226},
{0x099E, 0x0AFE},
{0x098C, 0x04E5},
{0x0990, 0x02BD},
{0x0992, 0xEE10},
{0x0994, 0xFC02},
{0x0996, 0xF7AD},
{0x0998, 0x0039},
{0x099A, 0x3CBD},
{0x099C, 0xB059},
{0x099E, 0xCC00},
{0x098C, 0x04F5},
{0x0990, 0x28BD},
{0x0992, 0xA558},
{0x0994, 0x8300},
{0x0996, 0x0027},
{0x0998, 0x0BCC},
{0x099A, 0x0026},
{0x099C, 0x30ED},
{0x099E, 0x00C6},
{0x098C, 0x0505},
{0x0990, 0x03BD},
{0x0992, 0xA544},
{0x0994, 0x3839},

{0x098C, 0x2006}, 	// MCU_ADDRESS [MON_ARG1]
{0x0990, 0x0415}, 	// MCU_DATA_0
{0x098C, 0xA005}, 	// MCU_ADDRESS [MON_CMD]
{0x0990, 0x0001}, 	// MCU_DATA_0
{SENSOR_WAIT_MS, 50}, 	//delay = 50  //  POLL  MON_PATCH_ID_0 =>  0x01

// Errata for Silicon Rev
{0x0018, 0x0028},
{0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
{0x0990, 0x0040}, 	// MCU_DATA_0
{0x098C, 0xAB24}, 	// MCU_ADDRESS [HG_LL_SAT2]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
{0x0990, 0x0030}, 	// MCU_DATA_0
{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
{0x0990, 0xFFFC}, 	// MCU_DATA_0
{0x098C, 0xAB2C}, 	// MCU_ADDRESS [HG_NR_START_R]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2D}, 	// MCU_ADDRESS [HG_NR_START_G]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2E}, 	// MCU_ADDRESS [HG_NR_START_B]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2F}, 	// MCU_ADDRESS [HG_NR_START_OL]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB34}, 	// MCU_ADDRESS [HG_NR_GAINSTART]
{0x0990, 0x0096}, 	// MCU_DATA_0

// syncronize the FW with the sensor
{SENSOR_WAIT_MS, 50},	// delay=50
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006},	// MCU_DATA_0
{SENSOR_WAIT_MS, 100},
{0x098C, 0xA103},	// MCU_ADDRES
{0x0990, 0x0005},	// MCU_DATA_0
{SENSOR_WAIT_MS, 100},

{0x3400, 0x7A20},	// set to discontinous clock

{SENSOR_TABLE_END, 0x0000}
};


static struct sensor_reg mode_1280x720[] = {
// Reset the sensor
{0x001A, 0x0001},
{SENSOR_WAIT_MS, 10}, 	
{0x001A, 0x0000},
{SENSOR_WAIT_MS, 50}, 	//delay=50

// select output interface
{0x001A, 0x0050},       // BITFIELD=0x001A, 0x0200, 0 // disable Parallel
{0x001A, 0x0058},       // BITFIELD=0x001A, 0x0008, 1 // MIPI

// program the on-chip PLL
{0x0014, 0x21F9},	// PLL Control: BYPASS PLL = 8697
{0x0010, 0x0115},       // PLL Dividers = 277
{0x0012, 0x00F5},       // wcd = 8
{0x0014, 0x2545},	// PLL Control: TEST_BYPASS on = 9541
{0x0014, 0x2547},	// PLL Control: PLL_ENABLE on = 9543
{0x0014, 0x2447},	// PLL Control: SEL_LOCK_DET on
{SENSOR_WAIT_MS, 1}, 	// Delay 1ms to allow PLL to loc
{0x0014, 0x2047},	// PLL Control: PLL_BYPASS off
{0x0014, 0x2046},	// PLL Control: TEST_BYPASS off

// enable powerup stop
{0x0018, 0x402D},       //LOAD=MCU Powerup Stop Enable
{SENSOR_WAIT_MS, 10}, 	//delay=10

// start MCU, includes wait for standby_done to clear
{0x0018, 0x402c},    	// LOAD=GO
{SENSOR_WAIT_MS, 10}, 	//delay=10

// sensor core & flicker timings
{0x098C, 0x2703},	//Output Width (A)
{0x0990, 0x0500},	//      = 1280
{0x098C, 0x2705},	//Output Height (A)
{0x0990, 0x02D0},	//      = 720
{0x098C, 0x2707},       //Output Width (B)
{0x0990, 0x0640},       //      = 1600
{0x098C, 0x2709},       //Output Height (B)
{0x0990, 0x04B0},       //      = 1200
{0x098C, 0x270D},       //Row Start (A)
{0x0990, 0x00F6},       //      = 246
{0x098C, 0x270F},       //Column Start (A)
{0x0990, 0x00A6},       //      = 166
{0x098C, 0x2711},       //Row End (A)
{0x0990, 0x03CD},       //      = 973
{0x098C, 0x2713},	//Column End (A)
{0x0990, 0x05AD},       //      = 1453
{0x098C, 0x2715},	//Row Speed (A)
{0x0990, 0x0111},	//      = 273
{0x098C, 0x2717},	//Read Mode (A)
{0x0990, 0x0024},	//      = 36
{0x098C, 0x2719},	//sensor_fine_correction (A)
{0x0990, 0x003A},	//      = 58
{0x098C, 0x271B},	//sensor_fine_IT_min (A)
{0x0990, 0x00F6},	//      = 246
{0x098C, 0x271D},	//sensor_fine_IT_max_margin (A)
{0x0990, 0x008B},	//      = 139
{0x098C, 0x271F},	//Frame Lines (A)
{0x0990, 0x032D},	//      = 813
{0x098C, 0x2721},	//Line Length (A)
{0x0990, 0x06F4},	//      = 1780
{0x098C, 0x2723},       //Row Start (B)
{0x0990, 0x0004},       //      = 4
{0x098C, 0x2725},       //Column Start (B)
{0x0990, 0x0004},       //      = 4
{0x098C, 0x2727},       //Row End (B)
{0x0990, 0x04BB},       //      = 1211
{0x098C, 0x2729},       //Column End (B)
{0x0990, 0x064B},       //      = 1611
{0x098C, 0x272B},       //Row Speed (B)
{0x0990, 0x0111},       //      = 273
{0x098C, 0x272D},       //Read Mode (B)
{0x0990, 0x0024},       //      = 36
{0x098C, 0x272F},       //sensor_fine_correction (B)
{0x0990, 0x003A},       //      = 58
{0x098C, 0x2731},       //sensor_fine_IT_min (B)
{0x0990, 0x00F6},       //      = 246
{0x098C, 0x2733},       //sensor_fine_IT_max_margin (B)
{0x0990, 0x008B},       //      = 139
{0x098C, 0x2735},       //Frame Lines (B)
{0x0990, 0x0521},       //      = 1313
{0x098C, 0x2737},       //Line Length (B)
{0x0990, 0x08EC},       //      = 2284
{0x098C, 0x2739},	//Crop_X0 (A)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x273B},	//Crop_X1 (A)
{0x0990, 0x04FF},	//      = 1279
{0x098C, 0x273D},	//Crop_Y0 (A)
{0x0990, 0x0000},	//      = 0
{0x098C, 0x273F},	//Crop_Y1 (A)
{0x0990, 0x02CF},	//      = 719
{0x098C, 0x2747},       //Crop_X0 (B)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x2749},       //Crop_X1 (B)
{0x0990, 0x063F},       //      = 1599
{0x098C, 0x274B},       //Crop_Y0 (B)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x274D},       //Crop_Y1 (B)
{0x0990, 0x04AF},       //      = 1199

{0x098C, 0x222D},	//R9 Step
{0x0990, 0x00C5},	//      = 197
{0x098C, 0xA408},	//search_f1_50
{0x0990, 0x0030},	//      = 48
{0x098C, 0xA409},	//search_f2_50
{0x0990, 0x0032},	//      = 50
{0x098C, 0xA40A},	//search_f1_60
{0x0990, 0x003A},	//      = 58
{0x098C, 0xA40B},	//search_f2_60
{0x0990, 0x003C},	//      = 60
{0x098C, 0x2411},	//R9_Step_60 (A)
{0x0990, 0x00C5},	//      = 197
{0x098C, 0x2413},	//R9_Step_50 (A)
{0x0990, 0x00EC},	//      = 236
{0x098C, 0x2415},       //R9_Step_60 (B)
{0x0990, 0x0099},       //      = 153
{0x098C, 0x2417},       //R9_Step_50 (B)
{0x0990, 0x00B8},       //      = 184
{0x098C, 0xA404},	//FD Mode
{0x0990, 0x0010},	//      = 16
{0x098C, 0xA40D},	//Stat_min
{0x0990, 0x0002},	//      = 2
{0x098C, 0xA40E},	//Stat_max
{0x0990, 0x0003},	//      = 3
{0x098C, 0xA410},	//Min_amplitude
{0x0990, 0x000A},	//      = 10

// AE settings
{0x098C, 0xA117}, 	// MCU_ADDRESS [SEQ_PREVIEW_0_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA11D}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA129}, 	// MCU_ADDRESS [SEQ_PREVIEW_3_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA24F}, 	// MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x0032}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0xA216},       // MCU_ADDRESS [AE_MAXGAIN23]
{0x0990, 0x0080},       // MCU_DATA_0
{0x098C, 0xA20E}, 	// MCU_ADDRESS [AE_MAX_VIRTGAIN]
{0x0990, 0x0080}, 	// MCU_DATA_0
{0x098C, 0x2212}, 	// MCU_ADDRESS [AE_MAX_DGAIN_AE1]
{0x0990, 0x00A4}, 	// MCU_DATA_02703

// Lens register settings for A-2031SOC (MT9D115) REV1
{0x364E, 0x0650}, 	// P_GR_P0Q0
{0x3650, 0x3209}, 	// P_GR_P0Q1
{0x3652, 0x5612}, 	// P_GR_P0Q2
{0x3654, 0xB86F}, 	// P_GR_P0Q3
{0x3656, 0xA834}, 	// P_GR_P0Q4
{0x3658, 0x00F0}, 	// P_RD_P0Q0
{0x365A, 0x74AB}, 	// P_RD_P0Q1
{0x365C, 0x4E72}, 	// P_RD_P0Q2
{0x365E, 0x948F}, 	// P_RD_P0Q3
{0x3660, 0xE4B3}, 	// P_RD_P0Q4
{0x3662, 0x01D0}, 	// P_BL_P0Q0
{0x3664, 0x7F6A}, 	// P_BL_P0Q1
{0x3666, 0x1EF2}, 	// P_BL_P0Q2
{0x3668, 0x8FEF}, 	// P_BL_P0Q3
{0x366A, 0xB7B3}, 	// P_BL_P0Q4
{0x366C, 0x0130}, 	// P_GB_P0Q0
{0x366E, 0x1B6B}, 	// P_GB_P0Q1
{0x3670, 0x5FD2}, 	// P_GB_P0Q2
{0x3672, 0xCFEF}, 	// P_GB_P0Q3
{0x3674, 0xB734}, 	// P_GB_P0Q4
{0x3676, 0x256D}, 	// P_GR_P1Q0
{0x3678, 0xB3EF}, 	// P_GR_P1Q1
{0x367A, 0xB5B0}, 	// P_GR_P1Q2
{0x367C, 0x61F2}, 	// P_GR_P1Q3
{0x367E, 0x5193}, 	// P_GR_P1Q4
{0x3680, 0x070E}, 	// P_RD_P1Q0
{0x3682, 0x8F6E}, 	// P_RD_P1Q1
{0x3684, 0x4B8F}, 	// P_RD_P1Q2
{0x3686, 0x16D2}, 	// P_RD_P1Q3
{0x3688, 0xC6F2}, 	// P_RD_P1Q4
{0x368A, 0xA28C}, 	// P_BL_P1Q0
{0x368C, 0xE50D}, 	// P_BL_P1Q1
{0x368E, 0xDE50}, 	// P_BL_P1Q2
{0x3690, 0x5551}, 	// P_BL_P1Q3
{0x3692, 0x7373}, 	// P_BL_P1Q4
{0x3694, 0xEC2B}, 	// P_GB_P1Q0
{0x3696, 0x4C8B}, 	// P_GB_P1Q1
{0x3698, 0xF430}, 	// P_GB_P1Q2
{0x369A, 0x3A92}, 	// P_GB_P1Q3
{0x369C, 0x0054}, 	// P_GB_P1Q4
{0x369E, 0x2E53}, 	// P_GR_P2Q0
{0x36A0, 0x8291}, 	// P_GR_P2Q1
{0x36A2, 0xC936}, 	// P_GR_P2Q2
{0x36A4, 0x1154}, 	// P_GR_P2Q3
{0x36A6, 0x0A79}, 	// P_GR_P2Q4
{0x36A8, 0x14B3}, 	// P_RD_P2Q0
{0x36AA, 0xDEF0}, 	// P_RD_P2Q1
{0x36AC, 0xB2F5}, 	// P_RD_P2Q2
{0x36AE, 0x7D72}, 	// P_RD_P2Q3
{0x36B0, 0x4257}, 	// P_RD_P2Q4
{0x36B2, 0x0A73}, 	// P_BL_P2Q0
{0x36B4, 0x8071}, 	// P_BL_P2Q1
{0x36B6, 0x8EB6}, 	// P_BL_P2Q2
{0x36B8, 0x7BB3}, 	// P_BL_P2Q3
{0x36BA, 0x55B8}, 	// P_BL_P2Q4
{0x36BC, 0x29F3}, 	// P_GB_P2Q0
{0x36BE, 0xF1F0}, 	// P_GB_P2Q1
{0x36C0, 0xD036}, 	// P_GB_P2Q2
{0x36C2, 0x08B4}, 	// P_GB_P2Q3
{0x36C4, 0x0F99}, 	// P_GB_P2Q4
{0x36C6, 0x8B11}, 	// P_GR_P3Q0
{0x36C8, 0x3EF2}, 	// P_GR_P3Q1
{0x36CA, 0x5FD4}, 	// P_GR_P3Q2
{0x36CC, 0x8C76}, 	// P_GR_P3Q3
{0x36CE, 0xDCF6}, 	// P_GR_P3Q4
{0x36D0, 0x9410}, 	// P_RD_P3Q0
{0x36D2, 0x5972}, 	// P_RD_P3Q1
{0x36D4, 0x9853}, 	// P_RD_P3Q2
{0x36D6, 0xA036}, 	// P_RD_P3Q3
{0x36D8, 0x59D6}, 	// P_RD_P3Q4
{0x36DA, 0x86B1}, 	// P_BL_P3Q0
{0x36DC, 0x70D1}, 	// P_BL_P3Q1
{0x36DE, 0x2835}, 	// P_BL_P3Q2
{0x36E0, 0xCC55}, 	// P_BL_P3Q3
{0x36E2, 0xE697}, 	// P_BL_P3Q4
{0x36E4, 0x9B10}, 	// P_GB_P3Q0
{0x36E6, 0x2F12}, 	// P_GB_P3Q1
{0x36E8, 0x7B34}, 	// P_GB_P3Q2
{0x36EA, 0xA976}, 	// P_GB_P3Q3
{0x36EC, 0xAD17}, 	// P_GB_P3Q4
{0x36EE, 0xE635}, 	// P_GR_P4Q0
{0x36F0, 0x0313}, 	// P_GR_P4Q1
{0x36F2, 0x0F59}, 	// P_GR_P4Q2
{0x36F4, 0x7755}, 	// P_GR_P4Q3
{0x36F6, 0xD23A}, 	// P_GR_P4Q4
{0x36F8, 0xF5F4}, 	// P_RD_P4Q0
{0x36FA, 0x0973}, 	// P_RD_P4Q1
{0x36FC, 0x4154}, 	// P_RD_P4Q2
{0x36FE, 0x10F6}, 	// P_RD_P4Q3
{0x3700, 0x18BA}, 	// P_RD_P4Q4
{0x3702, 0x9F35}, 	// P_BL_P4Q0
{0x3704, 0x6773}, 	// P_BL_P4Q1
{0x3706, 0x6DD8}, 	// P_BL_P4Q2
{0x3708, 0xFB53}, 	// P_BL_P4Q3
{0x370A, 0xDCBA}, 	// P_BL_P4Q4
{0x370C, 0xE0F5}, 	// P_GB_P4Q0
{0x370E, 0x3352}, 	// P_GB_P4Q1
{0x3710, 0x17B9}, 	// P_GB_P4Q2
{0x3712, 0x00F6}, 	// P_GB_P4Q3
{0x3714, 0xF9DA}, 	// P_GB_P4Q4
{0x3644, 0x0344}, 	// POLY_ORIGIN_C
{0x3642, 0x0270}, 	// POLY_ORIGIN_R
{0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL

// GAMMA Setting
{0x098C, 0xAB37}, 	// MCU_ADDRESS [HG_GAMMA_MORPH_CTRL]
{0x0990, 0x0003}, 	// MCU_DATA_0
{0x098C, 0x2B38}, 	// MCU_ADDRESS [HG_GAMMASTARTMORPH]
{0x0990, 0x2968}, 	// MCU_DATA_0
{0x098C, 0x2B3A}, 	// MCU_ADDRESS [HG_GAMMASTOPMORPH]
{0x0990, 0x2D50}, 	// MCU_DATA_0
{0x098C, 0x2B62}, 	// MCU_ADDRESS [HG_FTB_START_BM]
{0x0990, 0xFFFE}, 	// MCU_DATA_0
{0x098C, 0x2B64}, 	// MCU_ADDRESS [HG_FTB_STOP_BM]
{0x0990, 0xFFFF}, 	// MCU_DATA_0
{0x098C, 0xAB4F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB50}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
{0x0990, 0x0013}, 	// MCU_DATA_0
{0x098C, 0xAB51}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
{0x0990, 0x0027}, 	// MCU_DATA_0
{0x098C, 0xAB52}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
{0x0990, 0x0043}, 	// MCU_DATA_0
{0x098C, 0xAB53}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
{0x0990, 0x0068}, 	// MCU_DATA_0
{0x098C, 0xAB54}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
{0x0990, 0x0081}, 	// MCU_DATA_0
{0x098C, 0xAB55}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
{0x0990, 0x0093}, 	// MCU_DATA_0
{0x098C, 0xAB56}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
{0x0990, 0x00A3}, 	// MCU_DATA_0
{0x098C, 0xAB57}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
{0x0990, 0x00B0}, 	// MCU_DATA_0
{0x098C, 0xAB58}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
{0x0990, 0x00BC}, 	// MCU_DATA_0
{0x098C, 0xAB59}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
{0x0990, 0x00C7}, 	// MCU_DATA_0
{0x098C, 0xAB5A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
{0x0990, 0x00D1}, 	// MCU_DATA_0
{0x098C, 0xAB5B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
{0x0990, 0x00DA}, 	// MCU_DATA_0
{0x098C, 0xAB5C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
{0x0990, 0x00E2}, 	// MCU_DATA_0
{0x098C, 0xAB5D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
{0x0990, 0x00E9}, 	// MCU_DATA_0
{0x098C, 0xAB5E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
{0x0990, 0x00EF}, 	// MCU_DATA_0
{0x098C, 0xAB5F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
{0x0990, 0x00F4}, 	// MCU_DATA_0
{0x098C, 0xAB60}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
{0x0990, 0x00FA}, 	// MCU_DATA_0
{0x098C, 0xAB61}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
{0x0990, 0x00FF}, 	// MCU_DATA_0

// CCM
{0x098C, 0x2306}, 	// MCU_ADDRESS [AWB_CCM_L_0]
{0x0990, 0x01D6}, 	// MCU_DATA_0
{0x098C, 0x2308}, 	// MCU_ADDRESS [AWB_CCM_L_1]
{0x0990, 0xFF89}, 	// MCU_DATA_0
{0x098C, 0x230A}, 	// MCU_ADDRESS [AWB_CCM_L_2]
{0x0990, 0xFFA1}, 	// MCU_DATA_0
{0x098C, 0x230C}, 	// MCU_ADDRESS [AWB_CCM_L_3]
{0x0990, 0xFF73}, 	// MCU_DATA_0
{0x098C, 0x230E}, 	// MCU_ADDRESS [AWB_CCM_L_4]
{0x0990, 0x019C}, 	// MCU_DATA_0
{0x098C, 0x2310}, 	// MCU_ADDRESS [AWB_CCM_L_5]
{0x0990, 0xFFF1}, 	// MCU_DATA_0
{0x098C, 0x2312}, 	// MCU_ADDRESS [AWB_CCM_L_6]
{0x0990, 0xFFB0}, 	// MCU_DATA_0
{0x098C, 0x2314}, 	// MCU_ADDRESS [AWB_CCM_L_7]
{0x0990, 0xFF2D}, 	// MCU_DATA_0
{0x098C, 0x2316}, 	// MCU_ADDRESS [AWB_CCM_L_8]
{0x0990, 0x0223}, 	// MCU_DATA_0
{0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
{0x0990, 0x001C}, 	// MCU_DATA_0
{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
{0x0990, 0x0054}, 	// MCU_DATA_0
{0x098C, 0x231C}, 	// MCU_ADDRESS [AWB_CCM_RL_0]
{0x0990, 0xFFCD}, 	// MCU_DATA_0
{0x098C, 0x231E}, 	// MCU_ADDRESS [AWB_CCM_RL_1]
{0x0990, 0x0023},	// MCU_DATA_0
{0x098C, 0x2320}, 	// MCU_ADDRESS [AWB_CCM_RL_2]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0x2322}, 	// MCU_ADDRESS [AWB_CCM_RL_3]
{0x0990, 0x0026}, 	// MCU_DATA_0
{0x098C, 0x2324}, 	// MCU_ADDRESS [AWB_CCM_RL_4]
{0x0990, 0xFFE9}, 	// MCU_DATA_0
{0x098C, 0x2326}, 	// MCU_ADDRESS [AWB_CCM_RL_5]
{0x0990, 0xFFF1}, 	// MCU_DATA_0
{0x098C, 0x2328}, 	// MCU_ADDRESS [AWB_CCM_RL_6]
{0x0990, 0x003A}, 	// MCU_DATA_0
{0x098C, 0x232A}, 	// MCU_ADDRESS [AWB_CCM_RL_7]
{0x0990, 0x005D}, 	// MCU_DATA_0
{0x098C, 0x232C}, 	// MCU_ADDRESS [AWB_CCM_RL_8]
{0x0990, 0xFF69}, 	// MCU_DATA_0
{0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
{0x0990, 0x000C}, 	// MCU_DATA_0
{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
{0x0990, 0xFFD8}, 	// MCU_DATA_0

{0x0018, 0x0028},
{SENSOR_WAIT_MS, 100}, 	//delay = 50  //  POLL  MON_PATCH_ID_0 =>  0x01

{0x098C, 0xAB20}, 	// MCU_ADDRESS [HG_LL_SAT1]
{0x0990, 0x0040}, 	// MCU_DATA_0
{0x098C, 0xAB24}, 	// MCU_ADDRESS [HG_LL_SAT2]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
{0x0990, 0x0030}, 	// MCU_DATA_0
{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
{0x0990, 0xFFFC}, 	// MCU_DATA_0
{0x098C, 0xAB2C}, 	// MCU_ADDRESS [HG_NR_START_R]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2D}, 	// MCU_ADDRESS [HG_NR_START_G]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2E}, 	// MCU_ADDRESS [HG_NR_START_B]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2F}, 	// MCU_ADDRESS [HG_NR_START_OL]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB34}, 	// MCU_ADDRESS [HG_NR_GAINSTART]
{0x0990, 0x0096}, 	// MCU_DATA_0

// 30fps
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// 60hZ
{0x098C, 0xA215},	// MCU_ADDRESS [AE_INDEX_TH23]
{0x0990, 0x0004},	// MCU_DATA_0

// to reduce green at CWF
{0x098C, 0xA363},	// MCU_ADDRESS [AWB_TG_MIN0]
{0x0990, 0x00C9},	// MCU_DATA_0
// to increase red at day light
{0x098C, 0xA369},	// MCU_ADDRESS [AWB_KR_R]
{0x0990, 0x0080},	// MCU_DATA_0  

// syncronize the FW with the sensor
{0x098C, 0xA103},	// Refresh Sequencer Mode
{0x0990, 0x0006},	//       = 6
{SENSOR_WAIT_MS, 50},

{SENSOR_WAIT_MS, 50},	// delay=50
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD] Refresh Sequencer Mode
{0x0990, 0x0006},	// MCU_DATA_0
{SENSOR_WAIT_MS, 100},	// delay=100
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD] Refresh Sequencer
{0x0990, 0x0005},	// MCU_DATA_0
{SENSOR_WAIT_MS, 100},	// delay=100

{0x001e, 0x0505},
{0x3400, 0x7A20},	// set to discontinous clock

{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg mode_640x480[] = {
{0x001A, 0x0001},
{SENSOR_WAIT_MS, 10}, 	
{0x001A, 0x0000},
{SENSOR_WAIT_MS, 50}, 	//delay=50

// select output interface
{0x001A, 0x0050},       // BITFIELD=0x001A, 0x0200, 0 // disable Parallel
{0x001A, 0x0058},       // BITFIELD=0x001A, 0x0008, 1 // MIPI

// program the on-chip PLL
{0x0014, 0x21F9},	// PLL Control: BYPASS PLL = 8697

// mipi timing for YUV422 (clk_txfifo_wr = 85/42.5Mhz; clk_txfifo_rd = 63.75Mhz)
{0x0010, 0x0115},       // PLL Dividers = 277
{0x0012, 0x00F5},       // wcd = 8
{0x0014, 0x2545},	//PLL Control: TEST_BYPASS on = 9541
{0x0014, 0x2547},	//PLL Control: PLL_ENABLE on = 9543
{0x0014, 0x2447},	//PLL Control: SEL_LOCK_DET on
{SENSOR_WAIT_MS, 1},	//DELAY = 1               //Delay 1ms to allow PLL to loc
{0x0014, 0x2047},	//PLL Control: PLL_BYPASS off
{0x0014, 0x2046},	//PLL Control: TEST_BYPASS off

// enable powerup stop
{0x0018, 0x402D},       //LOAD=MCU Powerup Stop Enable
{SENSOR_WAIT_MS, 10},	//delay=10
{0x0018, 0x402c},       // LOAD=GO
{SENSOR_WAIT_MS, 10}, 	//delay=10

// sensor core & flicker timings
{0x098C, 0x2703},       //Output Width (A)
{0x0990, 0x0280},       //      = 640
{0x098C, 0x2705},       //Output Height (A)
{0x0990, 0x01E0},       //      = 480
{0x098C, 0x2707},       //Output Width (B)
{0x0990, 0x0640},       //      = 1600
{0x098C, 0x2709},       //Output Height (B)
{0x0990, 0x04B0},       //      = 1200
{0x098C, 0x270D},       //Row Start (A)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x270F},       //Column Start (A)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x2711},       //Row End (A)
{0x0990, 0x04BD},       //      = 1213
{0x098C, 0x2713},       //Column End (A)
{0x0990, 0x064D},       //      = 1613
{0x098C, 0x2715},       //Row Speed (A)
{0x0990, 0x0111},       //      = 273
{0x098C, 0x2717},       //Read Mode (A)
{0x0990, 0x046C},       //      = 1132
{0x098C, 0x2719},       //sensor_fine_correction (A)
{0x0990, 0x005A},       //      = 90
{0x098C, 0x271B},       //sensor_fine_IT_min (A)
{0x0990, 0x01BE},       //      = 446
{0x098C, 0x271D},       //sensor_fine_IT_max_margin (A)
{0x0990, 0x0131},       //      = 305
{0x098C, 0x271F},       //Frame Lines (A)
{0x0990, 0x02B3},       //      = 691
{0x098C, 0x2721},       //Line Length (A)
{0x0990, 0x08EC},       //      = 2284
{0x098C, 0x2723},       //Row Start (B)
{0x0990, 0x0004},       //      = 4
{0x098C, 0x2725},       //Column Start (B)
{0x0990, 0x0004},       //      = 4
{0x098C, 0x2727},       //Row End (B)
{0x0990, 0x04BB},       //      = 1211
{0x098C, 0x2729},       //Column End (B)
{0x0990, 0x064B},       //      = 1611
{0x098C, 0x272B},       //Row Speed (B)
{0x0990, 0x0111},       //      = 273
{0x098C, 0x272D},       //Read Mode (B)
{0x0990, 0x0024},       //      = 36
{0x098C, 0x272F},       //sensor_fine_correction (B)
{0x0990, 0x003A},       //      = 58
{0x098C, 0x2731},       //sensor_fine_IT_min (B)
{0x0990, 0x00F6},       //      = 246
{0x098C, 0x2733},       //sensor_fine_IT_max_margin (B)
{0x0990, 0x008B},       //      = 139
{0x098C, 0x2735},       //Frame Lines (B)
{0x0990, 0x0521},       //      = 1313
{0x098C, 0x2737},       //Line Length (B)
{0x0990, 0x08EC},       //      = 2284
{0x098C, 0x2739},       //Crop_X0 (A)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x273B},       //Crop_X1 (A)
{0x0990, 0x031F},       //      = 799
{0x098C, 0x273D},       //Crop_Y0 (A)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x273F},       //Crop_Y1 (A)
{0x0990, 0x0257},       //      = 599
{0x098C, 0x2747},       //Crop_X0 (B)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x2749},       //Crop_X1 (B)
{0x0990, 0x063F},       //      = 1599
{0x098C, 0x274B},       //Crop_Y0 (B)
{0x0990, 0x0000},       //      = 0
{0x098C, 0x274D},       //Crop_Y1 (B)
{0x0990, 0x04AF},       //      = 1199
{0x098C, 0x222D},       //R9 Step
{0x0990, 0x0099},       //      = 153
{0x098C, 0xA408},       //search_f1_50
{0x0990, 0x0024},       //      = 36
{0x098C, 0xA409},       //search_f2_50
{0x0990, 0x0027},       //      = 39
{0x098C, 0xA40A},       //search_f1_60
{0x0990, 0x002D},       //      = 45
{0x098C, 0xA40B},       //search_f2_60
{0x0990, 0x002F},       //      = 47
{0x098C, 0x2411},       //R9_Step_60 (A)
{0x0990, 0x0099},       //      = 153
{0x098C, 0x2413},       //R9_Step_50 (A)
{0x0990, 0x00B8},       //      = 184
{0x098C, 0x2415},       //R9_Step_60 (B)
{0x0990, 0x0099},       //      = 153
{0x098C, 0x2417},       //R9_Step_50 (B)
{0x0990, 0x00B8},       //      = 184

{0x098C, 0xA404},	//FD Mode
{0x0990, 0x0010},	//      = 16
{0x098C, 0xA40D},	//Stat_min
{0x0990, 0x0002},	//      = 2
{0x098C, 0xA40E},	//Stat_max
{0x0990, 0x0003},	//      = 3
{0x098C, 0xA410},	//Min_amplitude
{0x0990, 0x000A},	//      = 10

// AE setting
{0x098C, 0xA117}, 	// MCU_ADDRESS [SEQ_PREVIEW_0_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA11D}, 	// MCU_ADDRESS [SEQ_PREVIEW_1_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA129}, 	// MCU_ADDRESS [SEQ_PREVIEW_3_AE]
{0x0990, 0x0002}, 	// MCU_DATA_0
{0x098C, 0xA24F}, 	// MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x0032}, 	// MCU_DATA_0
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0xA216},       // MCU_ADDRESS [AE_MAXGAIN23]
{0x0990, 0x0080},       // MCU_DATA_0
{0x098C, 0xA20E}, 	// MCU_ADDRESS [AE_MAX_VIRTGAIN]
{0x0990, 0x0080}, 	// MCU_DATA_0
{0x098C, 0x2212}, 	// MCU_ADDRESS [AE_MAX_DGAIN_AE1]
{0x0990, 0x00A4}, 	// MCU_DATA_0

// Lens register settings for A-2031SOC (MT9D115) REV1
{0x364E, 0x0650}, 	// P_GR_P0Q0
{0x3650, 0x3209}, 	// P_GR_P0Q1
{0x3652, 0x5612}, 	// P_GR_P0Q2
{0x3654, 0xB86F}, 	// P_GR_P0Q3
{0x3656, 0xA834}, 	// P_GR_P0Q4
{0x3658, 0x00F0}, 	// P_RD_P0Q0
{0x365A, 0x74AB}, 	// P_RD_P0Q1
{0x365C, 0x4E72}, 	// P_RD_P0Q2
{0x365E, 0x948F}, 	// P_RD_P0Q3
{0x3660, 0xE4B3}, 	// P_RD_P0Q4
{0x3662, 0x01D0}, 	// P_BL_P0Q0
{0x3664, 0x7F6A}, 	// P_BL_P0Q1
{0x3666, 0x1EF2}, 	// P_BL_P0Q2
{0x3668, 0x8FEF}, 	// P_BL_P0Q3
{0x366A, 0xB7B3}, 	// P_BL_P0Q4
{0x366C, 0x0130}, 	// P_GB_P0Q0
{0x366E, 0x1B6B}, 	// P_GB_P0Q1
{0x3670, 0x5FD2}, 	// P_GB_P0Q2
{0x3672, 0xCFEF}, 	// P_GB_P0Q3
{0x3674, 0xB734}, 	// P_GB_P0Q4
{0x3676, 0x256D}, 	// P_GR_P1Q0
{0x3678, 0xB3EF}, 	// P_GR_P1Q1
{0x367A, 0xB5B0}, 	// P_GR_P1Q2
{0x367C, 0x61F2}, 	// P_GR_P1Q3
{0x367E, 0x5193}, 	// P_GR_P1Q4
{0x3680, 0x070E}, 	// P_RD_P1Q0
{0x3682, 0x8F6E}, 	// P_RD_P1Q1
{0x3684, 0x4B8F}, 	// P_RD_P1Q2
{0x3686, 0x16D2}, 	// P_RD_P1Q3
{0x3688, 0xC6F2}, 	// P_RD_P1Q4
{0x368A, 0xA28C}, 	// P_BL_P1Q0
{0x368C, 0xE50D}, 	// P_BL_P1Q1
{0x368E, 0xDE50}, 	// P_BL_P1Q2
{0x3690, 0x5551}, 	// P_BL_P1Q3
{0x3692, 0x7373}, 	// P_BL_P1Q4
{0x3694, 0xEC2B}, 	// P_GB_P1Q0
{0x3696, 0x4C8B}, 	// P_GB_P1Q1
{0x3698, 0xF430}, 	// P_GB_P1Q2
{0x369A, 0x3A92}, 	// P_GB_P1Q3
{0x369C, 0x0054}, 	// P_GB_P1Q4
{0x369E, 0x2E53}, 	// P_GR_P2Q0
{0x36A0, 0x8291}, 	// P_GR_P2Q1
{0x36A2, 0xC936}, 	// P_GR_P2Q2
{0x36A4, 0x1154}, 	// P_GR_P2Q3
{0x36A6, 0x0A79}, 	// P_GR_P2Q4
{0x36A8, 0x14B3}, 	// P_RD_P2Q0
{0x36AA, 0xDEF0}, 	// P_RD_P2Q1
{0x36AC, 0xB2F5}, 	// P_RD_P2Q2
{0x36AE, 0x7D72}, 	// P_RD_P2Q3
{0x36B0, 0x4257}, 	// P_RD_P2Q4
{0x36B2, 0x0A73}, 	// P_BL_P2Q0
{0x36B4, 0x8071}, 	// P_BL_P2Q1
{0x36B6, 0x8EB6}, 	// P_BL_P2Q2
{0x36B8, 0x7BB3}, 	// P_BL_P2Q3
{0x36BA, 0x55B8}, 	// P_BL_P2Q4
{0x36BC, 0x29F3}, 	// P_GB_P2Q0
{0x36BE, 0xF1F0}, 	// P_GB_P2Q1
{0x36C0, 0xD036}, 	// P_GB_P2Q2
{0x36C2, 0x08B4}, 	// P_GB_P2Q3
{0x36C4, 0x0F99}, 	// P_GB_P2Q4
{0x36C6, 0x8B11}, 	// P_GR_P3Q0
{0x36C8, 0x3EF2}, 	// P_GR_P3Q1
{0x36CA, 0x5FD4}, 	// P_GR_P3Q2
{0x36CC, 0x8C76}, 	// P_GR_P3Q3
{0x36CE, 0xDCF6}, 	// P_GR_P3Q4
{0x36D0, 0x9410}, 	// P_RD_P3Q0
{0x36D2, 0x5972}, 	// P_RD_P3Q1
{0x36D4, 0x9853}, 	// P_RD_P3Q2
{0x36D6, 0xA036}, 	// P_RD_P3Q3
{0x36D8, 0x59D6}, 	// P_RD_P3Q4
{0x36DA, 0x86B1}, 	// P_BL_P3Q0
{0x36DC, 0x70D1}, 	// P_BL_P3Q1
{0x36DE, 0x2835}, 	// P_BL_P3Q2
{0x36E0, 0xCC55}, 	// P_BL_P3Q3
{0x36E2, 0xE697}, 	// P_BL_P3Q4
{0x36E4, 0x9B10}, 	// P_GB_P3Q0
{0x36E6, 0x2F12}, 	// P_GB_P3Q1
{0x36E8, 0x7B34}, 	// P_GB_P3Q2
{0x36EA, 0xA976}, 	// P_GB_P3Q3
{0x36EC, 0xAD17}, 	// P_GB_P3Q4
{0x36EE, 0xE635}, 	// P_GR_P4Q0
{0x36F0, 0x0313}, 	// P_GR_P4Q1
{0x36F2, 0x0F59}, 	// P_GR_P4Q2
{0x36F4, 0x7755}, 	// P_GR_P4Q3
{0x36F6, 0xD23A}, 	// P_GR_P4Q4
{0x36F8, 0xF5F4}, 	// P_RD_P4Q0
{0x36FA, 0x0973}, 	// P_RD_P4Q1
{0x36FC, 0x4154}, 	// P_RD_P4Q2
{0x36FE, 0x10F6}, 	// P_RD_P4Q3
{0x3700, 0x18BA}, 	// P_RD_P4Q4
{0x3702, 0x9F35}, 	// P_BL_P4Q0
{0x3704, 0x6773}, 	// P_BL_P4Q1
{0x3706, 0x6DD8}, 	// P_BL_P4Q2
{0x3708, 0xFB53}, 	// P_BL_P4Q3
{0x370A, 0xDCBA}, 	// P_BL_P4Q4
{0x370C, 0xE0F5}, 	// P_GB_P4Q0
{0x370E, 0x3352}, 	// P_GB_P4Q1
{0x3710, 0x17B9}, 	// P_GB_P4Q2
{0x3712, 0x00F6}, 	// P_GB_P4Q3
{0x3714, 0xF9DA}, 	// P_GB_P4Q4
{0x3644, 0x0344}, 	// POLY_ORIGIN_C
{0x3642, 0x0270}, 	// POLY_ORIGIN_R
{0x3210, 0x01B8}, 	// COLOR_PIPELINE_CONTROL

// GAMMA Setting
{0x098C, 0xAB37}, 	// MCU_ADDRESS [HG_GAMMA_MORPH_CTRL]
{0x0990, 0x0003}, 	// MCU_DATA_0
{0x098C, 0x2B38}, 	// MCU_ADDRESS [HG_GAMMASTARTMORPH]
{0x0990, 0x2968}, 	// MCU_DATA_0
{0x098C, 0x2B3A}, 	// MCU_ADDRESS [HG_GAMMASTOPMORPH]
{0x0990, 0x2D50}, 	// MCU_DATA_0
{0x098C, 0x2B62}, 	// MCU_ADDRESS [HG_FTB_START_BM]
{0x0990, 0xFFFE}, 	// MCU_DATA_0
{0x098C, 0x2B64}, 	// MCU_ADDRESS [HG_FTB_STOP_BM]
{0x0990, 0xFFFF}, 	// MCU_DATA_0
{0x098C, 0xAB4F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_0]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0xAB50}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_1]
{0x0990, 0x0013}, 	// MCU_DATA_0
{0x098C, 0xAB51}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_2]
{0x0990, 0x0027}, 	// MCU_DATA_0
{0x098C, 0xAB52}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_3]
{0x0990, 0x0043}, 	// MCU_DATA_0
{0x098C, 0xAB53}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_4]
{0x0990, 0x0068}, 	// MCU_DATA_0
{0x098C, 0xAB54}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_5]
{0x0990, 0x0081}, 	// MCU_DATA_0
{0x098C, 0xAB55}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_6]
{0x0990, 0x0093}, 	// MCU_DATA_0
{0x098C, 0xAB56}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_7]
{0x0990, 0x00A3}, 	// MCU_DATA_0
{0x098C, 0xAB57}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_8]
{0x0990, 0x00B0}, 	// MCU_DATA_0
{0x098C, 0xAB58}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_9]
{0x0990, 0x00BC}, 	// MCU_DATA_0
{0x098C, 0xAB59}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_10]
{0x0990, 0x00C7}, 	// MCU_DATA_0
{0x098C, 0xAB5A}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_11]
{0x0990, 0x00D1}, 	// MCU_DATA_0
{0x098C, 0xAB5B}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_12]
{0x0990, 0x00DA}, 	// MCU_DATA_0
{0x098C, 0xAB5C}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_13]
{0x0990, 0x00E2}, 	// MCU_DATA_0
{0x098C, 0xAB5D}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_14]
{0x0990, 0x00E9}, 	// MCU_DATA_0
{0x098C, 0xAB5E}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_15]
{0x0990, 0x00EF}, 	// MCU_DATA_0
{0x098C, 0xAB5F}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_16]
{0x0990, 0x00F4}, 	// MCU_DATA_0
{0x098C, 0xAB60}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_17]
{0x0990, 0x00FA}, 	// MCU_DATA_0
{0x098C, 0xAB61}, 	// MCU_ADDRESS [HG_GAMMA_TABLE_B_18]
{0x0990, 0x00FF}, 	// MCU_DATA_0

// CCM
{0x098C, 0x2306}, 	// MCU_ADDRESS [AWB_CCM_L_0]
{0x0990, 0x01D6}, 	// MCU_DATA_0
{0x098C, 0x2308}, 	// MCU_ADDRESS [AWB_CCM_L_1]
{0x0990, 0xFF89}, 	// MCU_DATA_0
{0x098C, 0x230A}, 	// MCU_ADDRESS [AWB_CCM_L_2]
{0x0990, 0xFFA1}, 	// MCU_DATA_0
{0x098C, 0x230C}, 	// MCU_ADDRESS [AWB_CCM_L_3]
{0x0990, 0xFF73}, 	// MCU_DATA_0
{0x098C, 0x230E}, 	// MCU_ADDRESS [AWB_CCM_L_4]
{0x0990, 0x019C}, 	// MCU_DATA_0
{0x098C, 0x2310}, 	// MCU_ADDRESS [AWB_CCM_L_5]
{0x0990, 0xFFF1}, 	// MCU_DATA_0
{0x098C, 0x2312}, 	// MCU_ADDRESS [AWB_CCM_L_6]
{0x0990, 0xFFB0}, 	// MCU_DATA_0
{0x098C, 0x2314}, 	// MCU_ADDRESS [AWB_CCM_L_7]
{0x0990, 0xFF2D}, 	// MCU_DATA_0
{0x098C, 0x2316}, 	// MCU_ADDRESS [AWB_CCM_L_8]
{0x0990, 0x0223}, 	// MCU_DATA_0
{0x098C, 0x2318}, 	// MCU_ADDRESS [AWB_CCM_L_9]
{0x0990, 0x001C}, 	// MCU_DATA_0
{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
{0x0990, 0x0054}, 	// MCU_DATA_0
{0x098C, 0x231C}, 	// MCU_ADDRESS [AWB_CCM_RL_0]
{0x0990, 0xFFCD}, 	// MCU_DATA_0
{0x098C, 0x231E}, 	// MCU_ADDRESS [AWB_CCM_RL_1]
{0x0990, 0x0023}, 	// MCU_DATA_0
{0x098C, 0x2320}, 	// MCU_ADDRESS [AWB_CCM_RL_2]
{0x0990, 0x0010}, 	// MCU_DATA_0
{0x098C, 0x2322}, 	// MCU_ADDRESS [AWB_CCM_RL_3]
{0x0990, 0x0026}, 	// MCU_DATA_0
{0x098C, 0x2324}, 	// MCU_ADDRESS [AWB_CCM_RL_4]
{0x0990, 0xFFE9}, 	// MCU_DATA_0
{0x098C, 0x2326}, 	// MCU_ADDRESS [AWB_CCM_RL_5]
{0x0990, 0xFFF1}, 	// MCU_DATA_0
{0x098C, 0x2328}, 	// MCU_ADDRESS [AWB_CCM_RL_6]
{0x0990, 0x003A}, 	// MCU_DATA_0
{0x098C, 0x232A}, 	// MCU_ADDRESS [AWB_CCM_RL_7]
{0x0990, 0x005D}, 	// MCU_DATA_0
{0x098C, 0x232C}, 	// MCU_ADDRESS [AWB_CCM_RL_8]
{0x0990, 0xFF69}, 	// MCU_DATA_0
{0x098C, 0x232E}, 	// MCU_ADDRESS [AWB_CCM_RL_9]
{0x0990, 0x000C}, 	// MCU_DATA_0
{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
{0x0990, 0xFFD8}, 	// MCU_DATA_0

{0x0018, 0x0028},
{SENSOR_WAIT_MS, 100},  //  POLL  MON_PATCH_ID_0 =>  0x01

{0x098C, 0xAB20},   	// MCU_ADDRESS [HG_LL_SAT1]
{0x0990, 0x0040}, 	// MCU_DATA_0
{0x098C, 0xAB24}, 	// MCU_ADDRESS [HG_LL_SAT2]
{0x0990, 0x0000}, 	// MCU_DATA_0
{0x098C, 0x231A}, 	// MCU_ADDRESS [AWB_CCM_L_10]
{0x0990, 0x0030}, 	// MCU_DATA_0
{0x098C, 0x2330}, 	// MCU_ADDRESS [AWB_CCM_RL_10]
{0x0990, 0xFFFC}, 	// MCU_DATA_0
{0x098C, 0xAB2C}, 	// MCU_ADDRESS [HG_NR_START_R]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2D}, 	// MCU_ADDRESS [HG_NR_START_G]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2E}, 	// MCU_ADDRESS [HG_NR_START_B]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB2F}, 	// MCU_ADDRESS [HG_NR_START_OL]
{0x0990, 0x000A}, 	// MCU_DATA_0
{0x098C, 0xAB34}, 	// MCU_ADDRESS [HG_NR_GAINSTART]
{0x0990, 0x0096}, 	// MCU_DATA_0

// 30fps
{0x098C, 0xA20C}, 	// MCU_ADDRESS [AE_MAX_INDEX]
{0x0990, 0x0010}, 	// 60hZ
{0x098C, 0xA215},	// MCU_ADDRESS [AE_INDEX_TH23]
{0x0990, 0x0004},	// MCU_DATA_0

// to reduce green at CWF
{0x098C, 0xA363},	// MCU_ADDRESS [AWB_TG_MIN0]
{0x0990, 0x00C9},	// MCU_DATA_0
// to increase red at day light
{0x098C, 0xA369},	// MCU_ADDRESS [AWB_KR_R]
{0x0990, 0x0080},	// MCU_DATA_0  
{SENSOR_WAIT_MS, 50},

// syncronize the FW with the sensor
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006},	// MCU_DATA_0
{SENSOR_WAIT_MS, 100},
{0x098C, 0xA103},	// MCU_ADDRES
{0x0990, 0x0005},	// MCU_DATA_0
{SENSOR_WAIT_MS, 100},
{0x001e, 0x0505},
{0x3400, 0x7A20},	// set to discontinous clock
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_None[] = {
{0x098C, 0x2759},        //content A
{0x0990, 0x0000},
{0x098C, 0x275B},        //content B
{0x0990, 0x0000},
{0x098C, 0xA103},        //MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006},        //MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Mono[] = {
{0x098C, 0x2759},        //content A
{0x0990, 0x0001},
{0x098C, 0x275B},        //content B
{0x0990, 0x0001},        
{0x098C, 0xA103}, 	 //MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006}, 	 //MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Sepia[] = {
{0x098C, 0x2759},        //content A
{0x0990, 0x0002},
{0x098C, 0x275B},        //content B
{0x0990, 0x0002},
{0x098C, 0xA103},        //MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006},        //MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Negative[] = {
{0x098C, 0x2759},        //content A
{0x0990, 0x0003},
{0x098C, 0x275B},        //content B
{0x0990, 0x0003},
{0x098C, 0xA103},        //MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006},        //MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Solarize[] = {
{0x098C, 0x2759},        //content A
{0x0990, 0x0004},
{0x098C, 0x275B},        //content B
{0x0990, 0x0004},
{0x098C, 0xA103},        //MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0006},        //MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

//Sensor ISP Not Support this function
static struct sensor_reg ColorEffect_Posterize[] = {
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Auto[] = {
{0x098C, 0xA115},	// MCU_ADDRESS [SEQ_CAP_MODE]
{0x0990, 0x0000},	// MCU_DATA_0
{0x098C, 0xA11F},	// MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0001},	// MCU_DATA_0
{0x098C, 0xA103},	// MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005},	// MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Incandescent[] = {
{0x098C, 0xA115 },      // MCU_ADDRESS [SEQ_CAP_MODE]
{0x0990, 0x0000 },      // MCU_DATA_0
{0x098C, 0xA11F },      // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0000 },      // MCU_DATA_0
{0x098C, 0xA103 },      // MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005 },      // MCU_DATA_0
{SENSOR_WAIT_MS, 100},
{0x098C, 0xA353 },      // MCU_ADDRESS [AWB_CCM_POSITION]
{0x0990, 0x002B },      // MCU_DATA_0
{0x098C, 0xA34E },      // MCU_ADDRESS [AWB_GAIN_R]
{0x0990, 0x007B },      // MCU_DATA_0
{0x098C, 0xA34F },      // MCU_ADDRESS [AWB_GAIN_G]
{0x0990, 0x0080 },      // MCU_DATA_0
{0x098C, 0xA350 },      // MCU_ADDRESS [AWB_GAIN_B]
{0x0990, 0x007E },      // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Daylight[] = {
{0x098C, 0xA115 },       // MCU_ADDRESS [SEQ_CAP_MODE]
{0x0990, 0x0000 },       // MCU_DATA_0
{0x098C, 0xA11F },       // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0000 },       // MCU_DATA_0
{0x098C, 0xA103 },       // MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005 },       // MCU_DATA_0
{SENSOR_WAIT_MS, 100},
{0x098C, 0xA353 },       // MCU_ADDRESS [AWB_CCM_POSITION]
{0x0990, 0x007F },       // MCU_DATA_0
{0x098C, 0xA34E },       // MCU_ADDRESS [AWB_GAIN_R]
{0x0990, 0x008E },       // MCU_DATA_0
{0x098C, 0xA34F },       // MCU_ADDRESS [AWB_GAIN_G]
{0x0990, 0x0080 },       // MCU_DATA_0
{0x098C, 0xA350 },       // MCU_ADDRESS [AWB_GAIN_B]
{0x0990, 0x0074 },       // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Fluorescent[] = {
{0x098C, 0xA115  },     // MCU_ADDRESS [SEQ_CAP_MODE]
{0x0990, 0x0000  },     // MCU_DATA_0
{0x098C, 0xA11F  },     // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0000  },     // MCU_DATA_0
{0x098C, 0xA103  },     // MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005  },     // MCU_DATA_0
{SENSOR_WAIT_MS, 100},
{0x098C, 0xA353  },     // MCU_ADDRESS [AWB_CCM_POSITION]
{0x0990, 0x0036  },     // MCU_DATA_0
{0x098C, 0xA34E  },     // MCU_ADDRESS [AWB_GAIN_R]
{0x0990, 0x0099  },     // MCU_DATA_0
{0x098C, 0xA34F  },     // MCU_ADDRESS [AWB_GAIN_G]
{0x0990, 0x0080  },     // MCU_DATA_0
{0x098C, 0xA350  },     // MCU_ADDRESS [AWB_GAIN_B]
{0x0990, 0x007D  },     // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};
//craig / 11.05.11 / for fix 2M camera WB_Cloudy
static struct sensor_reg Whitebalance_CloudyDaylight[] = {
{0x098C, 0xA115  },     // MCU_ADDRESS [SEQ_CAP_MODE]
{0x0990, 0x0000  },     // MCU_DATA_0
{0x098C, 0xA11F  },     // MCU_ADDRESS [SEQ_PREVIEW_1_AWB]
{0x0990, 0x0000  },     // MCU_DATA_0
{0x098C, 0xA103  },     // MCU_ADDRESS [SEQ_CMD]
{0x0990, 0x0005  },     // MCU_DATA_0
{SENSOR_WAIT_MS, 100},
{0x098C, 0xA353  },     // MCU_ADDRESS [AWB_CCM_POSITION]
{0x0990, 0x0046  },     // MCU_DATA_0
{0x098C, 0xA34E  },     // MCU_ADDRESS [AWB_GAIN_R]
{0x0990, 0x007D  },     // MCU_DATA_0
{0x098C, 0xA34F  },     // MCU_ADDRESS [AWB_GAIN_G]
{0x0990, 0x0080  },     // MCU_DATA_0
{0x098C, 0xA350  },     // MCU_ADDRESS [AWB_GAIN_B]
{0x0990, 0x007E  },     // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};
//craig / 11.05.11 / for fix 2M camera WB_Cloudy


enum {
	SENSOR_MODE_1600x1200,
	SENSOR_MODE_1280x720,
	SENSOR_MODE_640x480,
};

static struct sensor_reg *mode_table[] = {
	[SENSOR_MODE_1600x1200] = mode_1600x1200,
	[SENSOR_MODE_1280x720]  = mode_1280x720,
	[SENSOR_MODE_640x480]   = mode_640x480,
};

//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings START
static struct sensor_reg exp_negative2[] = {
//[5.1 -2EV -- mean=100]	
{0x098C, 0xA24F  },   // MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x001D  },   // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};
static struct sensor_reg exp_negative1[] = {
//[5.2 -1EV -- mean=120]	
{0x098C, 0xA24F  },   // MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x0028  },   // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};
static struct sensor_reg exp_zero[] = {
//[5.3 0EV -- mean=140]	//
{0x098C, 0xA24F  },   // MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x0032  },   // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};
static struct sensor_reg exp_one[] = {
//[5.4 +1EV -- mean=160]	//
{0x098C, 0xA24F  },   // MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x003C  },   // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};
static struct sensor_reg exp_two[] = {
//[5.5 +2EV -- mean=180]	//
{0x098C, 0xA24F  },   // MCU_ADDRESS [AE_BASETARGET]
{0x0990, 0x0046  },   // MCU_DATA_0
{SENSOR_TABLE_END, 0x0000}
};

enum {
      YUV_Exposure_Negative_2 = 0,
      YUV_Exposure_Negative_1,
      YUV_Exposure_0,
      YUV_Exposure_1,
      YUV_Exposure_2
};
//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings END

static int sensor2031_read_reg(struct i2c_client *client, u16 addr, u16 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[4];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (addr >> 8);;
	data[1] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;

        swap(*(data+2),*(data+3)); //swap high and low byte to match table format
	memcpy(val, data+2, 2);

	return 0;
}

static int sensor2031_write_reg(struct i2c_client *client, u16 addr, u16 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[4];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
        data[2] = (u8) (val >> 8);
	data[3] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 4;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("yuv2031_sensor : i2c transfer failed, retrying %x %x\n",
		       addr, val);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}

static int sensor2031_write_table(struct i2c_client *client,
			      const struct sensor_reg table[])
{
	int err;
	const struct sensor_reg *next;
	u16 val;

	pr_info("yuv %s\n",__func__);
	for (next = table; next->addr != SENSOR_TABLE_END; next++) {
		if (next->addr == SENSOR_WAIT_MS) {
			msleep(next->val);
			continue;
		}

		val = next->val;

		err = sensor2031_write_reg(client, next->addr, val);
		if (err){
                    pr_err("yuv2031_sensor : write addr=%x,val=%x error ,stop write reg  \n",next->addr, val);
			return err;
			}
	}
	return 0;
}

static int get_sensor2031_current_width(struct i2c_client *client, u16 *val)
{
        int err;
         
        err = sensor2031_write_reg(client, 0x098c, 0x2703);
        if (err)
          return err;

        err = sensor2031_read_reg(client, 0x0990, val);

        if (err)
          return err;

        return 0;
}

static int sensor2031_set_mode(struct sensor_info *info, struct sensor_mode *mode)
{
	int sensor_table;
	int err;
        u16 val;

	pr_info("%s: xres %u yres %u\n",__func__, mode->xres, mode->yres);

	if (mode->xres == 1600 && mode->yres == 1200)
		sensor_table = SENSOR_MODE_1600x1200;
	else if (mode->xres == 1280 && mode->yres == 720)
		sensor_table = SENSOR_MODE_1280x720;
	else if (mode->xres == 640 && mode->yres == 480)
		sensor_table = SENSOR_MODE_640x480;
	else {
		pr_err("%s: invalid resolution supplied to set mode %d %d\n",
		       __func__, mode->xres, mode->yres);
		return -EINVAL;
	}

        err = get_sensor2031_current_width(info->i2c_client, &val);

        //if no table is programming before and request set to 1600x1200, then
        //we must use 1600x1200 table to fix CTS testing issue
        if(!(val == SENSOR_640_WIDTH_VAL || val == SENSOR_720_WIDTH_VAL || val == SENSOR_1600_WIDTH_VAL) && sensor_table == SENSOR_MODE_1600x1200)
        {
	   err = sensor2031_write_table(info->i2c_client, CTS_ZoomTest_mode_1600x1200);
        }
        else {
              //check already program the sensor mode, Aptina support Context B fast switching capture mode back to preview mode
              //we don't need to re-program the sensor mode for 640x480 table
              if(!(val == SENSOR_640_WIDTH_VAL && sensor_table == SENSOR_MODE_640x480))
              {
	             err = sensor2031_write_table(info->i2c_client, mode_table[sensor_table]);
	             if (err)
		       return err;
                     //polling sensor to confirm it's already in capture flow. 
                     //this can avoid frame mismatch issue due to inproper delay
                     if(sensor_table == SENSOR_MODE_1600x1200)
                     {
                        val = 0;
                        do {
                           err = sensor2031_write_reg(info->i2c_client, 0x098c, 0xa104);  //MCU_ADDRESS[SEQ_STATE]
	                   if (err)
		              return err;
                           err = sensor2031_read_reg(info->i2c_client, 0x0990, &val);     //MCU_DATA_0 value
	                   if (err)
		              return err;
                        }
                        while (val != 7);                   
                     }
              }
        }
	info->mode = sensor_table;
	return 0;
}

static long sensor2031_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct sensor_info *info = file->private_data;
        int err=0;

	pr_info("yuv %s\n",__func__);
	switch (cmd) {
	case SENSOR_IOCTL_SET_MODE:
	{
		struct sensor_mode mode;
		if (copy_from_user(&mode,
				   (const void __user *)arg,
				   sizeof(struct sensor_mode))) {
			return -EFAULT;
		}

		return sensor2031_set_mode(info, &mode);
	}
	case SENSOR_IOCTL_GET_STATUS:
	{

		return 0;
	}
        case SENSOR_IOCTL_SET_COLOR_EFFECT:
        {
            u8 coloreffect;
            if (copy_from_user(&coloreffect,
   	       (const void __user *)arg,
	        sizeof(coloreffect))) {
		return -EFAULT;
	    }
            switch(coloreffect)
            {
                case YUV_ColorEffect_None:
                     pr_info("yuv2031 SET_Coloreffect_None\n");
	             err = sensor2031_write_table(info->i2c_client, ColorEffect_None);
                     break;
                case YUV_ColorEffect_Mono:
                     pr_info("yuv2031 SET_Coloreffect_Mono\n");
	             err = sensor2031_write_table(info->i2c_client, ColorEffect_Mono);
                     break;
                case YUV_ColorEffect_Sepia:
                     pr_info("yuv2031 SET_Coloreffect_Sepia\n");
	             err = sensor2031_write_table(info->i2c_client, ColorEffect_Sepia);
                     break;
                case YUV_ColorEffect_Negative:
                     pr_info("yuv2031 SET_Coloreffect_Negative\n");
	             err = sensor2031_write_table(info->i2c_client, ColorEffect_Negative);
                     break;
                case YUV_ColorEffect_Solarize:
                     pr_info("yuv2031 SET_Coloreffect_Solarize\n");
	             err = sensor2031_write_table(info->i2c_client, ColorEffect_Solarize);
                     break;
                case YUV_ColorEffect_Posterize:
                     pr_info("yuv2031 SET_Coloreffect_Posterize\n");
	             err = sensor2031_write_table(info->i2c_client, ColorEffect_Posterize);
                     break;
                default:
                     break;
            }
	    if (err)
	        return err;
            return 0;
        }
        case SENSOR_IOCTL_SET_WHITE_BALANCE:
        {
            u8 whitebalance;
            if (copy_from_user(&whitebalance,
	       (const void __user *)arg,
    	        sizeof(whitebalance))) {
		return -EFAULT;
	    }
            switch(whitebalance)
            {
                case YUV_Whitebalance_Auto:
                     pr_info("yuv2031 SET_Whitebalance_Auto\n");
	             err = sensor2031_write_table(info->i2c_client, Whitebalance_Auto);
                     break;
                case YUV_Whitebalance_Incandescent:
                     pr_info("yuv2031 SET_Whitebalance_Incandescent\n");
	             err = sensor2031_write_table(info->i2c_client, Whitebalance_Incandescent);
                     break;
                case YUV_Whitebalance_Daylight:
                     pr_info("yuv2031 SET_Whitebalance_Daylight\n");
                     err = sensor2031_write_table(info->i2c_client, Whitebalance_Daylight);
                     break;
                case YUV_Whitebalance_Fluorescent:
                     pr_info("yuv2031 SET_Whitebalance_Flourescent\n");
                     err = sensor2031_write_table(info->i2c_client, Whitebalance_Fluorescent);
                     break;
                //craig++
                case YUV_Whitebalance_CloudyDaylight:
                     pr_info("yuv2031 SET_Whitebalance_CloudyDaylight\n");
                     err = sensor2031_write_table(info->i2c_client, Whitebalance_CloudyDaylight);
                     break;
                //craig--
                default:
                     break;
            }
            if (err)
	        return err;
            return 0;
        }
        case SENSOR_IOCTL_SET_SCENE_MODE:
        {
                return 0;
        }
//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings START
        case SENSOR_IOCTL_SET_EXPOSURE:
        {
            u8 exposure;
            if (copy_from_user(&exposure,
               (const void __user *)arg,
                sizeof(exposure))) {
                return -EFAULT; 
            }
            switch(exposure)
            {
                case YUV_Exposure_0:
                     pr_info("yuv2031 SET_EXPOSURE 0\n");
	             err = sensor2031_write_table(info->i2c_client, exp_zero);
                     break;
                case YUV_Exposure_1:
                     pr_info("yuv2031 SET_EXPOSURE 1\n");
	             err = sensor2031_write_table(info->i2c_client, exp_one);
                     break;
                case YUV_Exposure_2:
                     pr_info("yuv2031 SET_EXPOSURE 2\n");
	             err = sensor2031_write_table(info->i2c_client, exp_two);
                     break;
                case YUV_Exposure_Negative_1:
                     pr_info("yuv2031 SET_EXPOSURE -1\n");
	             err = sensor2031_write_table(info->i2c_client, exp_negative1);
                     break;
                case YUV_Exposure_Negative_2:
                     pr_info("yuv2031 SET_EXPOSURE -2\n");
	             err = sensor2031_write_table(info->i2c_client, exp_negative2);
                     break;
                default:
                     break;
            }
            if (err)
               return err;
           return 0;
//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings END
        }
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor2031_open(struct inode *inode, struct file *file)
{
	pr_info("yuv %s\n",__func__);
	file->private_data = info;
	if (info->pdata && info->pdata->power_on)
		info->pdata->power_on();
	return 0;
}

static int sensor2031_release(struct inode *inode, struct file *file)
{
       pr_info("yuv %s\n",__func__);
	if (info->pdata && info->pdata->power_off)
		info->pdata->power_off();
	file->private_data = NULL;
	return 0;
}


static const struct file_operations sensor_fileops = {
	.owner = THIS_MODULE,
	.open = sensor2031_open,
	.unlocked_ioctl = sensor2031_ioctl,
	.release = sensor2031_release,
};

static struct miscdevice sensor_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SENSOR_NAME,
	.fops = &sensor_fileops,
};

static int sensor2031_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err;

	pr_info("yuv %s\n",__func__);

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);

	if (!info) {
		pr_err("yuv_sensor : Unable to allocate memory!\n");
		return -ENOMEM;
	}

	err = misc_register(&sensor_device);
	if (err) {
		pr_err("yuv_sensor : Unable to register misc device!\n");
		kfree(info);
		return err;
	}

	info->pdata = client->dev.platform_data;
	info->i2c_client = client;

	i2c_set_clientdata(client, info);
	return 0;
}

static int sensor2031_remove(struct i2c_client *client)
{
	struct sensor_info *info;

	pr_info("yuv %s\n",__func__);
	info = i2c_get_clientdata(client);
	misc_deregister(&sensor_device);
	kfree(info);
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{ SENSOR_NAME, 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_i2c_driver = {
	.driver = {
		.name = SENSOR_NAME,
		.owner = THIS_MODULE,
	},
	.probe = sensor2031_probe,
	.remove = sensor2031_remove,
	.id_table = sensor_id,
};

static int __init sensor2031_init(void)
{
	pr_info("yuv %s\n",__func__);
	return i2c_add_driver(&sensor_i2c_driver);
}

static void __exit sensor2031_exit(void)
{
	pr_info("yuv %s\n",__func__);
	i2c_del_driver(&sensor_i2c_driver);
}

module_init(sensor2031_init);
module_exit(sensor2031_exit);

