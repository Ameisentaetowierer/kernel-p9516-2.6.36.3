#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <linux/platform_device.h>
#include <linux/switch.h>
#include <linux/slab.h>
#include <linux/gpio.h>   
#include <linux/interrupt.h> 

#define DRIVER_NAME     "h2w" // device name for Android Headset Observer

struct headphone_switch_data {
    struct switch_dev sdev;
    unsigned gpio;   
    unsigned irq;     
    struct work_struct work;
};
//COMPAL_START
struct headphone_switch_data *switch_data;
//COMPAL_END

static ssize_t switch_print_name(struct switch_dev *sdev, char *buf)
{
    return sprintf(buf, "%s\n", DRIVER_NAME);
}

static ssize_t switch_print_state(struct switch_dev *sdev, char *buf)
{
    return sprintf(buf, "%s\n", (switch_get_state(sdev) ? "1" : "0"));
}


static void headphone_switch_work(struct work_struct *work)
{
    int state;
    struct headphone_switch_data *pSwitch =
            container_of(work, struct headphone_switch_data, work);

    state = gpio_get_value(pSwitch->gpio);
    
    state = ((state+1) % 2);
    
    switch_set_state(&pSwitch->sdev, state);
}

#ifdef CONFIG_I_LOVE_PBJ30
void headphone_event(void)
{
    int state;

    state = gpio_get_value(switch_data->gpio);
    
    state = ((state+1) % 2);
    
    switch_set_state(&switch_data->sdev, state);
}
EXPORT_SYMBOL_GPL(headphone_event);
#endif

static irqreturn_t headphone_interrupt(int irq, void *dev_id)
{
    struct headphone_switch_data *pSwitch = 
            (struct headphone_switch_data *)dev_id;
    
    schedule_work(&pSwitch->work);

    return IRQ_HANDLED;
}

static int headphone_switch_probe(struct platform_device *pdev)
{
    struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;  

//COMPAL_START
#if 0
    struct headphone_switch_data *switch_data;
#endif
//COMPAL_END

    int ret = -EBUSY;

    //if (!pdata)
    //    return -EBUSY;
	
    switch_data = kzalloc(sizeof(struct headphone_switch_data), GFP_KERNEL);
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
	
    INIT_WORK(&switch_data->work, headphone_switch_work);
    
#ifdef CONFIG_I_LOVE_PBJ30
	//pbj30 not use
#else
    ret = request_irq(switch_data->irq, headphone_interrupt, 
              IRQF_DISABLED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
              DRIVER_NAME, switch_data);
    if (ret) {
        printk("headphone_switch request irq failed\n");
        goto err_request_irq;
    }
#endif

    // set current status
    headphone_switch_work(&switch_data->work);
    return 0;

err_request_irq:
    free_irq(switch_data->irq, switch_data);   
err_register_switch:
    kfree(switch_data);
    return ret;
}

static int __devexit headphone_switch_remove(struct platform_device *pdev)
{
    struct headphone_switch_data *switch_data = platform_get_drvdata(pdev);
	
    cancel_work_sync(&switch_data->work);
    switch_dev_unregister(&switch_data->sdev);
    kfree(switch_data);
    
    return 0;
}

static int headphone_switch_suspend(struct platform_device *pdev, 
                                    pm_message_t state)
{
    return 0;
}

static int headphone_switch_resume(struct platform_device *pdev)
{
//COMPAL_START
    printk(KERN_INFO"Headphone Detection Module Resume\n");	
    int state;
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
    
    state = gpio_get_value(switch_data->gpio);
    state = ((state+1) % 2);
    
    switch_set_state(&switch_data->sdev, state);
//COMPAL_END
    
    return 0;
}

static struct platform_driver headphone_switch_driver = {
    .probe      = headphone_switch_probe,
    .remove     = __devexit_p(headphone_switch_remove),
    .suspend    = headphone_switch_suspend,
    .resume     = headphone_switch_resume, 
    .driver     = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
    },
};

static int __init headphone_switch_init(void)
{
    int err;

    printk(KERN_INFO "Headphone Switch driver init ----->\n");
    
    err = platform_driver_register(&headphone_switch_driver);
    if (err)
        goto err_exit;

    printk(KERN_INFO "Headphone Switch register OK ----->>>\n");
    return 0;

err_exit:
    printk(KERN_INFO "Headphone Switch register Failed! ----->>>\n");
    return err;
}

static void __exit headphone_switch_exit(void)
{
    printk(KERN_INFO "Headphone Switch driver unregister ----->>>\n");
    platform_driver_unregister(&headphone_switch_driver);
}

module_init(headphone_switch_init);
module_exit(headphone_switch_exit);

MODULE_DESCRIPTION("PBJ20 Headphone Switch Driver");
MODULE_LICENSE("GPL");
