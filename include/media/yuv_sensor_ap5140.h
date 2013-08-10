#ifndef ___YUV5140_SENSOR_H__
#define ___YUV5140_SENSOR_H__

#include <linux/ioctl.h>  /* For IOCTL macros */

/*-------------------------------------------Important---------------------------------------
 * for changing the SENSOR_NAME, you must need to change the owner of the device. For example
 * Please add /dev/mt9d111 0600 media camera in below file
 * ./device/nvidia/ventana/ueventd.ventana.rc
 * Otherwise, ioctl will get permission deny
 * -------------------------------------------Important--------------------------------------
*/

#define SENSOR_NAME	"mt9d111"
#define DEV(x)          "/dev/"x
#define SENSOR_PATH     DEV(SENSOR_NAME)
#define LOG_NAME(x)     "ImagerODM-"x
#define LOG_TAG         LOG_NAME(SENSOR_NAME)
/* COMPAL - START - [2011/2/22  19:51] - Jamin - [Camera]: enable Aptina 5140  */
#define SENSOR_NAME_AP5140	"mt9d111"
/* COMPAL - END */
/* COMPAL - START - [2011/2/22  19:28] - Jamin - [Camera]: enable Aptina 2031 */
#define SENSOR_NAME_AP2031  "mt9d115"
/* COMPAL - END */

#define SENSOR_WAIT_MS       0   /* special number to indicate this is wait time require */
#define SENSOR_TABLE_END     1   /* special number to indicate this is end of table */
#define WRITE_REG_DATA8      2   /* special the data width as one byte */
#define WRITE_REG_DATA16     3   /* special the data width as one byte */
#define POLL_REG_BIT         4   /* poll the bit set */
//craig / 0701 / overexposure
#define CHECK_FLASH         20
//craig / 0701 / overexposure

#define SENSOR_MAX_RETRIES   5      /* max counter for retry I2C access */
/* COMPAL - START - [2011/5/9  17:7] - Jamin - [Camera]: fix hand issue  */
#if 1
#define SENSOR_POLL_RETRIES  500  /* max poll retry */
#else
#define SENSOR_POLL_RETRIES  20000  /* max poll retry */
#endif
/* COMPAL - END */
#define SENSOR_POLL_WAITMS   1      /* poll wait time */

#define SENSOR_IOCTL_SET_MODE		_IOW('o', 1, struct sensor_mode)
#define SENSOR_IOCTL_GET_STATUS		_IOR('o', 2, __u8)
#define SENSOR_IOCTL_SET_COLOR_EFFECT   _IOW('o', 3, __u8)
#define SENSOR_IOCTL_SET_WHITE_BALANCE  _IOW('o', 4, __u8)
#define SENSOR_IOCTL_SET_SCENE_MODE     _IOW('o', 5, __u8)
#define SENSOR_IOCTL_SET_AF_MODE        _IOW('o', 6, __u8)
#define SENSOR_IOCTL_GET_AF_STATUS      _IOW('o', 7, __u8)
#define SENSOR_IOCTL_SET_EXPOSURE       _IOW('o', 8,   int)
#define SENSOR_IOCTL_CAPTURE_CMD        _IOW('o', 9,  __u8)
#define SENSOR_IOCTL_SET_FLASH_MODE     _IOW('o', 10, __u8)
#define SENSOR_IOCTL_GET_BRIGHTNESS     _IOW('o', 11, __u16)
//[ASD2ES1 | Craig | 2011.04.09] for 5M Camera Picture Quality START
#define SENSOR_IOCTL_SET_PICTURE_QUALITY _IOW('o', 12, __u8)
//[ASD2ES1 | Craig | 2011.04.09] for 5M Camera Picture Quality END 

enum {
      IsYUVSensor = 1,
      Brightness,
} ;

enum {
      YUV_FlashOn = 0,
      YUV_FlashOff,
      YUV_FlashAuto,
      YUV_FlashTorch
};

enum {
      YUV_ColorEffect = 0,
      YUV_Whitebalance,
      YUV_SceneMode,
      YUV_Exposure,
      YUV_FlashMode,
//[ASD2ES1 | Craig | 2011.04.09] for 5M Camera Picture Quality START
      YUV_PictureQuality
//[ASD2ES1 | Craig | 2011.04.09] for 5M Camera Picture Quality END 
};

enum {
      YUV_ColorEffect_Invalid = 0,
      YUV_ColorEffect_Aqua,
      YUV_ColorEffect_Blackboard,
      YUV_ColorEffect_Mono,
      YUV_ColorEffect_Negative,
      YUV_ColorEffect_None,
      YUV_ColorEffect_Posterize,
      YUV_ColorEffect_Sepia,
      YUV_ColorEffect_Solarize,
      YUV_ColorEffect_Whiteboard
};

enum {
      YUV_Whitebalance_Invalid = 0,
      YUV_Whitebalance_Auto,
      YUV_Whitebalance_Incandescent,
      YUV_Whitebalance_Fluorescent,
      YUV_Whitebalance_WarmFluorescent,
      YUV_Whitebalance_Daylight,
      YUV_Whitebalance_CloudyDaylight,
      YUV_Whitebalance_Shade,
      YUV_Whitebalance_Twilight,
      YUV_Whitebalance_Custom
};

enum {
      YUV_SceneMode_Invalid = 0,
      YUV_SceneMode_Auto,
      YUV_SceneMode_Action,
      YUV_SceneMode_Portrait,
      YUV_SceneMode_Landscape,
      YUV_SceneMode_Beach,
      YUV_SceneMode_Candlelight,
      YUV_SceneMode_Fireworks,
      YUV_SceneMode_Night,
      YUV_SceneMode_NightPortrait,
      YUV_SceneMode_Party,
      YUV_SceneMode_Snow,
      YUV_SceneMode_Sports,
      YUV_SceneMode_SteadyPhoto,
      YUV_SceneMode_Sunset,
      YUV_SceneMode_Theatre,
      YUV_SceneMode_Barcode
};

struct sensor_mode {
	int xres;
	int yres;
};

#ifdef __KERNEL__
struct yuv_sensor_platform_data {
	int (*power_on)(void);
	int (*power_off)(void);

};
#endif /* __KERNEL__ */

#endif  /* __YUV_SENSOR_H__ */

