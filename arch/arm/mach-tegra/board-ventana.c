/*
 * arch/arm/mach-tegra/board-ventana.c
 *
 * Copyright (c) 2010 - 2011, NVIDIA Corporation.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/usb/android_composite.h>
#include <linux/usb/f_accessory.h>
#include <linux/mfd/tps6586x.h>
#include <linux/memblock.h>

#ifdef CONFIG_TOUCHSCREEN_PANJIT_I2C
#include <linux/i2c/panjit_ts.h>
#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MT_T9
#include <linux/i2c/atmel_maxtouch.h>
#endif

#include <sound/wm8903.h>

#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/iomap.h>
#include <mach/io.h>
#include <mach/i2s.h>
#include <mach/spdif.h>
#include <mach/audio.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/usb_phy.h>
#include <mach/tegra_das.h>

#include "board.h"
#include "clock.h"
#include "board-ventana.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "wakeups-t2.h"
#include <linux/spi/spi.h>      //+++[ASD2_ES1|Jaco_Gao|2011.04.02] Enable OFN

static struct usb_mass_storage_platform_data tegra_usb_fsg_platform = {
	.vendor = "NVIDIA",
	.product = "Tegra 2",
//Compal Start William 2011-08-23 , for not show USB mass storage in Windows
	.nluns = 0,
//	.nluns = 1,
//Compal Start William 2011-08-23

};

static struct platform_device tegra_usb_fsg_device = {
	.name = "usb_mass_storage",
	.id = -1,
	.dev = {
		.platform_data = &tegra_usb_fsg_platform,
	},
};

static struct plat_serial8250_port debug_uart_platform_data[] = {
	{
		.membase	= IO_ADDRESS(TEGRA_UARTD_BASE),
		.mapbase	= TEGRA_UARTD_BASE,
		.irq		= INT_UARTD,
		.flags		= UPF_BOOT_AUTOCONF | UPF_FIXED_TYPE,
		.type           = PORT_TEGRA,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 216000000,
	}, {
		.flags		= 0,
	}
};

static struct platform_device debug_uart = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = debug_uart_platform_data,
	},
};

static struct tegra_audio_platform_data tegra_spdif_pdata = {
	.dma_on = true,  /* use dma by default */
	.spdif_clk_rate = 5644800,
};

static struct tegra_utmip_config utmi_phy_config[] = {
	[0] = {
			.hssync_start_delay = 9,
			.idle_wait_delay = 17,
			.elastic_limit = 16,
			.term_range_adj = 6,
			.xcvr_setup = 15,
			.xcvr_lsfslew = 2,
			.xcvr_lsrslew = 2,
	},
	[1] = {
			.hssync_start_delay = 9,
			.idle_wait_delay = 17,
			.elastic_limit = 16,
			.term_range_adj = 6,
			.xcvr_setup = 8,
			.xcvr_lsfslew = 2,
			.xcvr_lsrslew = 2,
	},
};

static struct tegra_ulpi_config ulpi_phy_config = {
	.reset_gpio = TEGRA_GPIO_PG2,
	.clk = "clk_dev2",
	.inf_type = TEGRA_USB_LINK_ULPI,
};

#ifdef CONFIG_BCM4329_RFKILL

static struct resource ventana_bcm4329_rfkill_resources[] = {
	{
		.name   = "bcm4329_nshutdown_gpio",
		.start  = TEGRA_GPIO_PU0,
		.end    = TEGRA_GPIO_PU0,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device ventana_bcm4329_rfkill_device = {
	.name = "bcm4329_rfkill",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(ventana_bcm4329_rfkill_resources),
	.resource       = ventana_bcm4329_rfkill_resources,
};

static noinline void __init ventana_bt_rfkill(void)
{
	/*Add Clock Resource*/
	clk_add_alias("bcm4329_32k_clk", ventana_bcm4329_rfkill_device.name, \
				"blink", NULL);

	platform_device_register(&ventana_bcm4329_rfkill_device);

	return;
}
#else
static inline void ventana_bt_rfkill(void) { }
#endif

#ifdef CONFIG_BT_BLUESLEEP
static noinline void __init tegra_setup_bluesleep(void)
{
	struct platform_device *pdev = NULL;
	struct resource *res;

	pdev = platform_device_alloc("bluesleep", 0);
	if (!pdev) {
		pr_err("unable to allocate platform device for bluesleep");
		return;
	}

	res = kzalloc(sizeof(struct resource) * 3, GFP_KERNEL);
	if (!res) {
		pr_err("unable to allocate resource for bluesleep\n");
		goto err_free_dev;
	}

	res[0].name   = "gpio_host_wake";
	res[0].start  = TEGRA_GPIO_PU6;
	res[0].end    = TEGRA_GPIO_PU6;
	res[0].flags  = IORESOURCE_IO;

	res[1].name   = "gpio_ext_wake";
	res[1].start  = TEGRA_GPIO_PU1;
	res[1].end    = TEGRA_GPIO_PU1;
	res[1].flags  = IORESOURCE_IO;

	res[2].name   = "host_wake";
	res[2].start  = gpio_to_irq(TEGRA_GPIO_PU6);
	res[2].end    = gpio_to_irq(TEGRA_GPIO_PU6);
	res[2].flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE;

	if (platform_device_add_resources(pdev, res, 3)) {
		pr_err("unable to add resources to bluesleep device\n");
		goto err_free_res;
	}

	if (platform_device_add(pdev)) {
		pr_err("unable to add bluesleep device\n");
		goto err_free_res;
	}

	tegra_gpio_enable(TEGRA_GPIO_PU6);
	tegra_gpio_enable(TEGRA_GPIO_PU1);

	return;

err_free_res:
	kfree(res);
err_free_dev:
	platform_device_put(pdev);
	return;
}
#else
static inline void tegra_setup_bluesleep(void) { }
#endif

static __initdata struct tegra_clk_init_table ventana_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "uartd",	"pll_p",	216000000,	true},
	{ "uartc",	"pll_m",	600000000,	false},
	{ "blink",	"clk_32k",	32768,		false},
	{ "pll_p_out4",	"pll_p",	24000000,	true },
	{ "pwm",	"clk_32k",	32768,		false},
	{ "pll_a",	NULL,		56448000,	false},
	{ "pll_a_out0",	NULL,		11289600,	false},
	{ "clk_dev1",	"pll_a_out0",	0,		true},
	{ "i2s1",	"pll_a_out0",	11289600,	false},
	{ "i2s2",	"pll_a_out0",	11289600,	false},
	{ "audio",	"pll_a_out0",	11289600,	false},
	{ "audio_2x",	"audio",	22579200,	false},
	{ "spdif_out",	"pll_a_out0",	5644800,	false},
	{ "kbc",	"clk_32k",	32768,		true},
	{ NULL,		NULL,		0,		0},
};

//Compal Start William 2011-08-23
#if 0
#define USB_MANUFACTURER_NAME		"NVIDIA"
#define USB_PRODUCT_NAME		"Ventana"
#define USB_PRODUCT_ID_MTP_ADB		0x7100
#define USB_PRODUCT_ID_MTP		0x7102
#define USB_PRODUCT_ID_RNDIS		0x7103
#define USB_VENDOR_ID			0x0955
#else
#define USB_MANUFACTURER_NAME		"lenovo"
#define USB_PRODUCT_NAME		"IdeaPad Tablet K1"
#define USB_PRODUCT_ID_MTP_ADB		0x740A
#define USB_PRODUCT_ID_MTP		0x740A
#define USB_PRODUCT_ID_RNDIS		0x740B
#define USB_VENDOR_ID			0x17EF
#endif
//Compal End William  2011-08-23

static char *usb_functions_mtp_ums[] = { "mtp", "usb_mass_storage" };
static char *usb_functions_mtp_adb_ums[] = { "mtp", "adb", "usb_mass_storage" };
#ifdef CONFIG_USB_ANDROID_ACCESSORY
static char *usb_functions_accessory[] = { "accessory" };
static char *usb_functions_accessory_adb[] = { "accessory", "adb" };
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
static char *usb_functions_rndis[] = { "rndis" };
static char *usb_functions_rndis_adb[] = { "rndis", "adb" };
#endif
static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	"accessory",
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
	"mtp",
	"adb",
	"usb_mass_storage"
};

static struct android_usb_product usb_products[] = {
	{
		.product_id     = USB_PRODUCT_ID_MTP,
		.num_functions  = ARRAY_SIZE(usb_functions_mtp_ums),
		.functions      = usb_functions_mtp_ums,
	},
	{
		.product_id     = USB_PRODUCT_ID_MTP_ADB,
		.num_functions  = ARRAY_SIZE(usb_functions_mtp_adb_ums),
		.functions      = usb_functions_mtp_adb_ums,
	},
#ifdef CONFIG_USB_ANDROID_ACCESSORY
	{
		.vendor_id      = USB_ACCESSORY_VENDOR_ID,
		.product_id     = USB_ACCESSORY_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_accessory),
		.functions      = usb_functions_accessory,
	},
	{
		.vendor_id      = USB_ACCESSORY_VENDOR_ID,
		.product_id     = USB_ACCESSORY_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_accessory_adb),
		.functions      = usb_functions_accessory_adb,
	},
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	{
		.product_id     = USB_PRODUCT_ID_RNDIS,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis),
		.functions      = usb_functions_rndis,
	},
	{
		.product_id     = USB_PRODUCT_ID_RNDIS,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis_adb),
		.functions      = usb_functions_rndis_adb,
	},
#endif
};

/* standard android USB platform data */
static struct android_usb_platform_data andusb_plat = {
	.vendor_id              = USB_VENDOR_ID,
	.product_id             = USB_PRODUCT_ID_MTP_ADB,
	.manufacturer_name      = USB_MANUFACTURER_NAME,
	.product_name           = USB_PRODUCT_NAME,
	.serial_number          = NULL,
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static struct platform_device androidusb_device = {
	.name   = "android_usb",
	.id     = -1,
	.dev    = {
		.platform_data  = &andusb_plat,
	},
};

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data rndis_pdata = {
	.ethaddr = {0, 0, 0, 0, 0, 0},
	.vendorID = USB_VENDOR_ID,
	.vendorDescr = USB_MANUFACTURER_NAME,
};

static struct platform_device rndis_device = {
	.name   = "rndis",
	.id     = -1,
	.dev    = {
		.platform_data  = &rndis_pdata,
	},
};
#endif

static struct wm8903_platform_data wm8903_pdata = {
	.irq_active_low = 0,
	.micdet_cfg = 0x83,           /* enable mic bias current */
	.micdet_delay = 0,
	.gpio_base = WM8903_GPIO_BASE,
	.gpio_cfg = {
		WM8903_GPIO_NO_CONFIG,
		WM8903_GPIO_NO_CONFIG,
		0,                     /* as output pin */
		WM8903_GPn_FN_GPIO_MICBIAS_CURRENT_DETECT
		<< WM8903_GP4_FN_SHIFT, /* as micbias current detect */
		WM8903_GPIO_NO_CONFIG,
	},
};

static struct i2c_board_info __initdata ventana_i2c_bus1_board_info[] = {
	{
		I2C_BOARD_INFO("wm8903", 0x1a),
		.platform_data = &wm8903_pdata,
	},
};

static struct tegra_ulpi_config ventana_ehci2_ulpi_phy_config = {
	.reset_gpio = TEGRA_GPIO_PV1,
	.clk = "clk_dev2",
};

static struct tegra_ehci_platform_data ventana_ehci2_ulpi_platform_data = {
	.operating_mode = TEGRA_USB_HOST,
	.power_down_on_bus_suspend = 1,
	.phy_config = &ventana_ehci2_ulpi_phy_config,
	.phy_type = TEGRA_USB_PHY_TYPE_LINK_ULPI,
};

static struct tegra_i2c_platform_data ventana_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.slave_addr = 0x00FC,
//COMPAL Charles GEN1
	.scl_gpio	= TEGRA_GPIO_PC4,
	.sda_gpio	= TEGRA_GPIO_PC5,
	.arb_recovery = arb_lost_recovery,
//
};

static const struct tegra_pingroup_config i2c2_ddc = {
	.pingroup	= TEGRA_PINGROUP_DDC,
	.func		= TEGRA_MUX_I2C2,
};

static const struct tegra_pingroup_config i2c2_gen2 = {
	.pingroup	= TEGRA_PINGROUP_PTA,
	.func		= TEGRA_MUX_I2C2,
};

static struct tegra_i2c_platform_data ventana_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 2,
	.bus_clk_rate	= { 40000, 100000 },
	.bus_mux	= { &i2c2_ddc, &i2c2_gen2 },
	.bus_mux_len	= { 1, 1 },
	.slave_addr = 0x00FC,
//COMPAL Charles GEN2/DDC
        .scl_gpio       = TEGRA_GPIO_PI5,
        .sda_gpio       = TEGRA_GPIO_PI6,
        .arb_recovery = arb_lost_recovery,
//
};

static struct tegra_i2c_platform_data ventana_i2c3_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.slave_addr = 0x00FC,
//COMPAL Charles, CAM
        .scl_gpio       = TEGRA_GPIO_PBB2,
        .sda_gpio       = TEGRA_GPIO_PBB3,
        .arb_recovery = arb_lost_recovery,
//
};

static struct tegra_i2c_platform_data ventana_dvc_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.is_dvc		= true,
//COMPAL Charles, PWR
        .scl_gpio       = TEGRA_GPIO_PZ6,
        .sda_gpio       = TEGRA_GPIO_PZ7,
        .arb_recovery = arb_lost_recovery,
//
};

static struct tegra_audio_platform_data tegra_audio_pdata[] = {
	/* For I2S1 */
	[0] = {
		.i2s_master	= true,
		.dma_on		= true,  /* use dma by default */
		.i2s_master_clk = 44100,
		.i2s_clk_rate	= 11289600,
		.dap_clk	= "clk_dev1",
		.audio_sync_clk = "audio_2x",
		.mode		= I2S_BIT_FORMAT_I2S,
		.fifo_fmt	= I2S_FIFO_PACKED,
		.bit_size	= I2S_BIT_SIZE_16,
		.i2s_bus_width = 32,
		.dsp_bus_width = 16,
		.en_dmic = false, /* by default analog mic is used */
	},
	/* For I2S2 */
	[1] = {
		.i2s_master	= true,
		.dma_on		= true,  /* use dma by default */
		.i2s_master_clk = 8000,
		.dsp_master_clk = 8000,
		.i2s_clk_rate	= 2000000,
		.dap_clk	= "clk_dev1",
		.audio_sync_clk = "audio_2x",
		.mode		= I2S_BIT_FORMAT_DSP,
		.fifo_fmt	= I2S_FIFO_16_LSB,
		.bit_size	= I2S_BIT_SIZE_16,
		.i2s_bus_width = 32,
		.dsp_bus_width = 16,
	}
};

static struct tegra_das_platform_data tegra_das_pdata = {
	.dap_clk = "clk_dev1",
	.tegra_dap_port_info_table = {
		/* I2S1 <--> DAC1 <--> DAP1 <--> Hifi Codec */
		[0] = {
			.dac_port = tegra_das_port_i2s1,
			.dap_port = tegra_das_port_dap1,
			.codec_type = tegra_audio_codec_type_hifi,
			.device_property = {
				.num_channels = 2,
				.bits_per_sample = 16,
				.rate = 44100,
				.dac_dap_data_comm_format =
						dac_dap_data_format_all,
			},
		},
		[1] = {
			.dac_port = tegra_das_port_none,
			.dap_port = tegra_das_port_none,
			.codec_type = tegra_audio_codec_type_none,
			.device_property = {
				.num_channels = 0,
				.bits_per_sample = 0,
				.rate = 0,
				.dac_dap_data_comm_format = 0,
			},
		},
		[2] = {
			.dac_port = tegra_das_port_none,
			.dap_port = tegra_das_port_none,
			.codec_type = tegra_audio_codec_type_none,
			.device_property = {
				.num_channels = 0,
				.bits_per_sample = 0,
				.rate = 0,
				.dac_dap_data_comm_format = 0,
			},
		},
		/* I2S2 <--> DAC2 <--> DAP4 <--> BT SCO Codec */
		[3] = {
			.dac_port = tegra_das_port_i2s2,
			.dap_port = tegra_das_port_dap4,
			.codec_type = tegra_audio_codec_type_bluetooth,
			.device_property = {
				.num_channels = 1,
				.bits_per_sample = 16,
				.rate = 8000,
				.dac_dap_data_comm_format =
					dac_dap_data_format_dsp,
			},
		},
		[4] = {
			.dac_port = tegra_das_port_none,
			.dap_port = tegra_das_port_none,
			.codec_type = tegra_audio_codec_type_none,
			.device_property = {
				.num_channels = 0,
				.bits_per_sample = 0,
				.rate = 0,
				.dac_dap_data_comm_format = 0,
			},
		},
	},

	.tegra_das_con_table = {
		[0] = {
			.con_id = tegra_das_port_con_id_hifi,
			.num_entries = 2,
			.con_line = {
				[0] = {tegra_das_port_i2s1, tegra_das_port_dap1, true},
				[1] = {tegra_das_port_dap1, tegra_das_port_i2s1, false},
			},
		},
		[1] = {
			.con_id = tegra_das_port_con_id_bt_codec,
			.num_entries = 4,
			.con_line = {
				[0] = {tegra_das_port_i2s2, tegra_das_port_dap4, true},
				[1] = {tegra_das_port_dap4, tegra_das_port_i2s2, false},
				[2] = {tegra_das_port_i2s1, tegra_das_port_dap1, true},
				[3] = {tegra_das_port_dap1, tegra_das_port_i2s1, false},
			},
		},
	}
};

static void ventana_i2c_init(void)
{
	tegra_i2c_device1.dev.platform_data = &ventana_i2c1_platform_data;
	tegra_i2c_device2.dev.platform_data = &ventana_i2c2_platform_data;
	tegra_i2c_device3.dev.platform_data = &ventana_i2c3_platform_data;
	tegra_i2c_device4.dev.platform_data = &ventana_dvc_platform_data;

	i2c_register_board_info(0, ventana_i2c_bus1_board_info, 1);

	platform_device_register(&tegra_i2c_device1);
	platform_device_register(&tegra_i2c_device2);
	platform_device_register(&tegra_i2c_device3);
	platform_device_register(&tegra_i2c_device4);
}


#ifdef CONFIG_KEYBOARD_GPIO
//COMPAL_START
#if 0
#define GPIO_KEY(_id, _gpio, _iswake)		\
	{					\
		.code = _id,			\
		.gpio = TEGRA_GPIO_##_gpio,	\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = 10,	\
	}
#endif

#define GPIO_KEY(_id, _gpio, _isactivelow, _iswake)           \
        {                                       \
                .code = _id,                    \
                .gpio = TEGRA_GPIO_##_gpio,     \
                .active_low = _isactivelow,           \
                .desc = #_id,                   \
                .type = EV_KEY,                 \
                .wakeup = _iswake,              \
                .debounce_interval = 10,        \
        }
//COMPAL_END



static struct gpio_keys_button ventana_keys[] = {
//COMPAL_START
#if 0

	[0] = GPIO_KEY(KEY_FIND, PQ3, 0),
	[1] = GPIO_KEY(KEY_HOME, PQ1, 0),
	[2] = GPIO_KEY(KEY_BACK, PQ2, 0),
	[3] = GPIO_KEY(KEY_VOLUMEUP, PQ5, 0),
	[4] = GPIO_KEY(KEY_VOLUMEDOWN, PQ4, 0),
	[5] = GPIO_KEY(KEY_POWER, PV2, 1),
	[6] = GPIO_KEY(KEY_MENU, PC7, 0),
#endif
	[0] = GPIO_KEY(KEY_VOLUMEUP, PQ5, 1,  0),
    	[1] = GPIO_KEY(KEY_VOLUMEDOWN, PQ4, 1, 0),
   	[2] = GPIO_KEY(KEY_MENU, PC7, 0, 1),
    	[3] = GPIO_KEY(KEY_POWER, PI3, 0, 1),

};

#define PMC_WAKE_STATUS 0x14

static int ventana_wakeup_key(void)
{
	unsigned long status =
		readl(IO_ADDRESS(TEGRA_PMC_BASE) + PMC_WAKE_STATUS);

	//Charles 0722 start, clean wakeup status register
	writel(0xffffffff, IO_ADDRESS(TEGRA_PMC_BASE) + PMC_WAKE_STATUS);

	return status & (TEGRA_WAKE_GPIO_PC7 | TEGRA_WAKE_GPIO_PV3) ? KEY_POWER : KEY_RESERVED;
}

static struct gpio_keys_platform_data ventana_keys_platform_data = {
	.buttons	= ventana_keys,
	.nbuttons	= ARRAY_SIZE(ventana_keys),
	.wakeup_key	= ventana_wakeup_key,
};

static struct platform_device ventana_keys_device = {
	.name	= "gpio-keys",
	.id	= 0,
	.dev	= {
		.platform_data	= &ventana_keys_platform_data,
	},
};

static void ventana_keys_init(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ventana_keys); i++)
		tegra_gpio_enable(ventana_keys[i].gpio);
}
#endif

//COMPAL Chalres
static struct gpio_switch_platform_data dock_switch_platform_data = {
        .gpio = TEGRA_GPIO_PD0,
};

static struct platform_device dock_switch = {
        .name   = "dock",
        .id     = -1,
        .dev    = {
                .platform_data  = &dock_switch_platform_data,
        },
};

// For Rotation Lock
static struct gpio_switch_platform_data rotationlock_switch_platform_data = {
        .gpio = TEGRA_GPIO_PQ2,
};


static struct platform_device rotationlock_switch = {
        .name   = "rotationlock",
        .id     = -1,
        .dev    = {
                .platform_data  = &rotationlock_switch_platform_data,
        },
};

//For Psensor
static struct gpio_switch_platform_data psensor_platform_data = {
        .gpio = TEGRA_GPIO_PC1,
};

static struct platform_device u2_psensor = {
        .name   = "psensor",
        .id     = -1,
        .dev    = {
                .platform_data  = &psensor_platform_data,
        },
};
//COMPAL END


static struct platform_device tegra_camera = {
	.name = "tegra_camera",
	.id = -1,
};

static struct platform_device *ventana_devices[] __initdata = {
	&tegra_uartb_device,
	&tegra_uartc_device,
	&pmu_device,
	&tegra_gart_device,
	&tegra_aes_device,
#ifdef CONFIG_KEYBOARD_GPIO
	&ventana_keys_device,
#endif
	&tegra_wdt_device,
	&tegra_i2s_device1,
	&tegra_i2s_device2,
	&tegra_spdif_device,
	&tegra_avp_device,
	&tegra_camera,
	&tegra_das_device,
    &tegra_spi_device1,

//COMPAL Charles
	&dock_switch,
	&rotationlock_switch,
	&u2_psensor,
//COMPAL END
};


#ifdef CONFIG_TOUCHSCREEN_PANJIT_I2C
static struct panjit_i2c_ts_platform_data panjit_data = {
	.gpio_reset = TEGRA_GPIO_PQ7,
};

static const struct i2c_board_info ventana_i2c_bus1_touch_info[] = {
	{
	 I2C_BOARD_INFO("panjit_touch", 0x3),
	 .irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PV6),
	 .platform_data = &panjit_data,
	 },
};

static int __init ventana_touch_init_panjit(void)
{
	tegra_gpio_enable(TEGRA_GPIO_PV6);

	tegra_gpio_enable(TEGRA_GPIO_PQ7);
	i2c_register_board_info(0, ventana_i2c_bus1_touch_info, 1);

	return 0;
}
#endif

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MT_T9
/* Atmel MaxTouch touchscreen              Driver data */
/*-----------------------------------------------------*/
/*
 * Reads the CHANGELINE state; interrupt is valid if the changeline
 * is low.
 */
 /* COMPAL - START - [2011/3/2  15:14] - Jamin - [Touch]: add hw init function  */
static void ventana_touch_hw_init(void)
{
       tegra_gpio_enable(TEGRA_GPIO_PV6);
	tegra_gpio_enable(TEGRA_GPIO_PQ7);

    if (gpio_request(TEGRA_GPIO_PQ7, "maxTouch_reset") < 0)
    {
        printk("fail to request gpio for touch screen reset pin\n");
    }
   gpio_direction_output(TEGRA_GPIO_PQ7,0);
	//reset touch
	gpio_set_value(TEGRA_GPIO_PQ7, 0);
	msleep(100);
	gpio_set_value(TEGRA_GPIO_PQ7, 1);
	msleep(100);

	//set interrupt pin input
	gpio_request(TEGRA_GPIO_PV6, "maxTouch_interrupt");
	gpio_direction_input(TEGRA_GPIO_PV6);
}
static void ventana_touch_hw_reset(void)
{
       gpio_set_value(TEGRA_GPIO_PQ7, 1);
	msleep(10); 
	gpio_set_value(TEGRA_GPIO_PQ7, 0);
	msleep(100);
	gpio_set_value(TEGRA_GPIO_PQ7, 1);
	msleep(10);
}
static void ventana_touch_hw_exit(void)
{
	gpio_free(TEGRA_GPIO_PQ7);
	gpio_free(TEGRA_GPIO_PV6);
}
/* COMPAL - END */

static u8 read_chg(void)
{
	return gpio_get_value(TEGRA_GPIO_PV6);
}

static u8 read_rst(void)
{
	return gpio_get_value(TEGRA_GPIO_PQ7);
}

static u8 valid_interrupt(void)
{
	return !read_chg();
}

static struct mxt_platform_data Atmel_mxt_info = {
	 /* COMPAL - START - [2011/1/18  20:22] - Jamin - [Touch]: enable touch atmel   */
	/* Maximum number of simultaneous touches to report. */
       //not use  so mark 
	//.numtouch = 10,
	// TODO: no need for any hw-specific things at init/exit?
	//register function for set gpio control atmel touch
	.init_platform_hw = ventana_touch_hw_init,
	.exit_platform_hw = ventana_touch_hw_exit,
	.reset_platform_hw = ventana_touch_hw_reset,
	//set resolution 4096 * 4096
         .max_x = 4096,
        .max_y = 4096,
	 /* COMPAL - END */
	.valid_interrupt = &valid_interrupt,
	.read_chg = &read_chg,
	.read_reset = &read_rst,
};

static struct i2c_board_info __initdata i2c_info[] = {
	{
	 I2C_BOARD_INFO("maXTouch", MXT_I2C_ADDRESS),
	 .irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PV6),
	 .platform_data = &Atmel_mxt_info,
	 },
};

static int __init ventana_touch_init_atmel(void)
{
      /* COMPAL - START - [2011/3/2  15:52] - Jamin - [Touch]: enable atmel touch  */
	//tegra_gpio_enable(TEGRA_GPIO_PQ7);
	//gpio_direction_output(TEGRA_GPIO_PQ7,0);
       /* COMPAL - END */
	i2c_register_board_info(0, i2c_info, 1);

	return 0;
}
#endif

static struct usb_phy_plat_data tegra_usb_phy_pdata[] = {
	[0] = {
			.instance = 0,
			.vbus_irq = TPS6586X_INT_BASE + TPS6586X_INT_USB_DET,
			.vbus_gpio = -1,
	},
	[1] = {
			.instance = 1,
			.vbus_gpio = -1,
	},
	[2] = {
			.instance = 2,
			.vbus_gpio = -1,
	},
};

static struct tegra_ehci_platform_data tegra_ehci_pdata[] = {
	[0] = {
			.phy_config = &utmi_phy_config[0],
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
	},
	[1] = {
			.phy_config = &ulpi_phy_config,
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
			.phy_type = TEGRA_USB_PHY_TYPE_LINK_ULPI,
	},
	[2] = {
			.phy_config = &utmi_phy_config[1],
			.operating_mode = TEGRA_USB_HOST,
			.power_down_on_bus_suspend = 1,
	},
};

static struct platform_device *tegra_usb_otg_host_register(void)
{
	struct platform_device *pdev;
	void *platform_data;
	int val;

	pdev = platform_device_alloc(tegra_ehci1_device.name, tegra_ehci1_device.id);
	if (!pdev)
		return NULL;

	val = platform_device_add_resources(pdev, tegra_ehci1_device.resource,
		tegra_ehci1_device.num_resources);
	if (val)
		goto error;

	pdev->dev.dma_mask =  tegra_ehci1_device.dev.dma_mask;
	pdev->dev.coherent_dma_mask = tegra_ehci1_device.dev.coherent_dma_mask;

	platform_data = kmalloc(sizeof(struct tegra_ehci_platform_data), GFP_KERNEL);
	if (!platform_data)
		goto error;

	memcpy(platform_data, &tegra_ehci_pdata[0],
				sizeof(struct tegra_ehci_platform_data));
	pdev->dev.platform_data = platform_data;

	val = platform_device_add(pdev);
	if (val)
		goto error_add;

	return pdev;

error_add:
	kfree(platform_data);
error:
	pr_err("%s: failed to add the host contoller device\n", __func__);
	platform_device_put(pdev);
	return NULL;
}

static void tegra_usb_otg_host_unregister(struct platform_device *pdev)
{
	kfree(pdev->dev.platform_data);
	pdev->dev.platform_data = NULL;
	platform_device_unregister(pdev);
}

static struct tegra_otg_platform_data tegra_otg_pdata = {
	.host_register = &tegra_usb_otg_host_register,
	.host_unregister = &tegra_usb_otg_host_unregister,
};

static int __init ventana_gps_init(void)
{
	struct clk *clk32 = clk_get_sys(NULL, "blink");
	if (!IS_ERR(clk32)) {
		clk_set_rate(clk32,clk32->parent->rate);
		clk_enable(clk32);
	}

	tegra_gpio_enable(TEGRA_GPIO_PZ3);
	return 0;
}

static void ventana_power_off(void)
{
	int ret;

	ret = tps6586x_power_off();
	if (ret)
		pr_err("ventana: failed to power off\n");

	while(1);
}

static void __init ventana_power_off_init(void)
{
	pm_power_off = ventana_power_off;
}

#define SERIAL_NUMBER_LENGTH 20
static char usb_serial_num[SERIAL_NUMBER_LENGTH];
static void ventana_usb_init(void)
{
	char *src = NULL;
	int i;

	tegra_usb_phy_init(tegra_usb_phy_pdata, ARRAY_SIZE(tegra_usb_phy_pdata));
	/* OTG should be the first to be registered */
	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	platform_device_register(&tegra_usb_fsg_device);
	platform_device_register(&androidusb_device);
	platform_device_register(&tegra_udc_device);
	platform_device_register(&tegra_ehci2_device);

	tegra_ehci3_device.dev.platform_data=&tegra_ehci_pdata[2];
	platform_device_register(&tegra_ehci3_device);

#ifdef CONFIG_USB_ANDROID_RNDIS
	src = usb_serial_num;

	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}
	platform_device_register(&rndis_device);
#endif
}

//+++[ASD2_ES1|Jaco_Gao|2011.04.02] Enable OFN
static struct spi_board_info tegra_spi_ofn_devices[] __initdata = {
    {
        .modalias = "avago-ofn",
        .bus_num = 0,
        .chip_select = 0,
        .mode = SPI_MODE_3,
        .max_speed_hz = 1000000,
        .platform_data = NULL,
		//.irq = TEGRA_GPIO_TO_IRQ(TEGRA_GPIO_PG0),
        .irq = TEGRA_GPIO_PG0,
    },
};
static void __init register_spi_ofn_devices(void)
{
	tegra_gpio_enable(TEGRA_GPIO_PK3);
	tegra_gpio_enable(TEGRA_GPIO_PK2);
	tegra_gpio_enable(TEGRA_GPIO_PJ0);
	tegra_gpio_enable(TEGRA_GPIO_PJ2);
	tegra_gpio_enable(TEGRA_GPIO_PG0);
	tegra_gpio_enable(TEGRA_GPIO_PK4);      //Touch#, FPD
    
	if (spi_register_board_info(tegra_spi_ofn_devices,
		ARRAY_SIZE(tegra_spi_ofn_devices)) != 0)
	{
		pr_err("%s: spi_register_board_info returned error\n", __func__);
	}
}
//+++[ASD2_ES1|Jaco_Gao|2011.04.02] Enable OFN

static void __init tegra_ventana_init(void)
{
#if defined(CONFIG_TOUCHSCREEN_PANJIT_I2C) || \
	defined(CONFIG_TOUCHSCREEN_ATMEL_MT_T9)
	struct board_info BoardInfo;
#endif

	tegra_common_init();
	tegra_clk_init_from_table(ventana_clk_init_table);
	ventana_pinmux_init();
	ventana_i2c_init();
	snprintf(usb_serial_num, sizeof(usb_serial_num), "%llx", tegra_chip_uid());
	andusb_plat.serial_number = kstrdup(usb_serial_num, GFP_KERNEL);
	tegra_i2s_device1.dev.platform_data = &tegra_audio_pdata[0];
	tegra_i2s_device2.dev.platform_data = &tegra_audio_pdata[1];
	tegra_spdif_device.dev.platform_data = &tegra_spdif_pdata;
	if (is_tegra_debug_uartport_hs() == true)
		platform_device_register(&tegra_uartd_device);
	else
		platform_device_register(&debug_uart);
	tegra_das_device.dev.platform_data = &tegra_das_pdata;
	tegra_ehci2_device.dev.platform_data
		= &ventana_ehci2_ulpi_platform_data;
	platform_add_devices(ventana_devices, ARRAY_SIZE(ventana_devices));

	ventana_sdhci_init();
	ventana_charge_init();
	ventana_regulator_init();

#if defined(CONFIG_TOUCHSCREEN_PANJIT_I2C) || \
	defined(CONFIG_TOUCHSCREEN_ATMEL_MT_T9)

	tegra_get_board_info(&BoardInfo);
/* COMPAL - START - [2011/1/24  11:12] - Jamin - [Touch]: enable touch atmel   */
	/* boards with sku > 0 have atmel touch panels */
	//if (BoardInfo.sku) {
		pr_info("Initializing Atmel touch driver\n");
		ventana_touch_init_atmel();
	//} else {
	//	pr_info("Initializing Panjit touch driver\n");
	//	ventana_touch_init_panjit();
	//}
/* COMPAL - END */
#endif

#ifdef CONFIG_KEYBOARD_GPIO
	ventana_keys_init();
#endif
#ifdef CONFIG_KEYBOARD_TEGRA
	ventana_kbc_init();
#endif

	ventana_wired_jack_init();
	ventana_usb_init();
	ventana_gps_init();
	ventana_panel_init();
	ventana_sensors_init();
	ventana_bt_rfkill();
	ventana_power_off_init();
	ventana_emc_init();
#ifdef CONFIG_BT_BLUESLEEP
	tegra_setup_bluesleep();
#endif

    register_spi_ofn_devices();
    
//COMPAL Charles
    //enable gpio for rotation lock
    tegra_gpio_enable(TEGRA_GPIO_PQ2);
    //dock
    tegra_gpio_enable(TEGRA_GPIO_PD0);
    //psensor
    tegra_gpio_enable(TEGRA_GPIO_PC1);
//COMPAL END
}

int __init tegra_ventana_protected_aperture_init(void)
{
	tegra_protected_aperture_init(tegra_grhost_aperture);
	return 0;
}
late_initcall(tegra_ventana_protected_aperture_init);

void __init tegra_ventana_reserve(void)
{
	if (memblock_reserve(0x0, 4096) < 0)
		pr_warn("Cannot reserve first 4K of memory for safety\n");

	tegra_reserve(SZ_256M, SZ_8M, SZ_16M);
}

MACHINE_START(VENTANA, "ventana")
	.boot_params    = 0x00000100,
	.phys_io        = IO_APB_PHYS,
	.io_pg_offst    = ((IO_APB_VIRT) >> 18) & 0xfffc,
	.init_irq       = tegra_init_irq,
	.init_machine   = tegra_ventana_init,
	.map_io         = tegra_map_common_io,
	.reserve        = tegra_ventana_reserve,
	.timer          = &tegra_timer,
MACHINE_END
