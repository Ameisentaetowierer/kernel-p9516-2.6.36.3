#ifndef ___YUV2031_SENSOR_H__
#define ___YUV2031_SENSOR_H__

#include <linux/ioctl.h>  /* For IOCTL macros */

#define SENSOR_NAME	"mt9d115"
#define SENSOR_PATH     "/dev/mt9d115"

#define SENSOR_WAIT_MS       0 /* special number to indicate this is wait time require */
#define SENSOR_TABLE_END     1 /* special number to indicate this is end of table */
#define SENSOR_MAX_RETRIES   5 /* max counter for retry I2C access */

#define SENSOR_IOCTL_SET_MODE		_IOW('o', 1, struct sensor_mode)
#define SENSOR_IOCTL_GET_STATUS		_IOR('o', 2, __u8)
#define SENSOR_IOCTL_SET_COLOR_EFFECT   _IOW('o', 3, __u8)
#define SENSOR_IOCTL_SET_WHITE_BALANCE  _IOW('o', 4, __u8)
#define SENSOR_IOCTL_SET_SCENE_MODE     _IOW('o', 5, __u8)
#define SENSOR_IOCTL_SET_AF_MODE        _IOW('o', 6, __u8)
#define SENSOR_IOCTL_GET_AF_STATUS      _IOW('o', 7, __u8)
//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings START
#define SENSOR_IOCTL_SET_EXPOSURE       _IOW('o', 8, __u8)
//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings END
//[ASD2ES1 | Craig | 2011.04.08] for 2M camera Picture Quality START
#define SENSOR_IOCTL_SET_PICTURE_QUALITY _IOW('o', 9, __u8)
//[ASD2ES1 | Craig | 2011.04.08] for 2M camera Picture Quality END

enum {
      YUV_ColorEffect = 0,
      YUV_Whitebalance,
      YUV_SceneMode,
//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings START
      YUV_Exposure,
//[ASD2ES1 | Craig | 2011.04.06] for 2M camera settings END
//[ASD2ES1 | Craig | 2011.04.08] for 2M camera Picture Quality START
      YUV_PictureQuality
//[ASD2ES1 | Craig | 2011.04.08] for 2M camera Picture Quality END
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
