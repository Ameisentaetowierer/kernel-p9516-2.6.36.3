/*
 * sound/soc/tegra/tegra_wired_jack.c
 *
 * Copyright (c) 2011, NVIDIA Corporation.
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

#include <linux/types.h>
#include <linux/gpio.h>
#ifdef CONFIG_SWITCH
#include <linux/switch.h>
#endif
#include <linux/notifier.h>
#include <sound/jack.h>
#include <sound/soc.h>
#include <mach/audio.h>

#include "tegra_soc.h"
//compal start willy
#include "../codecs/wm8903.h"
//compal end willy
#define HEAD_DET_GPIO 0
#define MIC_DET_GPIO  1
#define SPK_EN_GPIO   3
//compal start willy
#define S_IRUG  (S_IRUSR|S_IRGRP)
static struct snd_soc_codec *cd;
unsigned long global_action = 0;
EXPORT_SYMBOL( global_action );
//compal end willy


struct wired_jack_conf tegra_wired_jack_conf = {
	-1, -1, -1, -1, 0, NULL, NULL
};

/* Based on hp_gpio and mic_gpio, hp_gpio is active low */
enum {
	HEADSET_WITHOUT_MIC = 0x00,
	HEADSET_WITH_MIC = 0x01,
	NO_DEVICE = 0x10,
	MIC = 0x11,
};

/* These values are copied from WiredAccessoryObserver */
enum headset_state {
	BIT_NO_HEADSET = 0,
	BIT_HEADSET = (1 << 0),
	BIT_HEADSET_NO_MIC = (1 << 1),
};

/* jack */
static struct snd_soc_jack *tegra_wired_jack;

static struct snd_soc_jack_gpio wired_jack_gpios[] = {
	{
		/* gpio pin depends on board traits */
		.name = "headphone-detect-gpio",
		.report = SND_JACK_HEADPHONE,
		.invert = 1,
//compal start willy
		.debounce_time = 300,
//compal end
	},
//compal start willy
#if 0
	{
		/* gpio pin depens on board traits */
		.name = "mic-detect-gpio",
		.report = SND_JACK_MICROPHONE,
		.invert = 1,
		.debounce_time = 200,
	},
#endif
//compal end willy
};

#ifdef CONFIG_SWITCH
static struct switch_dev wired_switch_dev = {
	.name = "h2w",
};

void tegra_switch_set_state(int state)
{
	switch_set_state(&wired_switch_dev, state);
}

static enum headset_state get_headset_state(void)
{
	enum headset_state state = BIT_NO_HEADSET;
	int flag = 0;
	int hp_gpio = -1;
	int mic_gpio = -1;

//compal start willy
	wm8903_mic_detect(cd,tegra_wired_jack,1,1);
///compal end willy

	/* hp_det_n is low active pin */
	if (tegra_wired_jack_conf.hp_det_n != -1)
		hp_gpio = gpio_get_value(tegra_wired_jack_conf.hp_det_n);
	if (tegra_wired_jack_conf.cdc_irq != -1)
		mic_gpio = gpio_get_value(tegra_wired_jack_conf.cdc_irq);

	pr_debug("hp_gpio:%d, mic_gpio:%d\n", hp_gpio, mic_gpio);
//compal start willy
	pr_debug("tegra_wired_jack->status = %d \n",tegra_wired_jack->status);
	if (hp_gpio)
	{
		tegra_wired_jack->status=0;
	}
	else
	{
		tegra_wired_jack->status=1;
	}
//compal end willy
	flag = (hp_gpio << 4) | mic_gpio;

	switch (flag) {
	case NO_DEVICE:
		state = BIT_NO_HEADSET;
		break;
	case HEADSET_WITH_MIC:
		state = BIT_HEADSET;
		break;
	case MIC:
		/* mic: would not report */
		break;
	case HEADSET_WITHOUT_MIC:
		state = BIT_HEADSET_NO_MIC;
		break;
	default:
		state = BIT_NO_HEADSET;
	}
//compal start willy
        int i,mic_status = 0,mic_count = 0;
        for(i=1;i<=10;i++){
                snd_soc_update_bits(cd, WM8903_INTERRUPT_POLARITY_1,
                        (WM8903_MICDET_INV | WM8903_MICSHRT_INV),(WM8903_MICDET_INV | WM8903_MICSHRT_INV));
                msleep(5);
                mic_status = snd_soc_read(cd, WM8903_INTERRUPT_STATUS_1);
                pr_debug("mic_event mic_status = %d \n",mic_status);
                snd_soc_update_bits(cd, WM8903_INTERRUPT_POLARITY_1,
                        (WM8903_MICDET_INV | WM8903_MICSHRT_INV),~(WM8903_MICDET_INV | WM8903_MICSHRT_INV));
                msleep(5);
                mic_status = ( snd_soc_read(cd, WM8903_INTERRUPT_STATUS_1) & 0xf000 ) >> 12;
                pr_debug("mic_event mic_status 0xf000 = %d \n",mic_status);
                if(mic_status & 0xc)
                       if(++mic_count > 6)
                                break;
        }
        if(mic_count > 6){
                pr_debug("Headphone inserted\n");
		snd_soc_write(cd, WM8903_CLOCK_RATE_TEST_4, 0x2a38);
                if(flag == BIT_NO_HEADSET)
                        global_action = 1;
                else
                        global_action = 0;
        }else{
                pr_debug("Headset inserted\n");
                pr_debug("flag = %d \n",flag);
                if(flag == BIT_NO_HEADSET)
                {
                        snd_soc_write(cd, WM8903_CLOCK_RATE_TEST_4, 0x2838);
                        global_action = 2;
			//snd_soc_write(cd, WM8903_POWER_MANAGEMENT_2,0x3);
                }
                else
                {
                        snd_soc_write(cd, WM8903_CLOCK_RATE_TEST_4, 0x2a38);
                        global_action = 0;
			//snd_soc_write(cd, WM8903_POWER_MANAGEMENT_3,0x3);
                }
        }
	if (global_action != 2)
		snd_soc_write(cd, WM8903_MIC_BIAS_CONTROL_0, 0x16);
	pr_debug("global_action = %d \n",global_action);
	pr_debug("state = %d \n",state);
//compal end willy
	return state;
}

static int wired_switch_notify(struct notifier_block *self,
			      unsigned long action, void* dev)
{
	tegra_switch_set_state(get_headset_state());

	return NOTIFY_OK;
}

void tegra_jack_suspend(void)
{
	snd_soc_jack_free_gpios(tegra_wired_jack,
				ARRAY_SIZE(wired_jack_gpios),
				wired_jack_gpios);
}

void tegra_jack_resume(void)
{
	snd_soc_jack_add_gpios(tegra_wired_jack,
				     ARRAY_SIZE(wired_jack_gpios),
				     wired_jack_gpios);
}

static struct notifier_block wired_switch_nb = {
	.notifier_call = wired_switch_notify,
};
#endif
//compal start willy
static ssize_t global_action_show(struct device *dev,struct device_attribute *attr, char *buf)
{
	int ret;
	pr_debug("sysfs global_action = %d \n",global_action);
	ret = (int)global_action;
	pr_debug("ret = %d \n",ret);
	return sprintf(buf, "%d\n", ret);;
}
static DEVICE_ATTR(global_action,0444, global_action_show, NULL);
static struct kobject *global_action_kobj;
static struct attribute *attributes[] = {
	&dev_attr_global_action.attr,
	NULL,
};
static struct attribute_group attribute_group = {
	.attrs = attributes,
};
//compal end willy


/* platform driver */
static int tegra_wired_jack_probe(struct platform_device *pdev)
{
	int ret;
	int hp_det_n, cdc_irq;
	int en_mic_int, en_mic_ext;
	int en_spkr;
	struct tegra_wired_jack_conf *pdata;

	pdata = (struct tegra_wired_jack_conf *)pdev->dev.platform_data;
	if (!pdata || !pdata->hp_det_n || !pdata->en_spkr ||
	    !pdata->cdc_irq || !pdata->en_mic_int || !pdata->en_mic_ext) {
		pr_err("Please set up gpio pins for jack.\n");
		return -EBUSY;
	}

	hp_det_n = pdata->hp_det_n;
	wired_jack_gpios[HEAD_DET_GPIO].gpio = hp_det_n;
//compal start willy
#if 0
	cdc_irq = pdata->cdc_irq;
	wired_jack_gpios[MIC_DET_GPIO].gpio = cdc_irq;
#endif
//compal end willy
	ret = snd_soc_jack_add_gpios(tegra_wired_jack,
				     ARRAY_SIZE(wired_jack_gpios),
				     wired_jack_gpios);
	if (ret) {
		pr_err("Could NOT set up gpio pins for jack.\n");
		snd_soc_jack_free_gpios(tegra_wired_jack,
					ARRAY_SIZE(wired_jack_gpios),
					wired_jack_gpios);
		return ret;
	}

	/* Mic switch controlling pins */
	en_mic_int = pdata->en_mic_int;
	en_mic_ext = pdata->en_mic_ext;

	ret = gpio_request(en_mic_int, "en_mic_int");
	if (ret) {
		pr_err("Could NOT get gpio for internal mic controlling.\n");
		gpio_free(en_mic_int);
	}
	gpio_direction_output(en_mic_int, 0);
	gpio_export(en_mic_int, false);

	ret = gpio_request(en_mic_ext, "en_mic_ext");
	if (ret) {
		pr_err("Could NOT get gpio for external mic controlling.\n");
		gpio_free(en_mic_ext);
	}
	gpio_direction_output(en_mic_ext, 0);
	gpio_export(en_mic_ext, false);

	en_spkr = pdata->en_spkr;
	ret = gpio_request(en_spkr, "en_spkr");
	if (ret) {
		pr_err("Could NOT set up gpio pin for amplifier.\n");
		gpio_free(en_spkr);
	}
	gpio_direction_output(en_spkr, 0);
	gpio_export(en_spkr, false);

	if (pdata->spkr_amp_reg)
		tegra_wired_jack_conf.amp_reg =
			regulator_get(NULL, pdata->spkr_amp_reg);
	tegra_wired_jack_conf.amp_reg_enabled = 0;

	/* restore configuration of these pins */
	tegra_wired_jack_conf.hp_det_n = hp_det_n;
	tegra_wired_jack_conf.en_mic_int = en_mic_int;
	tegra_wired_jack_conf.en_mic_ext = en_mic_ext;
	tegra_wired_jack_conf.cdc_irq = cdc_irq;
	tegra_wired_jack_conf.en_spkr = en_spkr;

	// Communicate the jack connection state at device bootup
	tegra_switch_set_state(get_headset_state());

#ifdef CONFIG_SWITCH
	snd_soc_jack_notifier_register(tegra_wired_jack,
				       &wired_switch_nb);
#endif
	return ret;
}

static int tegra_wired_jack_remove(struct platform_device *pdev)
{
	snd_soc_jack_free_gpios(tegra_wired_jack,
				ARRAY_SIZE(wired_jack_gpios),
				wired_jack_gpios);

	gpio_free(tegra_wired_jack_conf.en_mic_int);
	gpio_free(tegra_wired_jack_conf.en_mic_ext);
	gpio_free(tegra_wired_jack_conf.en_spkr);
	gpio_free(tegra_wired_jack_conf.cdc_irq);

	if (tegra_wired_jack_conf.amp_reg) {
		if (tegra_wired_jack_conf.amp_reg_enabled)
			regulator_disable(tegra_wired_jack_conf.amp_reg);
		regulator_put(tegra_wired_jack_conf.amp_reg);
	}

	return 0;
}

static struct platform_driver tegra_wired_jack_driver = {
	.probe = tegra_wired_jack_probe,
	.remove = tegra_wired_jack_remove,
	.driver = {
		.name = "tegra_wired_jack",
		.owner = THIS_MODULE,
	},
};


int tegra_jack_init(struct snd_soc_codec *codec)
{
	int ret;
//compal start willy
	cd = codec;
//compal end willy
	if (!codec)
		return -1;

	tegra_wired_jack = kzalloc(sizeof(*tegra_wired_jack), GFP_KERNEL);
	if (!tegra_wired_jack) {
		pr_err("failed to allocate tegra_wired_jack \n");
		return -ENOMEM;
	}

	/* Add jack detection */
	ret = snd_soc_jack_new(codec->socdev->card, "Wired Accessory Jack",
			       SND_JACK_HEADSET, tegra_wired_jack);
	if (ret < 0)
		goto failed;

#ifdef CONFIG_SWITCH
	/* Addd h2w swith class support */
	ret = switch_dev_register(&wired_switch_dev);
	if (ret < 0)
		goto switch_dev_failed;
#endif
//compal start willy
	global_action_kobj = kobject_create_and_add("mic", NULL);
	if (global_action_kobj == NULL) {
		pr_err("global_action_sysfs_init: subsystem_register failed\n");
 		ret = -ENOMEM;
	}
	ret = sysfs_create_group( global_action_kobj, &attribute_group);
	if (ret) {
		pr_err("%s: sysfs_create_group failed\n", __func__);
	}
//compal end willy


	ret = platform_driver_register(&tegra_wired_jack_driver);
	if (ret < 0)
		goto platform_dev_failed;

	return 0;

#ifdef CONFIG_SWITCH
switch_dev_failed:
	switch_dev_unregister(&wired_switch_dev);
#endif
platform_dev_failed:
	platform_driver_unregister(&tegra_wired_jack_driver);
failed:
	if (tegra_wired_jack) {
		kfree(tegra_wired_jack);
		tegra_wired_jack = 0;
	}
	return ret;
}

void tegra_jack_exit(void)
{
#ifdef CONFIG_SWITCH
	switch_dev_unregister(&wired_switch_dev);
#endif
	platform_driver_unregister(&tegra_wired_jack_driver);

	if (tegra_wired_jack) {
		kfree(tegra_wired_jack);
		tegra_wired_jack = 0;
	}
}
