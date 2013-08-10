#
# Makefile for the video capture/playback device drivers.
#
obj-y				+= avp/
obj-$(CONFIG_TEGRA_MEDIASERVER)	+= mediaserver/
obj-$(CONFIG_TEGRA_CAMERA)	+= tegra_camera.o
obj-$(CONFIG_VIDEO_OV5650)	+= ov5650.o
obj-$(CONFIG_VIDEO_OV2710)	+= ov2710.o
obj-$(CONFIG_VIDEO_SOC380)	+= soc380.o
obj-$(CONFIG_TORCH_SSL3250A)	+= ssl3250a.o
obj-$(CONFIG_VIDEO_SH532U)	+= sh532u.o
obj-$(CONFIG_VIDEO_AD5820)	+= ad5820.o
#/* COMPAL - START - [2011/8/25] - Sandow - [Camera]: enable Aptina 5140  */
obj-$(CONFIG_VIDEO_YUV_AP5140)	        += yuv_sensor_ap5140.o
#/* COMPAL - START - [2011/8/25] - Sandow - [Camera]: enable Aptina 2031  */
obj-$(CONFIG_VIDEO_YUV_AP2031)	        += yuv_sensor_ap2031.o
#/* COMPAL - START - [2011/8/25] - Sandow - [Camera]: enable flash led tps61052  */
obj-$(CONFIG_FLASH_LED_TPS61052)          += ltc3216.o
#/* COMPAL - END */


