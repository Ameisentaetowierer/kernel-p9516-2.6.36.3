#ifndef __PMC_REGS_H
#define __PMC_REGS_H

#define PMC_CTRL		0x0
#define PMC_SEC_DISABLE		0x4
#define PMC_PMC_SWRST		0x8
#define PMC_WAKE_MASK		0xc
#define PMC_WAKE_LEVEL		0x10
#define PMC_WAKE_STATUS		0x14
#define PMC_PMC_SW_WAKE_STATUS	0x18
#define PMC_DPAD_ORIDE		0x1C
#define PMC_DPD_SAMPLE  	0x20
#define PMC_DPD_ENABLE		0x24
#define PMC_PWRGATE_TIMER_OFF	0x28
#define PMC_PWRGATE_TIMER_ON	0x2C
#define PMC_PWRGATE_TOGGLE	0x30
#define PMC_REMOVE_CLAMPING_CMD 0x34
#define PMC_PWRGATE_STATUS	0x38
#define PMC_PWRGOOD_TIMER	0x3C
#define PMC_BLINK_TIMER		0x40
#define PMC_NO_IOPOWER		0x44
#define PMC_PWR_DET		0x48
#define PMC_PWR_DET_LATCH	0x4C
#define PMC_SCRATCH0		0x50
#define PMC_SCRATCH1		0x54
#define PMC_SCRATCH2		0x58
#define PMC_SCRATCH3		0x5C
#define PMC_SCRATCH4		0x60
#define PMC_SCRATCH5		0x64
#define PMC_SCRATCH6		0x68
#define PMC_SCRATCH7		0x6C
#define PMC_SCRATCH8		0x70
#define PMC_SCRATCH9		0x74
#define PMC_SCRATCH10		0x78
#define PMC_SCRATCH11		0x7C
#define PMC_SCRATCH12		0x80
#define PMC_SCRATCH13		0x84
#define PMC_SCRATCH14		0x88
#define PMC_SCRATCH15		0x8C
#define PMC_SCRATCH16		0x90
#define PMC_SCRATCH17		0x94
#define PMC_SCRATCH18		0x98
#define PMC_SCRATCH19		0x9C
#define PMC_SCRATCH20		0xA0
#define PMC_SCRATCH21		0xA4
#define PMC_SCRATCH22		0xA8
#define PMC_SCRATCH23		0xAC
#define PMC_SECURE_SCRATCH0	0xB0
#define PMC_SECURE_SCRATCH1	0xB4
#define PMC_SECURE_SCRATCH2	0xB8
#define PMC_SECURE_SCRATCH3	0xBC
#define PMC_SECURE_SCRATCH4	0xC0
#define PMC_SECURE_SCRATCH5	0xC4
#define PMC_CPUPWRGOOD_TIMER	0xC8
#define PMC_CPUPWROFF_TIMER	0xCC
#define PMC_PG_MASK		0xD0
#define PMC_PG_MASK_1		0xD4
#define PMC_AUTO_WAKE_LVL	0xD8
#define PMC_AUTO_WAKE_LVL_MASK	0xDC
#define PMC_WAKE_DELAY		0xE0
#define PMC_PWR_DET_VAL		0xE4
#define PMC_DDR_PWR		0xE8
#define PMC_USB_DEBOUNCE_DEL	0xEC
#define PMC_USB_AO		0xF0
#define PMC_CRYPTO_OP		0xF4
#define PMC_PLLP_WB0_OVERRIDE	0xF8
#define PMC_SCRATCH24		0xFC
#define PMC_SCRATCH25		0x100
#define PMC_SCRATCH26		0x104
#define PMC_SCRATCH27		0x108
#define PMC_SCRATCH28		0x10C
#define PMC_SCRATCH29		0x110
#define PMC_SCRATCH30		0x114
#define PMC_SCRATCH31		0x118
#define PMC_SCRATCH32		0x11C
#define PMC_SCRATCH33		0x120
#define PMC_SCRATCH34		0x124
#define PMC_SCRATCH35		0x128
#define PMC_SCRATCH36		0x12C
#define PMC_SCRATCH37		0x130
#define PMC_SCRATCH38		0x134
#define PMC_SCRATCH39		0x138
#define PMC_SCRATCH40		0x13C
#define PMC_SCRATCH41		0x140
#define PMC_SCRATCH42		0x144
#define PMC_BONDOUT_MIRROR0	0x148
#define PMC_BONDOUT_MIRROR1	0x14C
#define PMC_BONDOUT_MIRROR2	0x150
#define PMC_SYS_33V_EN		0x154
#define PMC_BONDOUT_MIRROR_ACCESS 0x158
#define PMC_GATE		0x15C

#endif