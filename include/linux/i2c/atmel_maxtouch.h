/*
 *  Atmel maXTouch header file
 *
 *  Copyright (c) 2010 Atmel Corporation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 or 3 as
 *  published by the Free Software Foundation.
 *  See the file "COPYING" in the main directory of this archive
 *  for more details.
 *
 */

#define	MXT224_I2C_ADDR1				0x4A
#define	MXT224_I2C_ADDR2				0x4B
#define	MXT1386_I2C_ADDR1				0x4C
#define	MXT1386_I2C_ADDR2				0x4D
#define	MXT1386_I2C_ADDR3				0x5A
#define	MXT1386_I2C_ADDR4				0x5B

/*
 * Select this address from above depending on what maXTouch
 * chip you have and how it's address pins are configured;
 * see datasheet.
 */

/* COMPAL - START - [2011/1/18  17:59] - Jamin - [Touch]: enable touch atmel   */
 //change slave address 0x4c
 #if 1
 #define MXT_I2C_ADDRESS                                 MXT1386_I2C_ADDR1
 #else
#define MXT_I2C_ADDRESS                                 MXT1386_I2C_ADDR3
 #endif
/* COMPAL - END */

/* COMPAL - START - [2011/6/16  14:23] - Jamin - [Touch]: update firmware 1.1  */
#if 1
#define MXT_BOOT_ADDRESS                                  0x26
#define MXT_TOTAL_FRAME_SIZE 235
#define I2C_BOOT_READ_OK                 1u
#define I2C_BOOT_READ_FAILED             2u
#define I2C_BOOT_WRITE_OK                1u
#define I2C_BOOT_WRITE_FAILED            2u
#define UPDATE_FW_OK 1u
#define UPDATE_FW_FAILED 2u
#else
#define MXT_BL_ADDRESS                                  0x25
#endif
/* COMPAL - END */

#define	MXT224_FAMILYID				        0x80
#define MXT1386_FAMILYID                                0xA0

#define	MXT224_CAL_VARIANTID				0x01
#define MXT224_UNCAL_VARIANTID                          0x00
#define MXT1386_CAL_VARIANTID                           0x00

#define MXT_MAX_REPORTED_WIDTH                          255
#define MXT_MAX_REPORTED_PRESSURE                       255
#define MXT_MAX_TOUCH_SIZE                              255
#define MXT_MAX_NUM_TOUCHES                             10

/* Fixed addresses inside maXTouch device */
#define	MXT_ADDR_INFO_BLOCK				0
#define	MXT_ADDR_OBJECT_TABLE				7
#define MXT_ID_BLOCK_SIZE                               7
#define	MXT_OBJECT_TABLE_ELEMENT_SIZE			6

/* Object types */
#define	MXT_DEBUG_DELTAS_T2				2
#define	MXT_DEBUG_REFERENCES_T3				3
#define	MXT_GEN_MESSAGEPROCESSOR_T5			5
#define	MXT_GEN_COMMANDPROCESSOR_T6			6
#define	MXT_GEN_POWERCONFIG_T7				7
#define	MXT_GEN_ACQUIRECONFIG_T8			8
#define	MXT_TOUCH_MULTITOUCHSCREEN_T9			9
#define MXT_TOUCH_SINGLETOUCHSCREEN_T10                 10
#define MXT_TOUCH_XSLIDER_T11                           11
#define MXT_TOUCH_YSLIDER_T12                           12
#define MXT_TOUCH_XWHEEL_T13                            13
#define MXT_TOUCH_YWHEEL_T14                            14
#define	MXT_TOUCH_KEYARRAY_T15				15
/* COMPAL - START - [2011/1/20  11:5] - Jamin - [Touch]: enable touch atmel   */
#define MXT_SPT_COMMSCONFIG_T18               18
/* COMPAL - END */
#define	MXT_SPT_GPIOPWM_T19				19
#define	MXT_PROCI_GRIPFACESUPPRESSION_T20		20
#define	MXT_PROCG_NOISESUPPRESSION_T22			22
#define	MXT_TOUCH_PROXIMITY_T23				23
#define	MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24		24
#define	MXT_SPT_SELFTEST_T25				25
#define MXT_DEBUG_CTERANGE_T26				26
#define	MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27		27
#define	MXT_SPT_CTECONFIG_T28				28
#define	MXT_TOUCH_KEYSET_T31				31
#define	MXT_TOUCH_XSLIDERSET_T32			32
#define	MXT_DEBUG_DIAGNOSTIC_T37			37
#define	MXT_USER_INFO_T38				38
/* COMPAL - START - [2011/1/20  11:11] - Jamin - [Touch]: enable touch atmel   */
#define MXT_PROCI_GRIPSUPPRESSION_T40  	40
#define MXT_PROCI_PALMSUPPRESSION_T41  	41
#define MXT_SPT_DIGITIZER_T43      43
#define MXT_SPT_MESSAGECOUNT_T44 44
/* COMPAL - END */

/*
 * If a message is read from mXT when there's no new messages available,
 * the report ID of the message will be 0xFF.
 */
#define	MXT_END_OF_MESSAGES				0xFF

/* GEN_COMMANDPROCESSOR_T6 Register offsets from T6 base address */
#define	MXT_ADR_T6_RESET				0x00
#define	MXT_ADR_T6_BACKUPNV				0x01
#define	MXT_ADR_T6_CALIBRATE				0x02
#define	MXT_ADR_T6_REPORTALL				0x03
#define	MXT_ADR_T6_RESERVED				0x04
#define	MXT_ADR_T6_DIAGNOSTIC				0x05

/* T6 Debug Diagnostics Commands */
#define	MXT_CMD_T6_PAGE_UP          0x01
#define	MXT_CMD_T6_PAGE_DOWN        0x02
#define	MXT_CMD_T6_DELTAS_MODE      0x10
#define	MXT_CMD_T6_REFERENCES_MODE  0x11
#define	MXT_CMD_T6_CTE_MODE         0x31

/* T6 Backup Command */
#define MXT_CMD_T6_BACKUP           0x55

/* SPT_DEBUG_DIAGNOSTIC_T37 Register offsets from T37 base address */
#define MXT_ADR_T37_PAGE                                0x01
#define	MXT_ADR_T37_DATA				0x02

/************************************************************************
 * MESSAGE OBJECTS ADDRESS FIELDS
 *
 ************************************************************************/
#define MXT_MSG_REPORTID                                0x00

/* MXT_GEN_MESSAGEPROCESSOR_T5 Message address definitions		*/
#define	MXT_MSG_T5_REPORTID				0x00
#define	MXT_MSG_T5_MESSAGE				0x01
#define	MXT_MSG_T5_CHECKSUM				0x08

/* MXT_GEN_COMMANDPROCESSOR_T6 Message address definitions		*/
#define	MXT_MSG_T6_STATUS				0x01
#define		MXT_MSGB_T6_COMSERR		0x04
#define		MXT_MSGB_T6_CFGERR		0x08
#define		MXT_MSGB_T6_CAL			0x10
#define		MXT_MSGB_T6_SIGERR		0x20
#define		MXT_MSGB_T6_OFL			0x40
#define		MXT_MSGB_T6_RESET		0x80
/* Three bytes */
#define	MXT_MSG_T6_CHECKSUM				0x02

/* MXT_GEN_POWERCONFIG_T7 NO Message address definitions		*/
/* MXT_GEN_ACQUIRECONFIG_T8 Message address definitions			*/
/* MXT_TOUCH_MULTITOUCHSCREEN_T9 Message address definitions		*/

#define	MXT_MSG_T9_STATUS				0x01
/* Status bit field */
#define		MXT_MSGB_T9_SUPPRESS		0x02
#define		MXT_MSGB_T9_AMP			0x04
#define		MXT_MSGB_T9_VECTOR		0x08
#define		MXT_MSGB_T9_MOVE		0x10
#define		MXT_MSGB_T9_RELEASE		0x20
#define		MXT_MSGB_T9_PRESS		0x40
#define		MXT_MSGB_T9_DETECT		0x80

#define	MXT_MSG_T9_XPOSMSB				0x02
#define	MXT_MSG_T9_YPOSMSB				0x03
#define	MXT_MSG_T9_XYPOSLSB				0x04
#define	MXT_MSG_T9_TCHAREA				0x05
#define	MXT_MSG_T9_TCHAMPLITUDE				0x06
#define	MXT_MSG_T9_TCHVECTOR				0x07

/* MXT_SPT_GPIOPWM_T19 Message address definitions			*/
#define	MXT_MSG_T19_STATUS				0x01

/* MXT_PROCI_GRIPFACESUPPRESSION_T20 Message address definitions	*/
#define	MXT_MSG_T20_STATUS				0x01
#define		MXT_MSGB_T20_FACE_SUPPRESS	0x01
/* MXT_PROCG_NOISESUPPRESSION_T22 Message address definitions		*/
#define	MXT_MSG_T22_STATUS				0x01
#define		MXT_MSGB_T22_FHCHG		0x01
#define		MXT_MSGB_T22_GCAFERR		0x04
#define		MXT_MSGB_T22_FHERR		0x08
#define	MXT_MSG_T22_GCAFDEPTH				0x02

/* MXT_TOUCH_PROXIMITY_T23 Message address definitions			*/
#define	MXT_MSG_T23_STATUS				0x01
#define		MXT_MSGB_T23_FALL		0x20
#define		MXT_MSGB_T23_RISE		0x40
#define		MXT_MSGB_T23_DETECT		0x80
/* 16 bit */
#define	MXT_MSG_T23_PROXDELTA				0x02

/* MXT_PROCI_ONETOUCHGESTUREPROCESSOR_T24 Message address definitions	*/
#define	MXT_MSG_T24_STATUS				0x01
#define	MXT_MSG_T24_XPOSMSB				0x02
#define	MXT_MSG_T24_YPOSMSB				0x03
#define	MXT_MSG_T24_XYPOSLSB				0x04
#define	MXT_MSG_T24_DIR					0x05
/* 16 bit */
#define	MXT_MSG_T24_DIST				0x06

/* MXT_SPT_SELFTEST_T25 Message address definitions			*/
#define	MXT_MSG_T25_STATUS				0x01
/* 5 Bytes */
#define		MXT_MSGR_T25_OK			0xFE
#define		MXT_MSGR_T25_INVALID_TEST	0xFD
#define		MXT_MSGR_T25_PIN_FAULT		0x11
#define		MXT_MSGR_T25_SIGNAL_LIMIT_FAULT	0x17
#define		MXT_MSGR_T25_GAIN_ERROR		0x20
#define	MXT_MSG_T25_INFO				0x02

/* MXT_PROCI_TWOTOUCHGESTUREPROCESSOR_T27 Message address definitions	*/
#define	MXT_MSG_T27_STATUS			0x01
#define		MXT_MSGB_T27_ROTATEDIR		0x10
#define		MXT_MSGB_T27_PINCH		0x20
#define		MXT_MSGB_T27_ROTATE		0x40
#define		MXT_MSGB_T27_STRETCH		0x80
#define	MXT_MSG_T27_XPOSMSB			0x02
#define	MXT_MSG_T27_YPOSMSB			0x03
#define	MXT_MSG_T27_XYPOSLSB			0x04
#define	MXT_MSG_T27_ANGLE			0x05

/* 16 bit */
#define	MXT_MSG_T27_SEPARATION				0x06

/* MXT_SPT_CTECONFIG_T28 Message address definitions			*/
#define	MXT_MSG_T28_STATUS				0x01
#define	MXT_MSGB_T28_CHKERR		0x01

/* One Touch Events */
#define	MT_GESTURE_RESERVED				0x00
#define	MT_GESTURE_PRESS				0x01
#define	MT_GESTURE_RELEASE				0x02
#define	MT_GESTURE_TAP					0x03
#define	MT_GESTURE_DOUBLE_TAP				0x04
#define	MT_GESTURE_FLICK				0x05
#define	MT_GESTURE_DRAG					0x06
#define	MT_GESTURE_SHORT_PRESS				0x07
#define	MT_GESTURE_LONG_PRESS				0x08
#define	MT_GESTURE_REPEAT_PRESS				0x09
#define	MT_GESTURE_TAP_AND_PRESS			0x0a
#define	MT_GESTURE_THROW				0x0b

/* Bootloader states */
#define WAITING_BOOTLOAD_COMMAND   0xC0
#define WAITING_FRAME_DATA         0x80
#define APP_CRC_FAIL               0x40
#define FRAME_CRC_CHECK            0x02
#define FRAME_CRC_PASS             0x04
#define FRAME_CRC_FAIL             0x03

#define MXT_MAX_FRAME_SIZE         552 //jamin modify 276

/* Debug levels */
#define NO_DEBUG       0
#define DEBUG_INFO     1
#define DEBUG_VERBOSE  2
#define DEBUG_MESSAGES 5
#define DEBUG_RAW      8
#define DEBUG_TRACE   10

/* IOCTL commands */
#define MXT_SET_ADDRESS     1	/* Sets the internal address pointer */
#define MXT_RESET           2	/* Resets the device */
#define MXT_CALIBRATE       3	/* Calibrates the device */
#define MXT_BACKUP          4	/* Backups the current state of registers to
				   NVM */
#define MXT_NONTOUCH_MSG    5	/* Only non-touch messages can be read from
				   the message buffer
				   (/dev/maXTouch_messages) */
#define MXT_ALL_MSG         6	/* All messages can be read from the message
				   buffer */

/* Message buffer size. This is a ring buffer, and when full, the oldest entry
   will be overwritten. */
#define MXT_MESSAGE_BUFFER_SIZE  128

/* Routines for memory access within a 16 bit address space */

static int mxt_read_block(struct i2c_client *client, u16 addr, u16 length,
			  u8 *value);
static int mxt_write_byte(struct i2c_client *client, u16 addr, u8 value);
static int mxt_write_block(struct i2c_client *client, u16 addr, u16 length,
			   u8 *value);

/* TODO: */
/* Bootloader specific function prototypes. */
/* COMPAL - START - [2011/4/18  15:25] - Jamin - [Touch]: enable atmel touch  */
u8 mxt_boot_read(u16 start, u8 size, u8 *mem);
int mxt_boot_write(u16 reg, u8 *read_val, unsigned int len);
#if 0
static int mxt_read_byte_bl(struct i2c_client *client, u8 * value);
static int mxt_read_block_bl(struct i2c_client *client, u16 length, u8 * value);
static int mxt_write_byte_bl(struct i2c_client *client, u8 value);
static int mxt_write_block_bl(struct i2c_client *client, u16 length,
			      u8 *value);
#endif
/* COMPAL - END */
/**
 * struct mxt_platform_data - includes platform specific informatio
 * related to Atmel maXTouch touchscreen controller.
 *
 * @numtouch:           Number of simultaneous touches supported
 * @init_platform_hw(): Initialization function, which can for example
 *                      trigger a hardware reset by toggling a GPIO pin
 * @exit_platform_hw(): Function to run when the driver is unloaded.
 * @valid_interrupt():  Function that checks the validity of the interrupt -
 *                      function that check the validity of a interrupt (by
 *                      reading the changeline interrupt pin and checking that
 *                      it really is low for example).
 * @max_x:              Reported X range
 * @max_y:              Reported Y range
 */

struct mxt_platform_data {
	u8 numtouch;		/* Number of touches to report  */
	void (*init_platform_hw) (void);
	void (*exit_platform_hw) (void);
	/* COMPAL - START - [2011/3/24  11:43] - Jamin - [Touch]: add reset node  */
       void (*reset_platform_hw) (void);
	u8 (*read_reset)(void);
	/* COMPAL - END */
	int max_x;		/* The default reported X range   */
	int max_y;		/* The default reported Y range   */
	 u8(*valid_interrupt) (void);
	 u8(*read_chg) (void);
};

static u8 mxt_valid_interrupt_dummy(void)
{
	return 1;
}

void mxt_hw_reset(void);
