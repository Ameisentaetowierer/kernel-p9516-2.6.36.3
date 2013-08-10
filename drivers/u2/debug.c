#include <linux/module.h>
#include <linux/io.h>
#include <mach/iomap.h>
#include <linux/interrupt.h>

#include "pmc_regs.h"
#include "emc_regs.h"
#ifdef CONFIG_I_LOVE_PBJ20
#include "sdmmc_regs.h"
static void __iomem *sdmmc3 = (void __iomem *)0xd801e400;
#endif



static struct kobject *dump_reg_kobj;

static ssize_t dump_reg_show(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf);
static ssize_t dump_reg_store(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf, size_t n);
static ssize_t debug_show(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf);
static ssize_t debug_store(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf, size_t n);
static ssize_t log_show(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf);
static ssize_t log_store(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf, size_t n);

	
#define DEBUG_ATTR(_name) \
         static struct kobj_attribute _name##_attr = { \
         .attr = { .name = __stringify(_name), .mode = S_IRUGO|S_IWUGO}, \
         .show = _name##_show, \
         .store = _name##_store, \
         }

DEBUG_ATTR(dump_reg);
DEBUG_ATTR(debug);
DEBUG_ATTR(log);

static struct attribute * attributes[] = { 
	&dump_reg_attr.attr,
	&debug_attr.attr,
	&log_attr.attr,
        NULL,
};

static struct attribute_group attribute_group = { 
        .attrs = attributes,
};

#define VAR_PREFIX "g_"

unsigned long g_temp1;
EXPORT_SYMBOL(g_temp1);

extern unsigned long kallsyms_lookup_name(const char *name);

#define IRQ_ENABLE "irqe"
#define IRQ_DISABLE "irqd"
#define READ_DEBUG_PREFIX "read_"
#define WRITE_DEBUG_PREFIX "write_"
#ifdef CONFIG_I_LOVE_PBJ20
#define RESET_SDCARD_CON "reset_sd"
#endif

static ssize_t debug_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char * s = buf;
	s += sprintf(s, "test debug\n");
	s += sprintf(s, "echo reset_sd > /sys/debug/debug, for reset sdcard con\n");
	return (s - buf);
}


static ssize_t debug_store(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf, size_t n)
{
	if(n >= strlen(IRQ_ENABLE) && strncmp(buf, IRQ_ENABLE, strlen(IRQ_ENABLE)) == 0) 
	{
		printk("[%s] %s\n", __FUNCTION__, buf);
		{
			char *temp;
			int irq;
			temp = buf + strlen(IRQ_ENABLE);
			irq = simple_strtol(temp, (char **) NULL, 10);
			printk("enable_irq %d\n", irq);
			enable_irq(irq);
		}
		return n;
	}

	if(n >= strlen(IRQ_DISABLE) && strncmp(buf, IRQ_DISABLE, strlen(IRQ_DISABLE)) == 0) 
	{
		printk("[%s] %s\n", __FUNCTION__, buf);
		{
			char *temp;
			int irq;
			temp = buf + strlen(IRQ_DISABLE);
			irq = simple_strtol(temp, (char **) NULL, 10);
			printk("disable_irq %d\n", irq);
			disable_irq(irq);
		}
		return n;
	}

	if(n >= strlen(READ_DEBUG_PREFIX) && strncmp(buf, READ_DEBUG_PREFIX, strlen(READ_DEBUG_PREFIX)) == 0) 
	{
		//example, echo read_temp1 > debug, for reading global variable: g_temp1
		char temp[30];

		printk("[%s]\t\t%s\n", __FUNCTION__, buf);
		memset(temp, 0, 30);
		temp[0] = 'g';
		temp[1] = '_';
		memcpy(temp+2, buf+strlen(READ_DEBUG_PREFIX), strlen(buf+strlen(READ_DEBUG_PREFIX)));
		printk("kallsyms_lookup_name(%s) = %ld\n", temp, kallsyms_lookup_name(temp));
		printk("kallsyms_lookup_name(%s) hardcode = %ld\n", "g_temp1", kallsyms_lookup_name(temp));
		return n;
	}

#ifdef CONFIG_I_LOVE_PBJ20
	if(n >= strlen(RESET_SDCARD_CON) && strncmp(buf,RESET_SDCARD_CON, strlen(RESET_SDCARD_CON)) == 0) 
	{
		printk("[%s]\t\t%s\n", __FUNCTION__, buf);
		//call sdmmc reset con here
		writeb(0x1, sdmmc3 + 0x2f);
		return n;
	}
#endif

	printk("wrong parameters\n");
	return n;
}


static ssize_t dump_reg_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char * s = buf;
	s += sprintf(s, "test\n");
	return (s - buf);
}

static void __iomem *pmc = IO_ADDRESS(TEGRA_PMC_BASE);
#define print_pmc_reg(X) printk("  %03xh %s\t0x%08x\n", X, #X, readl(pmc + X))

static void dump_pmc_reg(void){

	printk("********TEGRA_PMC_BASE PHYS=0x%x VIRT=0x%x***********\n", TEGRA_PMC_BASE, (unsigned int)IO_ADDRESS(TEGRA_PMC_BASE));
	print_pmc_reg(PMC_CTRL);
	print_pmc_reg(PMC_SEC_DISABLE);
	print_pmc_reg(PMC_PMC_SWRST);
	print_pmc_reg(PMC_WAKE_MASK);
	print_pmc_reg(PMC_WAKE_LEVEL);
	print_pmc_reg(PMC_WAKE_STATUS);
	print_pmc_reg(PMC_PMC_SW_WAKE_STATUS);
	print_pmc_reg(PMC_DPAD_ORIDE);	
	print_pmc_reg(PMC_DPD_SAMPLE);  
	print_pmc_reg(PMC_DPD_ENABLE);	
	print_pmc_reg(PMC_PWRGATE_TIMER_OFF);
	print_pmc_reg(PMC_PWRGATE_TIMER_ON);
	print_pmc_reg(PMC_PWRGATE_TOGGLE);
	print_pmc_reg(PMC_REMOVE_CLAMPING_CMD);
	print_pmc_reg(PMC_PWRGATE_STATUS);
	print_pmc_reg(PMC_PWRGOOD_TIMER);
	print_pmc_reg(PMC_BLINK_TIMER);
	print_pmc_reg(PMC_NO_IOPOWER);
	print_pmc_reg(PMC_PWR_DET);
	print_pmc_reg(PMC_PWR_DET_LATCH);
	print_pmc_reg(PMC_SCRATCH0);
	print_pmc_reg(PMC_SCRATCH1);
	print_pmc_reg(PMC_SCRATCH2);
	print_pmc_reg(PMC_SCRATCH3);
	print_pmc_reg(PMC_SCRATCH4);
	print_pmc_reg(PMC_SCRATCH5);
	print_pmc_reg(PMC_SCRATCH6);
	print_pmc_reg(PMC_SCRATCH7);
	print_pmc_reg(PMC_SCRATCH8);
	print_pmc_reg(PMC_SCRATCH9);
	print_pmc_reg(PMC_SCRATCH10);
	print_pmc_reg(PMC_SCRATCH11);
	print_pmc_reg(PMC_SCRATCH12);
	print_pmc_reg(PMC_SCRATCH13);
	print_pmc_reg(PMC_SCRATCH14);
	print_pmc_reg(PMC_SCRATCH15);
	print_pmc_reg(PMC_SCRATCH16);
	print_pmc_reg(PMC_SCRATCH17);
	print_pmc_reg(PMC_SCRATCH18);
	print_pmc_reg(PMC_SCRATCH19);
	print_pmc_reg(PMC_SCRATCH20);
	print_pmc_reg(PMC_SCRATCH21);
	print_pmc_reg(PMC_SCRATCH22);
	print_pmc_reg(PMC_SCRATCH23);
	print_pmc_reg(PMC_SECURE_SCRATCH0);
	print_pmc_reg(PMC_SECURE_SCRATCH1);
	print_pmc_reg(PMC_SECURE_SCRATCH2);
	print_pmc_reg(PMC_SECURE_SCRATCH3);
	print_pmc_reg(PMC_SECURE_SCRATCH4);
	print_pmc_reg(PMC_SECURE_SCRATCH5);
	print_pmc_reg(PMC_CPUPWRGOOD_TIMER);
	print_pmc_reg(PMC_CPUPWROFF_TIMER);
	print_pmc_reg(PMC_PG_MASK);
	print_pmc_reg(PMC_PG_MASK_1);
	print_pmc_reg(PMC_AUTO_WAKE_LVL);
	print_pmc_reg(PMC_AUTO_WAKE_LVL_MASK);
	print_pmc_reg(PMC_WAKE_DELAY);
	print_pmc_reg(PMC_PWR_DET_VAL);
	print_pmc_reg(PMC_DDR_PWR);
	print_pmc_reg(PMC_USB_DEBOUNCE_DEL);
	print_pmc_reg(PMC_USB_AO);
	print_pmc_reg(PMC_CRYPTO_OP);
	print_pmc_reg(PMC_PLLP_WB0_OVERRIDE);
	print_pmc_reg(PMC_SCRATCH24);
	print_pmc_reg(PMC_SCRATCH25);
	print_pmc_reg(PMC_SCRATCH26);
	print_pmc_reg(PMC_SCRATCH27);
	print_pmc_reg(PMC_SCRATCH28);
	print_pmc_reg(PMC_SCRATCH29);
	print_pmc_reg(PMC_SCRATCH30);
	print_pmc_reg(PMC_SCRATCH31);
	print_pmc_reg(PMC_SCRATCH32);
	print_pmc_reg(PMC_SCRATCH33);
	print_pmc_reg(PMC_SCRATCH34);
	print_pmc_reg(PMC_SCRATCH35);
	print_pmc_reg(PMC_SCRATCH36);
	print_pmc_reg(PMC_SCRATCH37);
	print_pmc_reg(PMC_SCRATCH38);
	print_pmc_reg(PMC_SCRATCH39);
	print_pmc_reg(PMC_SCRATCH40);
	print_pmc_reg(PMC_SCRATCH41);
	print_pmc_reg(PMC_SCRATCH42);
	print_pmc_reg(PMC_BONDOUT_MIRROR0);
	print_pmc_reg(PMC_BONDOUT_MIRROR1);
	print_pmc_reg(PMC_BONDOUT_MIRROR2);
	print_pmc_reg(PMC_SYS_33V_EN);
	print_pmc_reg(PMC_BONDOUT_MIRROR_ACCESS);
	print_pmc_reg(PMC_GATE);
	printk("*******************\n");
}

static void __iomem *emc = IO_ADDRESS(TEGRA_EMC_BASE);
#define print_emc_reg(X) printk("  %03xh %s\t0x%08x\n", X, #X, readl(emc + X))

static void dump_emc_reg(void){

	printk("********TEGRA_EMC_BASE PHYS=0x%x VIRT=0x%x***********\n", TEGRA_EMC_BASE, (unsigned int)IO_ADDRESS(TEGRA_EMC_BASE));
	print_emc_reg(EMC_CFG);
	print_emc_reg(EMC_ADR_CFG);
	print_emc_reg(EMC_ADR_CFG_1);
	print_emc_reg(EMC_REF_CTRL);
	print_emc_reg(EMC_PIN);
	print_emc_reg(EMC_TIMING_CTRL);
	print_emc_reg(EMC_RC);
	print_emc_reg(EMC_RFC);
	print_emc_reg(EMC_RAS);
	print_emc_reg(EMC_RP);
	print_emc_reg(EMC_R2W);
	print_emc_reg(EMC_W2R);
	print_emc_reg(EMC_R2P);
	print_emc_reg(EMC_W2P);
	print_emc_reg(EMC_RD_RCD);
	print_emc_reg(EMC_WR_RCD);
	print_emc_reg(EMC_RRD);
	print_emc_reg(EMC_REXT);
	print_emc_reg(EMC_WDV);
	print_emc_reg(EMC_QUSE);
	print_emc_reg(EMC_QRST);
	print_emc_reg(EMC_QSAFE);
	print_emc_reg(EMC_RDV);
	print_emc_reg(EMC_REFRESH);
	print_emc_reg(EMC_BURST_REFRESH_NUM);
	print_emc_reg(EMC_PDEX2WR);
	print_emc_reg(EMC_PDEX2RD);
	print_emc_reg(EMC_PCHG2PDEN);
	print_emc_reg(EMC_ACT2PDEN);
	print_emc_reg(EMC_AR2PDEN);
	print_emc_reg(EMC_RW2PDEN);
	print_emc_reg(EMC_TXSR);
	print_emc_reg(EMC_TCKE);
	print_emc_reg(EMC_TFAW);
	print_emc_reg(EMC_TRPAB);
	print_emc_reg(EMC_TCLKSTABLE);
	print_emc_reg(EMC_TCLKSTOP);
	print_emc_reg(EMC_TREFBW);
	print_emc_reg(EMC_QUSE_EXTRA);
	print_emc_reg(EMC_ODT_WRITE);
	print_emc_reg(EMC_ODT_READ);
	print_emc_reg(EMC_MRS);
	print_emc_reg(EMC_EMRS);
	print_emc_reg(EMC_REF);
	print_emc_reg(EMC_PRE);
	print_emc_reg(EMC_NOP);
	print_emc_reg(EMC_SELF_REF);
	print_emc_reg(EMC_DPD);
	print_emc_reg(EMC_MRW);
	print_emc_reg(EMC_MRR);
	print_emc_reg(EMC_FBIO_CFG1);
	print_emc_reg(EMC_FBIO_DQSIB_DLY);
	print_emc_reg(EMC_FBIO_DQSIB_DLY_MSB);
	print_emc_reg(EMC_FBIO_CFG5);
	print_emc_reg(EMC_FBIO_QUSE_DLY);
	print_emc_reg(EMC_FBIO_QUSE_DLY_MSB);
	print_emc_reg(EMC_FBIO_CFG6);
	print_emc_reg(EMC_DQS_TRIMMER_RD0);
	print_emc_reg(EMC_DQS_TRIMMER_RD1);
	print_emc_reg(EMC_DQS_TRIMMER_RD2);
	print_emc_reg(EMC_DQS_TRIMMER_RD3);
	print_emc_reg(EMC_LL_ARB_CONFIG);
	print_emc_reg(EMC_T_MIN_CRITICAL_HP);
	print_emc_reg(EMC_T_MIN_CRITICAL_TIMEOUT);
	print_emc_reg(EMC_T_MIN_LOAD);
	print_emc_reg(EMC_T_MAX_CRITICAL_HP);
	print_emc_reg(EMC_T_MAX_CRITICAL_TIMEOUT);
	print_emc_reg(EMC_T_MAX_LOAD);
	print_emc_reg(EMC_AUTO_CAL_CONFIG);
	print_emc_reg(EMC_AUTO_CAL_INTERVAL);
	print_emc_reg(EMC_AUTO_CAL_STATUS);
	print_emc_reg(EMC_REQ_CTRL);
	print_emc_reg(EMC_EMC_STATUS);
	print_emc_reg(EMC_CFG_2);
	print_emc_reg(EMC_CFG_DIG_DLL);
	print_emc_reg(EMC_DLL_XFORM_DQS);
	print_emc_reg(EMC_DLL_XFORM_QUSE);
	print_emc_reg(EMC_DIG_DLL_UPPER_STATUS);
	print_emc_reg(EMC_DIG_DLL_LOWER_STATUS);
	print_emc_reg(EMC_CTT_TERM_CTRL);
	print_emc_reg(EMC_ZCAL_REF_CNT);
	print_emc_reg(EMC_ZCAL_WAIT_CNT);
	print_emc_reg(EMC_ZCAL_MRW_CMD);	
	printk("*******************\n");
}

#ifdef CONFIG_I_LOVE_PBJ20

#define print_sdmmc3_reg(X) printk("  %03xh %s\t0x%08x\n", X, #X, readl(sdmmc3 + X))

static void dump_sdmmc3_reg(void){

	printk("********TEGRA_SDMMC3_BASE PHYS=0x%x VIRT=0x%x***********\n", TEGRA_SDMMC3_BASE,  (unsigned int)sdmmc3);
	print_sdmmc3_reg(SDMMC_SYSTEM_ADDRESS);
	print_sdmmc3_reg(SDMMC_BLOCK_SIZE_BLOCK_COUNT);
	print_sdmmc3_reg(SDMMC_ARGUMENT);
	print_sdmmc3_reg(SDMMC_CMD_XFER_MODE);
	print_sdmmc3_reg(SDMMC_RESPONSE_R0_R1);
	print_sdmmc3_reg(SDMMC_RESPONSE_R2_R3);
	print_sdmmc3_reg(SDMMC_RESPONSE_R4_R5);
	print_sdmmc3_reg(SDMMC_RESPONSE_R6_R7);
	print_sdmmc3_reg(SDMMC_BUFFER_DATA_PORT);
	print_sdmmc3_reg(SDMMC_PRESENT_STATE);
	print_sdmmc3_reg(SDMMC_POWER_CONTROL_HOST);
	print_sdmmc3_reg(SDMMC_SW_RESET_TIMEOUT_CTRL_CLOCK_CONTROL);
	print_sdmmc3_reg(SDMMC_INTERRUPT_STATUS);
	print_sdmmc3_reg(SDMMC_INTERRUPT_STATUS_ENABLE);
	print_sdmmc3_reg(SDMMC_INTERRUPT_SIGNAL_ENABLE);
	print_sdmmc3_reg(SDMMC_AUTO_CMD12_ERR_STATUS);
	print_sdmmc3_reg(SDMMC_CAPABILITIES);
	print_sdmmc3_reg(SDMMC_MAXIMUM_CURRENT);
	print_sdmmc3_reg(SDMMC_FORCE_EVENT);
	print_sdmmc3_reg(SDMMC_ADMA_ERR_STATUS);
	print_sdmmc3_reg(SDMMC_ADMA_SYSTEM_ADDRESS);
	print_sdmmc3_reg(SDMMC_SPI_INTERRUPT_SUPPORT);
	print_sdmmc3_reg(SDMMC_SLOT_INTERRUPT_STATUS);
	print_sdmmc3_reg(SDMMC_VENDOR_CLOCK_CNTRL);
	print_sdmmc3_reg(SDMMC_VENDOR_SPI_CNTRL);
	print_sdmmc3_reg(SDMMC_VENDOR_SPI_INTR_STATUS);
	print_sdmmc3_reg(SDMMC_VENDOR_CEATA_CNTRL);
	print_sdmmc3_reg(SDMMC_VENDOR_BOOT_CNTRL);
	print_sdmmc3_reg(SDMMC_VENDOR_BOOT_ACK_TIMEOUT);
	print_sdmmc3_reg(SDMMC_VENDOR_BOOT_DAT_TIMEOUT);
	print_sdmmc3_reg(SDMMC_VENDOR_DEBOUNCE_COUNT);
	printk("*******************\n");
}
#endif

static void dump_dvfs(void){

	printk("********TEGRA_EMC_BASE PHYS=0x%x VIRT=0x%x***********\n", TEGRA_EMC_BASE, (unsigned int)IO_ADDRESS(TEGRA_EMC_BASE));
	//46 registers
	print_emc_reg(EMC_RC); 
	print_emc_reg(EMC_RFC);
	print_emc_reg(EMC_RAS);
	print_emc_reg(EMC_RP);
	print_emc_reg(EMC_R2W);
	print_emc_reg(EMC_W2R);
	print_emc_reg(EMC_R2P);
	print_emc_reg(EMC_W2P);
	print_emc_reg(EMC_RD_RCD);
	print_emc_reg(EMC_WR_RCD);
	print_emc_reg(EMC_RRD);
	print_emc_reg(EMC_REXT);
	print_emc_reg(EMC_WDV);
	print_emc_reg(EMC_QUSE);
	print_emc_reg(EMC_QRST);
	print_emc_reg(EMC_QSAFE);
	print_emc_reg(EMC_RDV);
	print_emc_reg(EMC_REFRESH);
	print_emc_reg(EMC_BURST_REFRESH_NUM);
	print_emc_reg(EMC_PDEX2WR);
	print_emc_reg(EMC_PDEX2RD);
	print_emc_reg(EMC_PCHG2PDEN);
	print_emc_reg(EMC_ACT2PDEN);
	print_emc_reg(EMC_AR2PDEN);
	print_emc_reg(EMC_RW2PDEN);
	print_emc_reg(EMC_TXSR);
	print_emc_reg(EMC_TCKE);
	print_emc_reg(EMC_TFAW);
	print_emc_reg(EMC_TRPAB);
	print_emc_reg(EMC_TCLKSTABLE);
	print_emc_reg(EMC_TCLKSTOP);
	print_emc_reg(EMC_TREFBW);
	print_emc_reg(EMC_QUSE_EXTRA);
	print_emc_reg(EMC_FBIO_CFG6 );
	print_emc_reg(EMC_ODT_WRITE );
	print_emc_reg(EMC_ODT_READ);
	print_emc_reg(EMC_FBIO_CFG5 );
	print_emc_reg(EMC_CFG_DIG_DLL);
	print_emc_reg(EMC_DLL_XFORM_DQS);
	print_emc_reg(EMC_DLL_XFORM_QUSE);
	print_emc_reg(EMC_ZCAL_REF_CNT );
	print_emc_reg(EMC_ZCAL_WAIT_CNT);
	print_emc_reg(EMC_AUTO_CAL_INTERVAL);
	print_emc_reg(EMC_CFG_CLKTRIM_0);
	print_emc_reg(EMC_CFG_CLKTRIM_1);
	print_emc_reg(EMC_CFG_CLKTRIM_2);

}

static void __iomem *apb_misc = IO_ADDRESS(TEGRA_APB_MISC_BASE);
#define print_apb_misc_reg(X) printk("  %03xh %s\t0x%08x\n", X, #X, readl(apb_misc + X))

static void dump_apb_misc_reg(void){

#define APB_MISC_GP_MODEREG 0x800
#define APB_MISC_PP_STRAPPING_OPT_A 0x8


	printk("********TEGRA_APB_MISC_BASE PHYS=0x%x VIRT=0x%x***********\n", TEGRA_APB_MISC_BASE, (unsigned int)IO_ADDRESS(TEGRA_APB_MISC_BASE));
	print_apb_misc_reg(APB_MISC_PP_STRAPPING_OPT_A);
	printk("\t\tsdram config strap: %d (bit7:4)\n", (int)(*(volatile unsigned long *)(IO_ADDRESS(TEGRA_APB_MISC_BASE)+APB_MISC_PP_STRAPPING_OPT_A)>>4)&0xF);
	print_apb_misc_reg(APB_MISC_GP_MODEREG);

	printk("*******************\n");
}

#define DUMP_PMC_REG "pmc"
#define DUMP_EMC_REG "emc"
#ifdef CONFIG_I_LOVE_PBJ20
#define DUMP_SDMMC3_REG "sdmmc3"
#endif
#define DUMP_DVFS "dvfs"
#define DUMP_APBMISC_REG "apb_misc"

static ssize_t dump_reg_store(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf, size_t n)
{
	if(n >= strlen(DUMP_PMC_REG) && strncmp(buf, DUMP_PMC_REG, strlen(DUMP_PMC_REG)) == 0) 
	{
		printk("[%s]\t\t%s\n", __FUNCTION__, buf);
		dump_pmc_reg();
		return n;
	}

	if(n >= strlen(DUMP_EMC_REG) && strncmp(buf, DUMP_EMC_REG, strlen(DUMP_EMC_REG)) == 0) 
	{
		printk("[%s]\t\t%s\n", __FUNCTION__, buf);
		dump_emc_reg();
		return n;
	}

#ifdef CONFIG_I_LOVE_PBJ20
	if(n >= strlen(DUMP_SDMMC3_REG) && strncmp(buf, DUMP_SDMMC3_REG, strlen(DUMP_SDMMC3_REG)) == 0) 
	{
		printk("[%s]\t\t%s\n", __FUNCTION__, buf);
		dump_sdmmc3_reg();
		return n;
	}
#endif

	if(n >= strlen(DUMP_DVFS) && strncmp(buf, DUMP_DVFS, strlen(DUMP_DVFS)) == 0) 
	{
		printk("[%s]\t\t%s\n", __FUNCTION__, buf);
		dump_dvfs();
		return n;
	}

	if(n >= strlen(DUMP_APBMISC_REG) && strncmp(buf, DUMP_APBMISC_REG, strlen(DUMP_APBMISC_REG)) == 0) 
	{
		printk("[%s]\t\t%s\n", __FUNCTION__, buf);
		dump_apb_misc_reg();
		return n;
	}

	printk("wrong parameters\n");


	return n;
}

extern unsigned int g_debug_mode;

static ssize_t log_show(struct kobject *kobj, struct kobj_attribute *attr, char * buf)
{
	char * s = buf;
	s += sprintf(s, "%d\n", g_debug_mode);
	return (s - buf);
}

static ssize_t log_store(struct kobject* kobj, struct kobj_attribute* kobj_attr, char* buf, size_t n)
{
	return n;
}

static int __init debug_init(void)
{
    int err;

    dump_reg_kobj = kobject_create_and_add("debug", NULL);

    err = sysfs_create_group(dump_reg_kobj, &attribute_group);
    if(err){
        printk("sysfs_create failed, \n");
	goto err_debug;
    }

    //BE CAREFUL
    if(g_debug_mode==0){

        attributes[2] = NULL; //mark off log attribute, 
                              //LOST ALL ATTRIBUTES AFTER THE INDEX
    }

    return 0;

err_debug:
    kobject_del(dump_reg_kobj);    
    return -ENODEV;
}

static void __exit debug_exit(void)
{
}

module_init(debug_init);
module_exit(debug_exit);
