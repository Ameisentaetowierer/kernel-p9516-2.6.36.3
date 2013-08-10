/*
 * kernel/drivers/media/video/tegra
 *
 * Aptina MT9D111 sensor driver
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

/* COMPAL - START - [2011/4/12  11:5] - Jamin - [Camera]: enable camera ap5140 ap2031  */
#include <media/yuv_sensor_ap5140.h>
#include "yuv_ap5140_init_tab.h"
/* COMPAL - END */

#define CAM_CORE_B_OUTPUT_SIZE_WIDTH 0xC8C0
#define CAM_CORE_A_OUTPUT_SIZE_WIDTH 0xC86C
#define SENSOR_640_WIDTH_VAL 0x0518
#define ONE_TIME_TABLE_CHECK_REG 0x8016
#define ONE_TIME_TABLE_CHECK_DATA 0x086C 
//craig / 06.24 / for fix 5M camcorder fps to 30
static int preview_mode = SENSOR_MODE_1280x960;
static int video_mode = 1;
static u16 lowlight_reg = 0xB80C;
static int lowlight_threshold = 0;
static u16 focusmode_reg = 0xB007;
static u16 focusmode_threshold = 0x4000;
static u16 targetexposure_reg = 0xA409;
//craig
 
static int sensor5140_read_reg(struct i2c_client *client, u16 addr, u16 *val)
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

static int sensor5140_write_reg8(struct i2c_client *client, u16 addr, u16 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[3];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
	data[2] = (u8) (val & 0xff);
        
	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("yuv_sensor5140 : i2c transfer failed, retrying %x %x\n",
		       addr, val);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}
static int sensor5140_poll(struct i2c_client *client, u16 addr, u16 value,
			u16 mask)
{
	u16 data;
	int try, err;

	for (try = 0; try < SENSOR_POLL_RETRIES; try++) {
		err = sensor5140_read_reg(client, addr, &data);
		if (err)
			return err;
		pr_info("poll reg %x: %x == %x & %x %d times\n", addr,
			value, data, mask, try);
		if (value == (data & mask)) {
			return 0;
		}
		msleep(SENSOR_POLL_WAITMS);
	}
	pr_err("%s: poll for %d times %x == ([%x]=%x) & %x failed\n",__func__, try, value,
	       addr, data, mask);

	return -EIO;

}
static int sensor5140_poll_bit_set(struct i2c_client *client, u16 addr, u16 bit)
{
	return sensor5140_poll(client, addr, bit, bit);
}

static int sensor5140_write_reg16(struct i2c_client *client, u16 addr, u16 val)
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
		pr_err("yuv_sensor5140 : i2c transfer failed, retrying %x %x\n",
		       addr, val);
		msleep(3);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}
static int sensor5140_write_table(struct i2c_client *client,
			      const struct sensor_reg table[])
{
	const struct sensor_reg *next;
	int err;
        next = table ;
        

	pr_info("yuv %s \n",__func__);
	for (next = table; next->op != SENSOR_TABLE_END; next++) {

		switch (next->op) {
		case WRITE_REG_DATA8:
		{
			err = sensor5140_write_reg8(client, next->addr,
				next->val);
			if (err)
				return err;
			break;
		}
		case WRITE_REG_DATA16:
		{
			err = sensor5140_write_reg16(client, next->addr,
				next->val);
			if (err)
				return err;
			break;
		}
		case SENSOR_WAIT_MS:
		{
			msleep(next->val);
			break;
		}
		case POLL_REG_BIT:
		{
			err = sensor5140_poll_bit_set(client, next->addr,
				next->val);
			if (err)
				return err;
			break;
		}
//craig / 0701 / overexposure
               case CHECK_FLASH:
               {
                       u16 val1,val2,val3;
                       err = sensor5140_read_reg(info->i2c_client, focusmode_reg, &val1);
                       if (err)
                               return err;
                       err = sensor5140_read_reg(info->i2c_client, lowlight_reg, &val2);
                       if (err)
                               return err;  
                       if (info->focus_mode == SENSOR_AF_FULLTRIGER) {
                               if (((info->flash_mode == YUV_FlashAuto) && (lowlight_threshold == 1) && (val1 >= focusmode_threshold)) || ((info->flash_mode == YUV_FlashOn) && (val1 >= focusmode_threshold))) {                                      
                                       err = sensor5140_write_reg8(info->i2c_client, targetexposure_reg, 0x10);
                                       if (err)
                                           return err;                                                                 
                                       err = sensor5140_read_reg(info->i2c_client, targetexposure_reg, &val3);
                                       if (err)
                                           return err;
                               }
                               lowlight_threshold = 0;
                       }                       
                       else if (info->focus_mode == SENSOR_AF_INIFINITY) {
                               if ((info->flash_mode == YUV_FlashAuto || info->flash_mode == YUV_FlashOn) && (lowlight_threshold == 1)) {
                                     err = sensor5140_write_reg8(info->i2c_client, targetexposure_reg, 0x10);
                                     if (err)
return err;                                           
                                     err = sensor5140_read_reg(info->i2c_client, targetexposure_reg, &val3);
                                     if (err)
                                         return err;
                               }
                               lowlight_threshold = 0;
                       }
                       else {                                                          
                               err = sensor5140_write_reg8(info->i2c_client, targetexposure_reg, 0x40);
                               if (err)
                                   return err;                         
                               lowlight_threshold = 0;                         
                       }                       
                       break;
//craig / 0701 / overexposure  
               }
		default:
			pr_err("%s: invalid operation 0x%x\n", __func__,
				next->op);
			return err;
		}
	}
	return 0;
}

static int get_sensor5140_current_width(struct i2c_client *client, u16 *val, u16 reg)
{
        int err;
         
        err = sensor5140_read_reg(client, reg, val);

        if (err)
          return err;

        return 0;
}

static int sensor5140_set_mode(struct sensor_info *info, struct sensor_mode *mode)
{
	int sensor_table;
	int err;
        u16 val;

	pr_info("%s: xres %u yres %u\n",__func__, mode->xres, mode->yres);

	if (mode->xres == 2592 && mode->yres == 1944){
		sensor_table = SENSOR_MODE_2592x1944;
		video_mode = 1;}
//craig / 05.19 / fix camera preview size
	else if (mode->xres == 1280 && mode->yres == 960){
		sensor_table = SENSOR_MODE_1280x960;
		video_mode = 1;}
//craig	
	else if (mode->xres == 1280 && mode->yres == 720){
		sensor_table = SENSOR_MODE_1280x720;
		video_mode = 0;}
	else if (mode->xres == 640 && mode->yres == 480){
		sensor_table = SENSOR_MODE_640x480;
		video_mode = 0;}
	else {
		pr_err("%s: invalid resolution supplied to set mode %d %d\n",
		       __func__, mode->xres, mode->yres);
		return -EINVAL;
	}

        err = get_sensor5140_current_width(info->i2c_client, &val,CAM_CORE_A_OUTPUT_SIZE_WIDTH);

        pr_err("First time initial check value %x\n",val);
        //check already program the sensor mode, Aptina support Context B fast switching capture mode back to preview mode
        //we don't need to re-program the sensor mode for 640x480 table
        if ((val != SENSOR_640_WIDTH_VAL) && (sensor_table == SENSOR_MODE_2592x1944))
        {
	    pr_err("initial cts tab %d\n",sensor_table);
	    err = sensor5140_write_table(info->i2c_client, mode_2592x1944_CTS);
	    if (err)
		return err;
        }
//craig / 05.26 / fix AF and solve green screen
        if (sensor_table == SENSOR_MODE_2592x1944) {
		pr_info("yuv5 %s: sensor_table == SENSOR_MODE_2592x1944\n", __func__);
		err = sensor5140_write_table(info->i2c_client, is_preview);
		if (err)
			return err;
		err = sensor5140_write_table(info->i2c_client, mode_table[sensor_table]);
		if (err)
			return err;
		err = sensor5140_write_table(info->i2c_client, is_capture);
		if (err)
			return err;
	    }
//craig
//craig / 05.19 / fix camera preview size
        //if(!((val == SENSOR_640_WIDTH_VAL) && (sensor_table == SENSOR_MODE_640x480)))
        if(!((val == SENSOR_640_WIDTH_VAL) && (sensor_table == preview_mode)))
//craig
        {
            if(sensor_table == SENSOR_MODE_2592x1944)
                return 0 ; //do not thing
	    pr_err("initial tab %d\n",sensor_table);
	    err = sensor5140_write_table(info->i2c_client, mode_table[sensor_table]);
	    if (err)
	        return err;
//craig / 05.26 / fix AF and solve green screen	        
	    err = sensor5140_write_table(info->i2c_client, is_preview);
		if (err)
			return err;
        }
//craig
//craig / 05.19 / fix camera preview size
        //if((val == SENSOR_640_WIDTH_VAL) && (sensor_table == SENSOR_MODE_640x480))
        if((val == SENSOR_640_WIDTH_VAL) && (sensor_table == preview_mode))
//craig
        {
            sensor5140_write_table(info->i2c_client, is_preview);
        }

	info->mode = sensor_table;
// One time programming table start ONE_TIME_TABLE_CHECK_REG
        err = get_sensor5140_current_width(info->i2c_client, &val,ONE_TIME_TABLE_CHECK_REG);
//Webber for sepia color effect
        if(!(val == ONE_TIME_TABLE_CHECK_DATA))
        //if(!(val == ONE_TIME_TABLE_CHECK_DATA) && !(sensor_table == SENSOR_MODE_640x480))
        {
	       pr_err("First time initial one time tab\n");
	       err = sensor5140_write_table(info->i2c_client, af_load_fw);
	       if (err)
		 return err;  
  
	       err = sensor5140_write_table(info->i2c_client, char_settings);
	       if (err)
		 return err;  

	       err = sensor5140_write_table(info->i2c_client, patch_ram);
	       if (err)
		 return err;

	       err = sensor5140_write_table(info->i2c_client, awb_setting);
	       if (err)
		 return err;

	       err = sensor5140_write_table(info->i2c_client, pa_calib);
	       if (err)
		 return err;

		/* COMPAL - START - [2011/5/15  15:51] - Jamin - [Camera]: fix flash sometimes could not work  */
		//use software enable flash led so disable sensor strobe 
		#if 1
		 sensor5140_write_table(info->i2c_client,flash_off);
		#else
	       err = sensor5140_write_table(info->i2c_client, auto_flash_init);
	       if (err)
		 return err;
		 #endif
		 /* COMPAL - END */
		 

      }
    
	return 0;
}
static int sensor5140_set_af_mode(struct sensor_info *info, u8 mode)
{
	int err;

	pr_info("%s: mode %d\n",__func__, mode);
//craig / 0701 / overexposure
        info->focus_mode = mode;
//craig / 0701 / overexposure
        err = sensor5140_write_table(info->i2c_client, af_mode_table[mode]);
	if (err)
	return err;

	return 0;
}
static int sensor5140_get_af_status(struct sensor_info *info)
{
	int err;
        u16 val;

	pr_info("%s: \n",__func__);
	err = sensor5140_write_reg16(info->i2c_client, 0x098E, 0xB000);
	if (err)
	    return err;
	/* COMPAL - START - [2011/5/15  14:9] - Jamin - [Camera]: fix auto focuse too slow  */
        //msleep(100);
	/* COMPAL - END */
        err = sensor5140_read_reg(info->i2c_client, 0xB000, &val);
	pr_info("%s: value %x \n",__func__, val);
        return val;

}
static long sensor5140_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct sensor_info *info = file->private_data;
        int err=0;
        u16 val;
        int num;


	pr_info("yuv %s\n cmd %u",__func__, cmd);
	switch (cmd) {
	case SENSOR_IOCTL_SET_MODE:
	{
		struct sensor_mode mode;
		if (copy_from_user(&mode,
				   (const void __user *)arg,
				   sizeof(struct sensor_mode))) {
			return -EFAULT;
		}

		return sensor5140_set_mode(info, &mode);
	}
	case SENSOR_IOCTL_GET_STATUS:
	{

		return 0;
	}
	case SENSOR_IOCTL_GET_BRIGHTNESS:
	{
           /* COMPAL - START - [2011/5/18  18:51] - Jamin - [Camera]: fix sometimes can not get brightness   */
           pr_info(" %s: SENSOR_IOCTL_GET_BRIGHTNESS ...+ \n",__func__); 
	    #if 0
	    err = sensor5140_write_reg16(info->i2c_client, 0x098E, 0xB80C);
	    if (err)
	        return err;
            msleep(100);
		#endif	
	      /* COMPAL - END */
		
            err = sensor5140_read_reg(info->i2c_client, 0xB80C, &val);
	    pr_info("%s: value %x \n",__func__, val);

//craig / 0701 / overexposure
            num = val;
            if (num > 1000)
                lowlight_threshold = 1;
                else
                lowlight_threshold = 0;
//craig / 0701 / overexposure

	    /* COMPAL - START - [2011/5/18  18:53] - Jamin - [Camera]: fix sometimes can not get brightness   */
          pr_info(" Get Brightness %d \n",val);
	    pr_info(" %s: SENSOR_IOCTL_GET_BRIGHTNESS ...- \n",__func__);
	    /* COMPAL - END */
           if (copy_to_user((void __user *)arg, &val,2)) 
	       pr_info("%s %d\n", __func__, __LINE__);
            return 0;
	}
	case SENSOR_IOCTL_SET_FLASH_MODE:
	{
//craig / 0701 / overexposure
             info->flash_mode = arg;
//craig / 0701 / overexposure
	  	/* COMPAL - START - [2011/5/15  15:53] - Jamin - [Camera]: fix flash sometimes could not work  */
            //use software enable flash led so disable sensor strobe pin
	     #if 1
		//  sensor5140_write_table(info->i2c_client,flash_off);
	     #else
	     if(arg == YUV_FlashAuto)
                sensor5140_write_table(info->i2c_client,flash_auto) ;
             if(arg == YUV_FlashOn)
                sensor5140_write_table(info->i2c_client,flash_on) ;
	      /* COMPAL - START - [2011/4/22  12:3] - Jamin - [Camera]: fix user setting flash off issue  */
	     if(arg== YUV_FlashOff)
		   sensor5140_write_table(info->i2c_client,flash_off);
		 
            err = sensor5140_read_reg(info->i2c_client, 0xC030, &val);
	     #endif
	    /* COMPAL - END */
	    pr_info("Set flash mode as %lu reg value %x\n",arg, val);
            return 0;
	}
	case SENSOR_IOCTL_GET_AF_STATUS:
	{
            return sensor5140_get_af_status(info);
	}
	case SENSOR_IOCTL_SET_AF_MODE:
	{
            return sensor5140_set_af_mode(info, arg);
	}
	case SENSOR_IOCTL_CAPTURE_CMD:
	{
#if YUV_SENSOR_STROBE
            if(arg == 1)
            {
                sensor5140_write_table(info->i2c_client, mode_capture_cmd);
	        pr_info("send capture command finish \n");
            }
            if(arg == 2)
            {
                err = get_sensor5140_current_width(info->i2c_client, &val,CAM_CORE_B_OUTPUT_SIZE_WIDTH);
                if(val != 640)
                    sensor5140_write_table(info->i2c_client, contextb_vga);
	        pr_info("switch to context-B finish %d\n",val);
            }
            if(arg == 3)
            {
                err = get_sensor5140_current_width(info->i2c_client, &val,CAM_CORE_B_OUTPUT_SIZE_WIDTH);
                if(val != 2592)
                    sensor5140_write_table(info->i2c_client, mode_2592x1944);
	        pr_info("switch to context-B finish %d\n",val);
            }
#endif
            return err;
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
                      pr_info("YUV5140_ColorEffect_None\n");
	                 err = sensor5140_write_table(info->i2c_client, ColorEffect_None);
                         break;
                    case YUV_ColorEffect_Mono:
                      pr_info("YUV5140_ColorEffect_Mono\n");
	                 err = sensor5140_write_table(info->i2c_client, ColorEffect_Mono);
                         break;
                    case YUV_ColorEffect_Sepia:
                      pr_info("YUV5140_ColorEffect_Sepia\n");
	                 err = sensor5140_write_table(info->i2c_client, ColorEffect_Sepia);
                         break;
                    case YUV_ColorEffect_Negative:
                      pr_info("YUV5140_ColorEffect_Negative\n");
	                 err = sensor5140_write_table(info->i2c_client, ColorEffect_Negative);
                         break;
                    case YUV_ColorEffect_Solarize:
                      pr_info("YUV5140_ColorEffect_Solarize\n");
	                 err = sensor5140_write_table(info->i2c_client, ColorEffect_Solarize);
                         break;
                    case YUV_ColorEffect_Posterize:
                      pr_info("YUV5140_ColorEffect_Posterize\n");
	                 err = sensor5140_write_table(info->i2c_client, ColorEffect_Posterize);
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
                     pr_info("YUV5140_Whitebalance_Auto\n");
	                 err = sensor5140_write_table(info->i2c_client, Whitebalance_Auto);
                         break;
                    case YUV_Whitebalance_Incandescent:
                     pr_info("YUV5140_Whitebalance_Incandescent\n");
	                 err = sensor5140_write_table(info->i2c_client, Whitebalance_Incandescent);
                         break;
                    case YUV_Whitebalance_Daylight:
                     pr_info("YUV5140_Whitebalance_Daylight\n");
	                 err = sensor5140_write_table(info->i2c_client, Whitebalance_Daylight);
                         break;
                    case YUV_Whitebalance_Fluorescent:
                     pr_info("YUV5140_Whitebalance_Fluorescent\n");
	                 err = sensor5140_write_table(info->i2c_client, Whitebalance_Fluorescent);
                         break;
//[ASD2ES1 | Craig | 2011.04.06] for 5M camera settings START
                case YUV_Whitebalance_CloudyDaylight:
                     err = sensor5140_write_table(info->i2c_client, Whitebalance_CloudyDaylight);
                     break; 
//[ASD2ES1 | Craig | 2011.04.06] for 5M camera settings END
                    default:
                         break;
                }
	        if (err)
		   return err;
                return 0;
        }
        case SENSOR_IOCTL_SET_SCENE_MODE:
        {
              u8 scenemode;
              if (copy_from_user(&scenemode,
				 (const void __user *)arg,
				 sizeof(scenemode))) {
		return -EFAULT;
	       }
//craig / 06.24 / for fix 5M camcorder fps to 30	       
	    if (video_mode){
//craig
            switch(scenemode)
            {
                case YUV_SceneMode_Auto:
                     pr_info("YUV5140_SceneMode_Auto\n");
	             err = sensor5140_write_table(info->i2c_client, scene_auto);
                     break;
                case YUV_SceneMode_Action:
                     pr_info("YUV5140_SceneMode_Action\n");
	             err = sensor5140_write_table(info->i2c_client, scene_action);
                     break;
                case YUV_SceneMode_Portrait:
                     pr_info("yuv5140 SceneMode_Protrait\n");
	             err = sensor5140_write_table(info->i2c_client, scene_portrait);
                     break;
                case YUV_SceneMode_Landscape:
                     pr_info("YUV5140_SceneMode_Landscape\n");
	             err = sensor5140_write_table(info->i2c_client, scene_landscape);
                     break;
                case YUV_SceneMode_Night:
                     pr_info("YUV5140_SceneMode_Night\n");
	             err = sensor5140_write_table(info->i2c_client, scene_night);
                     break;
                case YUV_SceneMode_NightPortrait:
                     pr_info("YUV5140_SceneMode_NightPortrait\n");
	             err = sensor5140_write_table(info->i2c_client, scene_nightportrait);
                     break;
                case YUV_SceneMode_Theatre:
                     pr_info("YUV5140_SceneMode_Theatre\n");
	             err = sensor5140_write_table(info->i2c_client, scene_theatre);
                     break;
                case YUV_SceneMode_Beach:
                     pr_info("YUV5140_SceneMode_Beach\n");
	             err = sensor5140_write_table(info->i2c_client, scene_beach);
                     break;
                case YUV_SceneMode_Snow:
                     pr_info("YUV5140_SceneMode_Snow\n");
	             err = sensor5140_write_table(info->i2c_client, scene_snow);
                     break;
                case YUV_SceneMode_Sunset:
                     pr_info("YUV5140_SceneMode_Sunset\n");
	             err = sensor5140_write_table(info->i2c_client, scene_sunset);
                     break;
                case YUV_SceneMode_SteadyPhoto:
                     pr_info("YUV5140_SceneMode_SteadyPhoto\n");
	             err = sensor5140_write_table(info->i2c_client, scene_steadyphoto);
                     break;
                case YUV_SceneMode_Fireworks:
                     pr_info("YUV5140_SceneMode_Fireworks\n");
	             err = sensor5140_write_table(info->i2c_client, scene_fireworks);
                     break;

                default:
                     break;
                }
			}
	    if (err)
		return err;
                return 0;
        }
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
                     pr_info("yuv5140 SET_EXPOSURE 0\n");
	             err = sensor5140_write_table(info->i2c_client, exp_zero);
                     break;
                case YUV_Exposure_1:
                     pr_info("yuv5140 SET_EXPOSURE 1\n");
	             err = sensor5140_write_table(info->i2c_client, exp_one);
                     break;
                case YUV_Exposure_2:
                     pr_info("yuv5140 SET_EXPOSURE 2\n");
	             err = sensor5140_write_table(info->i2c_client, exp_two);
                     break;
                case YUV_Exposure_Negative_1:
                     pr_info("yuv5140 SET_EXPOSURE -1\n");
	             err = sensor5140_write_table(info->i2c_client, exp_negative1);
                     break;
                case YUV_Exposure_Negative_2:
                     pr_info("yuv5140 SET_EXPOSURE -2\n");
	             err = sensor5140_write_table(info->i2c_client, exp_negative2);
                     break;
                default:
                     break;
                }
            if (err)
	        return err;
                return 0;
        }
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor5140_open(struct inode *inode, struct file *file)
{
	pr_info("yuv %s\n",__func__);
	file->private_data = info;
	if (info->pdata && info->pdata->power_on)
		info->pdata->power_on();
	return 0;
}

int sensor5140_release(struct inode *inode, struct file *file)
{
       pr_info("yuv %s\n",__func__);
	if (info->pdata && info->pdata->power_off)
		info->pdata->power_off();
	file->private_data = NULL;
	return 0;
}


static const struct file_operations sensor_fileops = {
	.owner = THIS_MODULE,
	.open = sensor5140_open,
	.unlocked_ioctl = sensor5140_ioctl,
	.release = sensor5140_release,
};

static struct miscdevice sensor_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SENSOR_NAME,
	.fops = &sensor_fileops,
};

static int sensor5140_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err;

	pr_info("yuv %s\n",__func__);

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);

	if (!info) {
		pr_err("yuv_sensor5140 : Unable to allocate memory!\n");
		return -ENOMEM;
	}

	err = misc_register(&sensor_device);
	if (err) {
		pr_err("yuv_sensor5140 : Unable to register misc device!\n");
		kfree(info);
		return err;
	}

	info->pdata = client->dev.platform_data;
	info->i2c_client = client;

	i2c_set_clientdata(client, info);
	return 0;
}

static int sensor5140_remove(struct i2c_client *client)
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
	.probe = sensor5140_probe,
	.remove = sensor5140_remove,
	.id_table = sensor_id,
};

static int __init sensor5140_init(void)
{
	pr_info("yuv %s\n",__func__);
	return i2c_add_driver(&sensor_i2c_driver);
}

static void __exit sensor5140_exit(void)
{
	pr_info("yuv %s\n",__func__);
	i2c_del_driver(&sensor_i2c_driver);
}

module_init(sensor5140_init);
module_exit(sensor5140_exit);

