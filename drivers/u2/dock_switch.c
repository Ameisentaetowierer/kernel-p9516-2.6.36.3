#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/slab.h>
#include <linux/gpio.h>   
#include <linux/interrupt.h> 

#define DRIVER_NAME     "dock"

struct dock_switch_data {
    struct switch_dev sdev;
    unsigned gpio;   
    unsigned irq;     
    struct work_struct work;
};

static ssize_t switch_print_name(struct switch_dev *sdev, char *buf)                                  
{
    return sprintf(buf, "%s\n", DRIVER_NAME);
}

static ssize_t switch_print_state(struct switch_dev *sdev, char *buf)
{
    return sprintf(buf, "%s\n", (switch_get_state(sdev) ? "1" : "0"));
}
//Compal Start William 2011-08-26, add zhaoqian's USB3(Host) docking patch
//zhaoq
extern void enable_usb3 (void);
extern void disable_usb3 (void);
static int old_state=0;
//end
//Compal End William 2011-08-26
static void dock_switch_work(struct work_struct *work)
{
    int state;
    struct dock_switch_data *pSwitch =
               container_of(work, struct dock_switch_data, work);

    state = gpio_get_value(pSwitch->gpio);
//Compal Start William 2011-08-26, add zhaoqian's USB3(Host) docking patch
    printk(" dock_switch, gpio is %x\n", state);
/*because plug in host dock,  triger PlugAction_val to changed, so add enable usb3 in here temprately by poll PlugAction_val.
the right way is mcu send irq to arm to inform dock plug in ,need compal to add this function in mcu,20110608 zhaoq*/
	if(state !=  old_state){
		old_state = state;
		if( state == 1)
			enable_usb3();
		else
			disable_usb3();
	}
//end zhaoq
//Compal End William 2011-08-26
    switch_set_state(&pSwitch->sdev, state);
}


static irqreturn_t dock_interrupt(int irq, void *dev_id)
{
    struct dock_switch_data *pSwitch = 
               (struct dock_switch_data *)dev_id;
    
    schedule_work(&pSwitch->work);

    return IRQ_HANDLED;
}

static int dock_switch_probe(struct platform_device *pdev)
{
    struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;  
    struct dock_switch_data *switch_data;
    int ret = -EBUSY;

    if (!pdata)
        return -EBUSY;
	
    switch_data = kzalloc(sizeof(struct dock_switch_data), GFP_KERNEL);
    if (!switch_data)
        return -ENOMEM;

    switch_data->gpio = pdata->gpio;                  
    switch_data->irq = gpio_to_irq(pdata->gpio);       
    switch_data->sdev.print_state = switch_print_state;
    switch_data->sdev.name = DRIVER_NAME;
    switch_data->sdev.print_name = switch_print_name;
    switch_data->sdev.print_state = switch_print_state;
    ret = switch_dev_register(&switch_data->sdev);
    if (ret < 0)
        goto err_register_switch;
	
    INIT_WORK(&switch_data->work, dock_switch_work);

    ret = request_irq(switch_data->irq, dock_interrupt, 
                  IRQF_DISABLED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                  DRIVER_NAME, switch_data);
    if (ret) {
        printk("dock_switch request irq failed\n");
        goto err_request_irq;
    }

    // set current status
    dock_switch_work(&switch_data->work);
    return 0;

err_request_irq:
    free_irq(switch_data->irq, switch_data);   
err_register_switch:
    kfree(switch_data);
    return ret;
}

static int __devexit dock_switch_remove(struct platform_device *pdev)
{
    struct dock_switch_data *switch_data = platform_get_drvdata(pdev);
	
    cancel_work_sync(&switch_data->work);
    switch_dev_unregister(&switch_data->sdev);
    kfree(switch_data);
    
    return 0;
}

static int dock_switch_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}

static int dock_switch_resume(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver dock_switch_driver = {
    .probe      = dock_switch_probe,
    .remove     = __devexit_p(dock_switch_remove),
    .suspend    = dock_switch_suspend,
    .resume     = dock_switch_resume, 
    .driver     = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
    },
};

static int __init dock_switch_init(void)
{
    int err;

    printk(KERN_INFO "Dock Switch driver init ----->\n");
    
    err = platform_driver_register(&dock_switch_driver);
    if (err)
        goto err_exit;

    printk(KERN_INFO "Dock Switch register OK ----->>>\n");
    return 0;

err_exit:
    printk(KERN_INFO "Dock Switch register Failed! ----->>>\n");
    return err;
}

static void __exit dock_switch_exit(void)
{
    printk(KERN_INFO "Dock Switch driver unregister ----->>>\n");
    platform_driver_unregister(&dock_switch_driver);
}

module_init(dock_switch_init);
module_exit(dock_switch_exit);

MODULE_DESCRIPTION("Dock Switch Driver");
MODULE_LICENSE("GPL");
