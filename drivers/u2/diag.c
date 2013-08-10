#include <linux/module.h>
#include <linux/fs.h>
#include <linux/dmi.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>

#include "../../arch/arm/mach-tegra/gpio-names.h"
#include "../../arch/arm/mach-tegra/include/mach/gpio.h"

#define TRACE_DIAG 1

#if TRACE_DIAG
#define DIAG(x...) printk(KERN_INFO "[pbj20_diag] " x)
#else
#define DIAG(x...) do {} while (0)
#endif

static struct kobject *diag_kobj;

static ssize_t sku_show(struct kobject*, struct kobj_attribute*, char*);
static ssize_t sku_store(struct kobject*, struct kobj_attribute*, char*);

#define DIAG_ATTR(_name) \
         static struct kobj_attribute _name##_attr = { \
         .attr = { .name = __stringify(_name), .mode = S_IRUGO|S_IWUGO}, \
         .show = _name##_show, \
         .store = _name##_store, \
         }

DIAG_ATTR(sku);

static struct attribute * attributes[] = {
	&sku_attr.attr,
        NULL,
};

static struct attribute_group attribute_group = {
        .attrs = attributes,
};

/*
 * A6 = GPIO_PQ0 [ ('q' - 'a') * 8 + 0 = 128 ]
 * B5 = GPIO_PQ3 [ ('q' - 'a') * 8 + 3 = 131 ]
 * A3 = GPIO_PQ6 [ ('q' - 'a') * 8 + 6 = 134 ]
 *
 */

//sku start
static ssize_t sku_show(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf)
{
    int A6pinValue = 2;
    int B5pinValue = 2;
    int A3pinValue = 2;

    unsigned A6_gpio_num = TEGRA_GPIO_PQ0;
    unsigned B5_gpio_num = TEGRA_GPIO_PQ3;
    unsigned A3_gpio_num = TEGRA_GPIO_PQ6;

    int ret = -1;
    int sku_num = 0;

    // A6
    tegra_gpio_enable(A6_gpio_num);
    ret = gpio_request(A6_gpio_num, "A6_GPIO_PQ0");
    if (ret < 0)
        goto a6_err_request_gpio;

    ret = gpio_direction_input(A6_gpio_num);
    if (ret < 0)
        goto a6_err_set_gpio_input;

    A6pinValue = gpio_get_value(A6_gpio_num);

    // B5
    tegra_gpio_enable(B5_gpio_num);
    ret = gpio_request(B5_gpio_num, "B5_GPIO_PQ0");
    if (ret < 0)
        goto b5_err_request_gpio;

    ret = gpio_direction_input(B5_gpio_num);
    if (ret < 0)
        goto b5_err_set_gpio_input;

    B5pinValue = gpio_get_value(B5_gpio_num);

    // A3
    tegra_gpio_enable(A3_gpio_num);
    ret = gpio_request(A3_gpio_num, "A3_GPIO_PQ0");
    if (ret < 0)
        goto a3_err_request_gpio;

    ret = gpio_direction_input(A3_gpio_num);
    if (ret < 0)
        goto a3_err_set_gpio_input;

    A3pinValue = gpio_get_value(A3_gpio_num);

a3_err_set_gpio_input:
    gpio_free(A3_gpio_num);

a3_err_request_gpio:
b5_err_set_gpio_input:
    tegra_gpio_disable(A3_gpio_num);
    gpio_free(B5_gpio_num);

b5_err_request_gpio:
a6_err_set_gpio_input:
    tegra_gpio_disable(B5_gpio_num);
    gpio_free(A6_gpio_num);

a6_err_request_gpio:
    tegra_gpio_disable(A6_gpio_num);

    if(A6pinValue == 2 || B5pinValue == 2 || A3pinValue == 2)
	return sprintf(buf, "someting wrong, see dmesg\n");

    if(A6pinValue == 1)
	sku_num += 4;
    if(B5pinValue == 1)
	sku_num += 2;
    if(A3pinValue == 1)
	sku_num += 1;

    return sprintf(buf, "%d\n", sku_num);
}

static ssize_t sku_store(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf)
{
	return 0;
}
//sku end

extern void SysShutdown(void);
extern void SysRestart(void);

static void pbj20_pm_power_off(void)
{
    DIAG("SysShutdown\n");
    SysShutdown();
}

static void pbj20_pm_reset(void)
{
    DIAG("SysRestart\n");
    SysRestart();
}

static int __init pbj20_diag_init(void)
{
    int err;

    diag_kobj = kobject_create_and_add("diag", NULL);
    if (diag_kobj == NULL){
        DIAG("lcd_edid subsystem_register failed\n");
	goto err_diag;
    }

    err = sysfs_create_group(diag_kobj, &attribute_group);
    if(err){
        DIAG("sysfs_create failed, \n");
	goto err_diag;
    }

    DIAG("pm_power_off = pbj20_pm_power_off\n");
    DIAG("arm_pm_restart = pbj20_pm_reset\n");
    pm_power_off = pbj20_pm_power_off;
    arm_pm_restart = pbj20_pm_reset;

    DIAG("PBJ20 diag Driver Initialized\n");

    return 0;

err_diag:
    kobject_del(diag_kobj);
    return -ENODEV;
}

static void __exit pbj20_diag_exit(void)
{
}

module_init(pbj20_diag_init);
module_exit(pbj20_diag_exit);
