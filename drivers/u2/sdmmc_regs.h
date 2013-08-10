#ifndef __SDMMC_REGS_H
#define __SDMMC_REGS_H

#define SDMMC_SYSTEM_ADDRESS			0x0
#define SDMMC_BLOCK_SIZE_BLOCK_COUNT	0x4
#define SDMMC_ARGUMENT					0x8
#define SDMMC_CMD_XFER_MODE				0xC
#define SDMMC_RESPONSE_R0_R1			0x10
#define SDMMC_RESPONSE_R2_R3			0x14
#define SDMMC_RESPONSE_R4_R5			0x18
#define SDMMC_RESPONSE_R6_R7			0x1C
#define SDMMC_BUFFER_DATA_PORT			0x20
#define SDMMC_PRESENT_STATE				0x24
#define SDMMC_POWER_CONTROL_HOST		0x28
#define SDMMC_SW_RESET_TIMEOUT_CTRL_CLOCK_CONTROL	0x2C
#define SDMMC_INTERRUPT_STATUS			0x30
#define SDMMC_INTERRUPT_STATUS_ENABLE	0x34
#define SDMMC_INTERRUPT_SIGNAL_ENABLE	0x38
#define SDMMC_AUTO_CMD12_ERR_STATUS		0x3C
#define SDMMC_CAPABILITIES				0x40
#define SDMMC_MAXIMUM_CURRENT			0x48
#define SDMMC_FORCE_EVENT				0x50
#define SDMMC_ADMA_ERR_STATUS			0x54
#define SDMMC_ADMA_SYSTEM_ADDRESS		0x58
#define SDMMC_SPI_INTERRUPT_SUPPORT		0xF0
#define SDMMC_SLOT_INTERRUPT_STATUS		0xFC
#define SDMMC_VENDOR_CLOCK_CNTRL		0x100
#define SDMMC_VENDOR_SPI_CNTRL			0x104
#define SDMMC_VENDOR_SPI_INTR_STATUS	0x108
#define SDMMC_VENDOR_CEATA_CNTRL		0x10C
#define SDMMC_VENDOR_BOOT_CNTRL			0x110
#define SDMMC_VENDOR_BOOT_ACK_TIMEOUT	0x114
#define SDMMC_VENDOR_BOOT_DAT_TIMEOUT	0x118
#define SDMMC_VENDOR_DEBOUNCE_COUNT		0x11C

#endif
