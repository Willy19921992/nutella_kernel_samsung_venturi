/* linux/arch/arm/mach-s5pv210/mach-aries.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/gpio.h>
#include <linux/gpio_event.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/max8998.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/usb/ch9.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/clk.h>
#include <linux/usb/ch9.h>
#include <linux/input/cypress-touchkey.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/skbuff.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/gpio.h>
#include <mach/gpio-venturi-settings.h>
#include <mach/adc.h>
#include <mach/param.h>
#include <mach/system.h>

// VenturiGB_Usys_jypark 2011.08.08 - DMB [[
#ifdef CONFIG_S3C64XX_DEV_SPI
#include <plat/s3c64xx-spi.h>
#include <mach/spi-clocks.h>
#endif
// VenturiGB_Usys_jypark 2011.08.08 - DMB ]]

#include <mach/sec_switch.h>

#include <linux/usb/gadget.h>
#include <linux/fsa9480.h>

#ifdef CONFIG_PN544
#include <linux/pn544.h>
#endif

#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/wlan_plat.h>

#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <plat/media.h>
#include <mach/media.h>

#ifdef CONFIG_S5PV210_POWER_DOMAIN
#include <mach/power-domain.h>
#endif
#include <mach/cpu-freq-v210.h>


#ifdef CONFIG_SENSORS_BMA222
#include <linux/i2c/bma023_dev.h>
#endif

#ifdef CONFIG_SENSORS_MMC328X
#include <linux/i2c/mmc328x.h>
#endif

#ifdef CONFIG_S5PV210_POWER_DOMAIN
#include <mach/power-domain.h>
#endif

#ifdef CONFIG_VIDEO_CE147
#include <media/ce147_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5KA3DFX
#include <media/s5ka3dfx_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K5CCGX
#include <media/s5k5ccgx_platform.h>
#endif
#ifdef CONFIG_VIDEO_S5K4ECGX
#include <media/s5k4ecgx.h>
#endif

#include <plat/regs-serial.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/fb.h>
#include <plat/mfc.h>
#include <plat/iic.h>
#include <plat/pm.h>

#include <plat/sdhci.h>
#include <plat/fimc.h>
#include <plat/jpeg.h>
#include <plat/clock.h>
#include <plat/regs-otg.h>
#include <linux/gp2a.h>
#include <../../../drivers/video/samsung/s3cfb.h>
#ifdef CONFIG_SAMSUNG_JACK
#include <linux/sec_jack.h>
#else
#include <mach/sec_jack.h>
#endif
#include <linux/input/mxt224.h>
#include <linux/max17040_battery.h>
#include <linux/mfd/max8998.h>
#include <linux/switch.h>
#include <mach/sec_battery.h>
#include <mach/max8998_function.h>

#ifdef CONFIG_KERNEL_DEBUG_SEC
#include <linux/kernel_sec_common.h>
#endif

#include "aries.h"

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

struct device *gps_dev = NULL;
EXPORT_SYMBOL(gps_dev);

unsigned int HWREV =0;
EXPORT_SYMBOL(HWREV);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

#define KERNEL_REBOOT_MASK      0xFFFFFFFF
#define REBOOT_MODE_FAST_BOOT		7

#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24

#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

struct wifi_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static int aries_notifier_call(struct notifier_block *this,
					unsigned long code, void *_cmd)
{
	int mode = REBOOT_MODE_NONE;

	if ((code == SYS_RESTART) && _cmd) {
		if (!strcmp((char *)_cmd, "arm11_fota"))
			mode = REBOOT_MODE_ARM11_FOTA;
		else if (!strcmp((char *)_cmd, "arm9_fota"))
			mode = REBOOT_MODE_ARM9_FOTA;
		else if (!strcmp((char *)_cmd, "recovery"))
			mode = REBOOT_MODE_RECOVERY;
		else if (!strcmp((char *)_cmd, "bootloader"))
			mode = REBOOT_MODE_FAST_BOOT;
		else if (!strcmp((char *)_cmd, "download"))
			mode = REBOOT_MODE_DOWNLOAD;
		else
			mode = REBOOT_MODE_NONE;
	}
	if (code != SYS_POWER_OFF) {
		if (sec_set_param_value) {
			sec_set_param_value(__REBOOT_MODE, &mode);
		}
	}

	return NOTIFY_DONE;
}

static struct notifier_block aries_reboot_notifier = {
	.notifier_call = aries_notifier_call,
};

static ssize_t hwrev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%x\n", HWREV);
}

static DEVICE_ATTR(hwrev, S_IRUGO, hwrev_show, NULL);

static void gps_gpio_init(void)
{
	struct device *gps_dev;

	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	if (IS_ERR(gps_dev)) {
		pr_err("Failed to create device(gps)!\n");
		goto err;
	}
	if (device_create_file(gps_dev, &dev_attr_hwrev) < 0)
		pr_err("Failed to create device file(%s)!\n",
		       dev_attr_hwrev.attr.name);
	
	gpio_request(GPIO_GPS_nRST, "GPS_nRST");	/* XMMC3CLK */
	s3c_gpio_setpull(GPIO_GPS_nRST, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_nRST, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_nRST, 1);

	gpio_request(GPIO_GPS_PWR_EN, "GPS_PWR_EN");	/* XMMC3CLK */
	s3c_gpio_setpull(GPIO_GPS_PWR_EN, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_PWR_EN, 0);

	s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
	gpio_export(GPIO_GPS_nRST, 1);
	gpio_export(GPIO_GPS_PWR_EN, 1);

	gpio_export_link(gps_dev, "GPS_nRST", GPIO_GPS_nRST);
	gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN);

 err:
	return;
}

static void aries_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");
};

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define S5PV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define S5PV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define S5PV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg aries_uartcfgs[] __initdata = {
	{
		.hwport		= 0,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
		.wake_peer	= aries_bt_uart_wake_peer,
	},
	{
		.hwport		= 1,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
#ifndef CONFIG_FIQ_DEBUGGER
	{
		.hwport		= 2,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
#endif
	{
		.hwport		= 3,
		.flags		= 0,
		.ucon		= S5PV210_UCON_DEFAULT,
		.ulcon		= S5PV210_ULCON_DEFAULT,
		.ufcon		= S5PV210_UFCON_DEFAULT,
	},
};

#define S5PV210_LCD_WIDTH 480
#define S5PV210_LCD_HEIGHT 800

static struct s3cfb_lcd hx8369 = {
        .width = S5PV210_LCD_WIDTH,
        .height = S5PV210_LCD_HEIGHT,
	.p_width = 52,
	.p_height = 86,
	.bpp = 24,
	.freq = 60,

	.timing = {
		.h_fp = 32,
		.h_bp = 32,
		.h_sw = 14,
		.v_fp = 12,
		.v_fpe = 1,
		.v_bp = 12,
		.v_bpe = 1,
		.v_sw = 8,
	},
	.polarity = {
		.rise_vclk 	= 0, // video data fetch at DOTCLK falling edge 
		.inv_hsync 	= 1,	// low active
		.inv_vsync 	= 1,	// low active
		.inv_vden 	= 0,	// data is vaild when DEpin is high
	},
};

#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0 (12288 * SZ_1K)
// Disabled to save memory (we can't find where it's used)
//#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1 (9900 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2 (12288 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0 (14336 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1 (21504 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD (S5PV210_LCD_WIDTH * \
					     S5PV210_LCD_HEIGHT * 4 * \
					     (CONFIG_FB_S3C_NR_BUFFERS + \
						 (CONFIG_FB_S3C_NUM_OVLY_WIN * \
						  CONFIG_FB_S3C_NUM_BUF_OVLY_WIN)))
// Was 8M, but we're only using it to encode VGA jpegs
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG (4096 * SZ_1K)
#define  S5PV210_ANDROID_PMEM_MEMSIZE_PMEM (5550 * SZ_1K)
#define  S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 (3000 * SZ_1K)
#define  S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_ADSP (1500 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_TEXTSTREAM (3000 * SZ_1K)


static struct s5p_media_device aries_media_devs[] = {
	[0] = {
		.id = S5P_MDEV_MFC,
		.name = "mfc",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0,
		.paddr = 0,
	},
	[1] = {
		.id = S5P_MDEV_MFC,
		.name = "mfc",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1,
		.paddr = 0,
	},
	[2] = {
		.id = S5P_MDEV_FIMC0,
		.name = "fimc0",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0,
		.paddr = 0,
	},
/*	[3] = {
		.id = S5P_MDEV_FIMC1,
		.name = "fimc1",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1,
		.paddr = 0,
	},*/
	[4] = {
		.id = S5P_MDEV_FIMC2,
		.name = "fimc2",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2,
		.paddr = 0,
	},
	[5] = {
		.id = S5P_MDEV_JPEG,
		.name = "jpeg",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG,
		.paddr = 0,
	},
	[6] = {
		.id = S5P_MDEV_FIMD,
		.name = "fimd",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD,
		.paddr = 0,
	},		
	[7] = {
		.id = S5P_MDEV_TEXSTREAM,
		.name = "s3c_bc",
		.bank = 1,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_TEXTSTREAM,
		.paddr = 0,
	},		
};

static struct regulator_consumer_supply ldo3_consumer[] = {
	REGULATOR_SUPPLY("pd_io", "s3c-usbgadget")
};

static struct regulator_consumer_supply ldo5_consumer[] = {
	REGULATOR_SUPPLY("vtf", NULL),
};

static struct regulator_consumer_supply ldo7_consumer[] = {
	REGULATOR_SUPPLY("vlcd", NULL),
};

static struct regulator_consumer_supply ldo8_consumer[] = {
	REGULATOR_SUPPLY("pd_core", "s3c-usbgadget"),
	REGULATOR_SUPPLY("tvout", NULL),
};

static struct regulator_consumer_supply ldo11_consumer[] = {
	REGULATOR_SUPPLY("cam_af", NULL),
};

static struct regulator_consumer_supply ldo12_consumer[] = {
	REGULATOR_SUPPLY("mag_vcc", NULL),
};

static struct regulator_consumer_supply ldo13_consumer[] = {
	REGULATOR_SUPPLY("vga_core", NULL),
};

static struct regulator_consumer_supply ldo14_consumer[] = {
	REGULATOR_SUPPLY("vled", NULL),
};

static struct regulator_consumer_supply ldo15_consumer[] = {
	REGULATOR_SUPPLY("cam_soc_a", NULL),
};

static struct regulator_consumer_supply ldo16_consumer[] = {
	REGULATOR_SUPPLY("cam_soc_io", NULL),
};

static struct regulator_consumer_supply ldo17_consumer[] = {
	REGULATOR_SUPPLY("vcc_lcd", NULL),
};

static struct regulator_consumer_supply buck1_consumer[] = {
	REGULATOR_SUPPLY("vddarm", NULL),
};

static struct regulator_consumer_supply buck2_consumer[] = {
	REGULATOR_SUPPLY("vddint", NULL),
};

static struct regulator_consumer_supply buck4_consumer[] = {
	REGULATOR_SUPPLY("cam_soc_core", NULL),
};

static struct regulator_init_data aries_ldo2_data = {
	.constraints	= {
		.name		= "VALIVE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.always_on	= 1,
		.state_mem	= {
			.enabled = 1,
		},
	},
};

static struct regulator_init_data aries_ldo3_data = {
	.constraints	= {
		.name		= "VUSB_1.1V",
		.min_uV		= 1100000,
		.max_uV		= 1100000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo3_consumer),
	.consumer_supplies	= ldo3_consumer,
};

static struct regulator_init_data aries_ldo4_data = {
	.constraints	= {
		.name		= "VADC_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.always_on	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
};

static struct regulator_init_data aries_ldo5_data = {
	.constraints	= {
		.name		= "VTF_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo5_consumer),
	.consumer_supplies	= ldo5_consumer,
};

static struct regulator_init_data aries_ldo7_data = {
	.constraints	= {
		.name		= "VLCD_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 0,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo7_consumer),
	.consumer_supplies	= ldo7_consumer,
};

static struct regulator_init_data aries_ldo8_data = {
	.constraints	= {
		.name		= "VUSB_3.3V",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo8_consumer),
	.consumer_supplies	= ldo8_consumer,
};

static struct regulator_init_data aries_ldo9_data = {
	.constraints	= {
		.name		= "VCC_2.8V_PDA",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.always_on	= 1,
	},
};

static struct regulator_init_data aries_ldo11_data = {
	.constraints	= {
		.name		= "CAM_AF_3.0V",
		.min_uV		= 3000000,
		.max_uV		= 3000000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo11_consumer),
	.consumer_supplies	= ldo11_consumer,
};

static struct regulator_init_data aries_ldo12_data = {
	.constraints	= {
		.name		= "MAG_VCC_2.6V",
		.min_uV		= 2600000,
		.max_uV		= 2600000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo12_consumer),
	.consumer_supplies	= ldo12_consumer,
};

static struct regulator_init_data aries_ldo13_data = {
	.constraints	= {
		.name		= "VGA_CORE_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo13_consumer),
	.consumer_supplies	= ldo13_consumer,
};

static struct regulator_init_data aries_ldo14_data = {
	.constraints	= {
		.name		= "VCC_3.3V_LED",
		.min_uV		= 3300000,
		.max_uV		= 3300000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo14_consumer),
	.consumer_supplies	= ldo14_consumer,
};

static struct regulator_init_data aries_ldo15_data = {
	.constraints	= {
		.name		= "CAM_SOC_A2.7V",
		.min_uV		= 2700000,
		.max_uV		= 2700000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo15_consumer),
	.consumer_supplies	= ldo15_consumer,
};

static struct regulator_init_data aries_ldo16_data = {
	.constraints	= {
		.name		= "CAM_SOC_IO_2.8V",
		.min_uV		= 2800000,
		.max_uV		= 2800000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo16_consumer),
	.consumer_supplies	= ldo16_consumer,
};

static struct regulator_init_data aries_ldo17_data = {
	.constraints	= {
		.name		= "VCC_3.0V_LCD",
		.min_uV		= 2600000,
		.max_uV		= 2600000,
		.apply_uV	= 1,
		.always_on	= 0,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(ldo17_consumer),
	.consumer_supplies	= ldo17_consumer,
};

static struct regulator_init_data aries_buck1_data = {
	.constraints	= {
		.name		= "VDD_ARM",
		.min_uV		= 750000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV	= 1250000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck1_consumer),
	.consumer_supplies	= buck1_consumer,
};

static struct regulator_init_data aries_buck2_data = {
	.constraints	= {
		.name		= "VDD_INT",
		.min_uV		= 750000,
		.max_uV		= 1500000,
		.apply_uV	= 1,
		.valid_ops_mask	= REGULATOR_CHANGE_VOLTAGE |
				  REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.uV	= 1100000,
			.mode	= REGULATOR_MODE_NORMAL,
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck2_consumer),
	.consumer_supplies	= buck2_consumer,
};

static struct regulator_init_data aries_buck3_data = {
	.constraints	= {
		.name		= "VCC_1.8V",
		.min_uV		= 1800000,
		.max_uV		= 1800000,
		.apply_uV	= 1,
		.always_on	= 1,
	},
};

static struct regulator_init_data aries_buck4_data = {
	.constraints	= {
		.name		= "CAM_SOC_CORE_1.2V",
		.min_uV		= 1200000,
		.max_uV		= 1200000,
		.apply_uV	= 1,
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		.state_mem	= {
			.disabled = 1,
		},
	},
	.num_consumer_supplies	= ARRAY_SIZE(buck4_consumer),
	.consumer_supplies	= buck4_consumer,
};

static struct max8998_regulator_data aries_regulators[] = {
	{ MAX8998_LDO2,  &aries_ldo2_data },
	{ MAX8998_LDO3,  &aries_ldo3_data },
	{ MAX8998_LDO4,  &aries_ldo4_data },
	{ MAX8998_LDO5,  &aries_ldo5_data },
	{ MAX8998_LDO7,  &aries_ldo7_data },
	{ MAX8998_LDO8,  &aries_ldo8_data },
	{ MAX8998_LDO9,  &aries_ldo9_data },
	{ MAX8998_LDO11, &aries_ldo11_data },
	{ MAX8998_LDO12, &aries_ldo12_data },
	{ MAX8998_LDO13, &aries_ldo13_data },
	{ MAX8998_LDO14, &aries_ldo14_data },
	{ MAX8998_LDO15, &aries_ldo15_data },
	{ MAX8998_LDO16, &aries_ldo16_data },
	{ MAX8998_LDO17, &aries_ldo17_data },
	{ MAX8998_BUCK1, &aries_buck1_data },
	{ MAX8998_BUCK2, &aries_buck2_data },
	{ MAX8998_BUCK3, &aries_buck3_data },
	{ MAX8998_BUCK4, &aries_buck4_data },
};

static struct max8998_adc_table_data temper_table[] =  {
	{  264,  650 },
	{  275,  640 },
	{  286,  630 },
	{  293,  620 },
	{  299,  610 },
	{  306,  600 },
#if !defined(CONFIG_ARIES_NTT)
	{  324,  590 },
	{  341,  580 },
	{  354,  570 },
	{  368,  560 },
#else
	{  310,  590 },
	{  315,  580 },
	{  320,  570 },
	{  324,  560 },
#endif
	{  381,  550 },
	{  396,  540 },
	{  411,  530 },
	{  427,  520 },
	{  442,  510 },
	{  457,  500 },
	{  472,  490 },
	{  487,  480 },
	{  503,  470 },
	{  518,  460 },
	{  533,  450 },
	{  554,  440 },
	{  574,  430 },
	{  595,  420 },
	{  615,  410 },
	{  636,  400 },
	{  656,  390 },
	{  677,  380 },
	{  697,  370 },
	{  718,  360 },
	{  738,  350 },
	{  761,  340 },
	{  784,  330 },
	{  806,  320 },
	{  829,  310 },
	{  852,  300 },
	{  875,  290 },
	{  898,  280 },
	{  920,  270 },
	{  943,  260 },
	{  966,  250 },
	{  990,  240 },
	{ 1013,  230 },
	{ 1037,  220 },
	{ 1060,  210 },
	{ 1084,  200 },
	{ 1108,  190 },
	{ 1131,  180 },
	{ 1155,  170 },
	{ 1178,  160 },
	{ 1202,  150 },
	{ 1226,  140 },
	{ 1251,  130 },
	{ 1275,  120 },
	{ 1299,  110 },
	{ 1324,  100 },
	{ 1348,   90 },
	{ 1372,   80 },
	{ 1396,   70 },
	{ 1421,   60 },
	{ 1445,   50 },
	{ 1468,   40 },
	{ 1491,   30 },
	{ 1513,   20 },
#if !defined(CONFIG_ARIES_NTT)
	{ 1536,   10 },
	{ 1559,    0 },
	{ 1577,  -10 },
	{ 1596,  -20 },
#else
	{ 1518,   10 },
	{ 1524,    0 },
	{ 1544,  -10 },
	{ 1570,  -20 },
#endif
	{ 1614,  -30 },
	{ 1619,  -40 },
	{ 1632,  -50 },
	{ 1658,  -60 },
	{ 1667,  -70 }, 
};
struct max8998_charger_callbacks *charger_callbacks;
static enum cable_type_t set_cable_status = CABLE_TYPE_NONE;

static void max8998_charger_register_callbacks(
		struct max8998_charger_callbacks *ptr)
{
	charger_callbacks = ptr;
	/* if there was a cable status change before the charger was
	ready, send this now */
	if ((set_cable_status != 0) && charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

static struct max8998_charger_data aries_charger = {
	.register_callbacks	= &max8998_charger_register_callbacks,
	.adc_table		= temper_table,
	.adc_array_size		= ARRAY_SIZE(temper_table),
};

static struct max8998_platform_data max8998_pdata = {
        .num_regulators = ARRAY_SIZE(aries_regulators),
        .regulators     = aries_regulators,
        .charger        = &aries_charger,
        /* Preloads must be in increasing order of voltage value */
        .buck1_voltage4 = 950000,
        .buck1_voltage3 = 1050000,
        .buck1_voltage2 = 1200000,
        .buck1_voltage1 = 1275000,
        .buck2_voltage2 = 1000000,
        .buck2_voltage1 = 1100000,
        .buck1_set1     = GPIO_BUCK_1_EN_A,
        .buck1_set2     = GPIO_BUCK_1_EN_B,
        .buck2_set3     = GPIO_BUCK_2_EN,
        .buck1_default_idx = 1,
        .buck2_default_idx = 0,
};

struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id	= -1,
};

#ifdef CONFIG_TOUCHSCREEN_CYTMA340
int led_power_control(int onoff){
	struct regulator *regulator;

	regulator = regulator_get(NULL, "vled");
	if (IS_ERR(regulator)) {
		printk("[TSP][ERROR] regulator_get fail\n");
		return -1;
	}

	if (onoff) {
		regulator_enable(regulator);
	} else {
		if (regulator_is_enabled(regulator))
			regulator_force_disable(regulator);
	}
	regulator_put(regulator);
	return 0;

}
EXPORT_SYMBOL(led_power_control);
#endif



static void hx8369_cfg_gpio(struct platform_device *pdev)
{
	int i;

	printk("[%s][minhyodebug]=================\n", __func__);

	/* DISPLAY_HSYNC */
	s3c_gpio_cfgpin(GPIO_DISPLAY_HSYNC, GPIO_DISPLAY_HSYNC_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_HSYNC, S3C_GPIO_PULL_NONE);

	/* DISPLAY_VSYNC */
	s3c_gpio_cfgpin(GPIO_DISPLAY_VSYNC, GPIO_DISPLAY_VSYNC_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_VSYNC, S3C_GPIO_PULL_NONE);

	/* DISPLAY_DE */
	s3c_gpio_cfgpin(GPIO_DISPLAY_DE, GPIO_DISPLAY_DE_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_DE, S3C_GPIO_PULL_NONE);

	/* DISPLAY_PCLK */
	s3c_gpio_cfgpin(GPIO_DISPLAY_PCLK, GPIO_DISPLAY_PCLK_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_PCLK, S3C_GPIO_PULL_NONE);

	/*
		WARNING:
			This code works on situation that LCD data pin is
			set serially by hardware
	 */
	for (i = 0; i < 24; i++)	{
		s3c_gpio_cfgpin(GPIO_LCD_D0 + i, GPIO_LCD_D0_AF);
		s3c_gpio_setpull(GPIO_LCD_D0 + i, S3C_GPIO_PULL_NONE);
	}

	/* mDNIe SEL: why we shall write 0x2 ? */
#ifdef CONFIG_FB_S3C_MDNIE
	writel(0x1, S5P_MDNIE_SEL);
#else
	writel(0x2, S5P_MDNIE_SEL);
#endif
#if 0
	/* drive strength to max */
	writel(0xffffffff, S5PC_VA_GPIO + 0x12c);
	writel(0xffffffff, S5PC_VA_GPIO + 0x14c);
	writel(0xffffffff, S5PC_VA_GPIO + 0x16c);
	writel(0x000000ff, S5PC_VA_GPIO + 0x18c);
#endif

	/* DISPLAY_CS */
	s3c_gpio_cfgpin(GPIO_DISPLAY_CS, GPIO_DISPLAY_CS_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_CS, S3C_GPIO_PULL_NONE);

	/* DISPLAY_CLK */
	gpio_request(GPIO_DISPLAY_CLK, "GPIO_DISPLAY_CLK");
	s3c_gpio_cfgpin(GPIO_DISPLAY_CLK, GPIO_DISPLAY_CLK_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_CLK, S3C_GPIO_PULL_NONE);
	gpio_direction_output(GPIO_DISPLAY_CLK, 1);

	/* DISPLAY_SI */
	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, GPIO_DISPLAY_SI_AF);
	s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);

	/* Venturi Backlight */
	s3c_gpio_cfgpin(GPIO_BACKLIGHT_EN, GPIO_BACKLIGHT_AF);
	s3c_gpio_setpull(GPIO_BACKLIGHT_EN, S3C_GPIO_PULL_NONE);

	/*KGVS : configuring GPJ2(4) as FM interrupt */
	//s3c_gpio_cfgpin(S5PV210_GPJ2(4), S5PV210_GPJ2_4_GPIO_INT20_4);
}

void lcd_cfg_gpio_early_suspend(void)
{
	int i;
	printk("[%s]\n", __func__);

	gpio_request(GPIO_DISPLAY_HSYNC, "GPIO_DISPLAY_HSYNC");
	s3c_gpio_setpull(GPIO_DISPLAY_HSYNC, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_DISPLAY_HSYNC, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_DISPLAY_HSYNC, 0);

	gpio_request(GPIO_DISPLAY_VSYNC, "GPIO_DISPLAY_VSYNC");
	s3c_gpio_setpull(GPIO_DISPLAY_VSYNC, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_DISPLAY_VSYNC, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_DISPLAY_VSYNC, 0);

	gpio_request(GPIO_DISPLAY_DE, "GPIO_DISPLAY_DE");
	s3c_gpio_setpull(GPIO_DISPLAY_DE, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_DISPLAY_DE, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_DISPLAY_DE, 0);

	gpio_request(GPIO_DISPLAY_PCLK, "GPIO_DISPLAY_PCLK");
	s3c_gpio_setpull(GPIO_DISPLAY_PCLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_DISPLAY_PCLK, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_DISPLAY_PCLK, 0);

	/*
		WARNING:
			This code works on situation that LCD data pin is
			set serially by hardware
	 */
	for (i = 0; i < 24; i++)	{
		gpio_request(GPIO_LCD_D0 + i, "GPIO_LCD_D0 + i,");
		s3c_gpio_setpull(GPIO_LCD_D0 + i, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_LCD_D0 + i, S3C_GPIO_OUTPUT);
		gpio_direction_output(GPIO_LCD_D0 + i, 0);
	}
	// drive strength to min
	writel(0x00000000, S5P_VA_GPIO + 0x12c);		// GPF0DRV
	writel(0x00000000, S5P_VA_GPIO + 0x14c);		// GPF1DRV
	writel(0x00000000, S5P_VA_GPIO + 0x16c);		// GPF2DRV
	writel(0x00000000, S5P_VA_GPIO + 0x18c);		// GPF3DRV

	gpio_request(GPIO_DISPLAY_CS, "GPIO_DISPLAY_CS");
	s3c_gpio_setpull(GPIO_DISPLAY_CS, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_DISPLAY_CS, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_DISPLAY_CS, 0);

	gpio_request(GPIO_DISPLAY_CLK, "GPIO_DISPLAY_CLK");
	s3c_gpio_setpull(GPIO_DISPLAY_CLK, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_DISPLAY_CLK, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_DISPLAY_CLK, 0);

	gpio_request(GPIO_DISPLAY_SI, "GPIO_DISPLAY_SI");
	s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_DISPLAY_SI, 0);

	gpio_request(GPIO_MLCD_RST, "GPIO_MLCD_RST");
	s3c_gpio_setpull(GPIO_MLCD_RST, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_MLCD_RST, 0);


	gpio_free(GPIO_DISPLAY_HSYNC);
	gpio_free(GPIO_DISPLAY_VSYNC);
	gpio_free(GPIO_DISPLAY_DE);
	gpio_free(GPIO_DISPLAY_PCLK);
	gpio_free(GPIO_DISPLAY_CS);
	gpio_free(GPIO_DISPLAY_CLK);
	gpio_free(GPIO_DISPLAY_SI);
	gpio_free(GPIO_MLCD_RST);

	for (i = 0; i < 24; i++)	{
			gpio_free(GPIO_LCD_D0 + i);
	}

}
EXPORT_SYMBOL(lcd_cfg_gpio_early_suspend);

void lcd_cfg_gpio_late_resume(void)
{

}
EXPORT_SYMBOL(lcd_cfg_gpio_late_resume);

static int hx8369_reset_lcd(struct platform_device *pdev)
{
	int err;

	err = gpio_request(GPIO_MLCD_RST, "MLCD_RST");
	if (err) {
		printk(KERN_ERR "failed to request MP0(5) for "
				"lcd reset control\n");
		return err;
	}

	gpio_direction_output(GPIO_MLCD_RST, 1);
	msleep(50);

	gpio_set_value(GPIO_MLCD_RST, 0);
	msleep(10);

	gpio_set_value(GPIO_MLCD_RST, 1);
	msleep(10);

	gpio_free(GPIO_MLCD_RST);

	return 0;
}

static int hx8369_backlight_on(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_FB_S3C_NT35580
static int get_lcdtype(void)
{
	int panel_id;
	
	if((gpio_get_value(GPIO_OLED_ID)==0) && ( gpio_get_value(GPIO_DIC_ID) != 0))
		panel_id = 0; // Sony Panel
	else if((gpio_get_value(GPIO_OLED_ID)!=0) && ( gpio_get_value(GPIO_DIC_ID) == 0))
		panel_id = 2; // Hitachi Panel
	else
		panel_id = 1; // Hydis Panel

    printk("LCD_ID1=0x%x, LCD_ID2=0x%x, LCDTYPE=%s\n", gpio_get_value(GPIO_OLED_ID), gpio_get_value(GPIO_DIC_ID), ((panel_id==0) ? "SONY" : (panel_id== 2) ? "HITACHI" : "HYDIS") ); 

	return panel_id;
}
#endif

#ifdef CONFIG_FB_S3C_HX8369
static struct s3c_platform_fb hx8369_data __initdata = {
	.hw_ver		= 0x62,
	.clk_name	= "sclk_fimd",
	.nr_wins	= 5,
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,

	.lcd = &hx8369,
	.cfg_gpio	= hx8369_cfg_gpio,
	.backlight_on	= hx8369_backlight_on,
	.reset_lcd	= hx8369_reset_lcd,
};
#endif

#define LCD_BUS_NUM	3
#define DISPLAY_CS	S5PV210_MP01(1)
#define SUB_DISPLAY_CS	S5PV210_MP01(2)
#define DISPLAY_CLK	S5PV210_MP04(1)
#define DISPLAY_SI	S5PV210_MP04(3)

#ifdef CONFIG_FB_S3C_TL2796
static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "tl2796",
		.platform_data	= &aries_panel_data,
		.max_speed_hz	= 1520000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

static struct spi_gpio_platform_data tl2796_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= -1,
	.num_chipselect = 2,
};

static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &tl2796_spi_gpio_data,
	},
};
#endif

// VenturiGB_Usys_jypark 2011.08.08 - DMB [[
#ifdef CONFIG_S3C64XX_DEV_SPI

#define SMDK_MMCSPI_CS 0
static struct s3c64xx_spi_csinfo smdk_spi0_csi[] = {
 [SMDK_MMCSPI_CS] = {
 .line = S5PV210_GPB(1),
 .set_level = gpio_set_value,
 .fb_delay = 0x0,
 },
};
/*
static struct s3c64xx_spi_csinfo smdk_spi1_csi[] = {
 [SMDK_MMCSPI_CS] = {
 .line = S5PV210_GPB(5),
 .set_level = gpio_set_value,
 .fb_delay = 0x0,
 },
};
*/ 

static struct spi_board_info s3c_spi_devs[] __initdata = {
 [0] = {
 .modalias        = "tdmbspi", /* device node name */
 .mode            = SPI_MODE_0, /* CPOL=0, CPHA=0 */
 .max_speed_hz    = 5000000,
 /* Connected to SPI-0 as 1st Slave */
 .bus_num         = 0,
 .irq             = IRQ_SPI0,
 .chip_select     = 0,
 .controller_data = &smdk_spi0_csi[SMDK_MMCSPI_CS],
 },
 #if 0
 [1] = {
 .modalias        = "spidev", /* device node name */
 .mode            = SPI_MODE_0, /* CPOL=0, CPHA=0 */
 .max_speed_hz    = 10000000,
 /* Connected to SPI-1 as 1st Slave */
 .bus_num         = 1,
 .irq             = IRQ_SPI1,
 .chip_select     = 0,
 .controller_data = &smdk_spi1_csi[SMDK_MMCSPI_CS],
 },
 #endif
};
#endif
// VenturiGB_Usys_jypark 2011.08.08 - DMB ]]

#ifdef CONFIG_FB_S3C_HX8369
static struct spi_board_info spi_board_info[] __initdata = {
	{
		.modalias	= "hx8369",
		.platform_data	= &aries_panel_data,
		.max_speed_hz	= 1520000,
		.bus_num	= LCD_BUS_NUM,
		.chip_select	= 0,
		.mode		= SPI_MODE_3,
		.controller_data = (void *)DISPLAY_CS,
	},
};

static struct spi_gpio_platform_data hx8369_spi_gpio_data = {
	.sck	= DISPLAY_CLK,
	.mosi	= DISPLAY_SI,
	.miso	= 0,
	.num_chipselect = 2,
};

static struct platform_device s3c_device_spi_gpio = {
	.name	= "spi_gpio",
	.id	= LCD_BUS_NUM,
	.dev	= {
		.parent		= &s3c_device_fb.dev,
		.platform_data	= &hx8369_spi_gpio_data,
	},
};
#endif

static struct i2c_gpio_platform_data venturi_i2c4_platdata = {
	.sda_pin		= GPIO_AP_SDA_18V,
	.scl_pin		= GPIO_AP_SCL_18V,
	.udelay			= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c4_device = {
	.name			= "i2c-gpio",
	.id			= 4,
	.dev.platform_data	= &venturi_i2c4_platdata,
};

static struct i2c_gpio_platform_data venturi_i2c5_platdata = {
	.sda_pin		= GPIO_AP_SDA_28V,
	.scl_pin		= GPIO_AP_SCL_28V,
	.udelay			= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c5_device = {
	.name			= "i2c-gpio",
	.id			= 5,
	.dev.platform_data	= &venturi_i2c5_platdata,
};

static struct i2c_gpio_platform_data venturi_i2c6_platdata = {
	.sda_pin		= GPIO_AP_PMIC_SDA,
	.scl_pin		= GPIO_AP_PMIC_SCL,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c6_device = {
	.name			= "i2c-gpio",
	.id			= 6,
	.dev.platform_data	= &venturi_i2c6_platdata,
};

static struct i2c_gpio_platform_data venturi_i2c7_platdata = {
	.sda_pin		= GPIO_USB_SDA_28V,
	.scl_pin		= GPIO_USB_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c7_device = {
	.name			= "i2c-gpio",
	.id			= 7,
	.dev.platform_data	= &venturi_i2c7_platdata,
};
// For FM radio
#if !defined(CONFIG_ARIES_NTT)
static struct i2c_gpio_platform_data venturi_i2c8_platdata = {
	.sda_pin		= GPIO_FM_SDA_28V,
	.scl_pin		= GPIO_FM_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c8_device = {
	.name			= "i2c-gpio",
	.id			= 8,
	.dev.platform_data	= &venturi_i2c8_platdata,
};
#endif

#if defined(CONFIG_BATTERY_MAX17040)
static struct i2c_gpio_platform_data venturi_i2c9_platdata = {
	.sda_pin		= FUEL_SDA_18V,
	.scl_pin		= FUEL_SCL_18V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c9_device = {
	.name			= "i2c-gpio",
	.id			= 9,
	.dev.platform_data	= &venturi_i2c9_platdata,
};
#endif

#if defined(CONFIG_KEYPAD_CYPRESS_TOUCH)
static struct i2c_gpio_platform_data venturi_i2c10_platdata = {
	.sda_pin		= _3_TOUCH_SDA_28V,
	.scl_pin		= _3_TOUCH_SCL_28V,
	.udelay 		= 0, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c10_device = {
	.name			= "i2c-gpio",
	.id			= 10,
	.dev.platform_data	= &venturi_i2c10_platdata,
};
#endif

#if defined(CONFIG_OPTICAL_GP2A)
static struct i2c_gpio_platform_data venturi_i2c11_platdata = {
	.sda_pin		= GPIO_ALS_SDA_28V,
	.scl_pin		= GPIO_ALS_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c11_device = {
	.name			= "i2c-gpio",
	.id			= 11,
	.dev.platform_data	= &venturi_i2c11_platdata,
};
#endif

static struct i2c_gpio_platform_data venturi_i2c12_platdata = {
	.sda_pin		= GPIO_MSENSE_SDA_28V,
	.scl_pin		= GPIO_MSENSE_SCL_28V,
	.udelay 		= 2, /* 250KHz */
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c12_device = {
	.name			= "i2c-gpio",
	.id			= 12,
	.dev.platform_data	= &venturi_i2c12_platdata,
};

#ifdef CONFIG_PN544
static struct i2c_gpio_platform_data venturi_i2c14_platdata = {
	.sda_pin		= NFC_SDA_18V,
	.scl_pin		= NFC_SCL_18V,
	.udelay			= 2,
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.scl_is_output_only	= 0,
};

static struct platform_device venturi_i2c14_device = {
	.name			= "i2c-gpio",
	.id			= 14,
	.dev.platform_data	= &venturi_i2c14_platdata,
};
#endif

#if defined(CONFIG_KEYPAD_CYPRESS_TOUCH)
static void touch_keypad_gpio_init(void)
{
	int ret = 0;

	ret = gpio_request(_3_GPIO_TOUCH_EN, "TOUCH_EN");
	if (ret)
		printk(KERN_ERR "Failed to request gpio touch_en.\n");
}

static void touch_keypad_onoff(int onoff)
{
	gpio_direction_output(_3_GPIO_TOUCH_EN, onoff);

	if (onoff == TOUCHKEY_OFF)
		msleep(30);
	else
		msleep(25);
}

static const int touch_keypad_code[] = {
	KEY_MENU,
	KEY_BACK,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_CAMERA,
	KEY_SEND,	
};

static struct touchkey_platform_data touchkey_data = {
	.keycode_cnt = ARRAY_SIZE(touch_keypad_code),
	.keycode = touch_keypad_code,
	.touchkey_onoff = touch_keypad_onoff,
};
#endif

static struct gpio_event_direct_entry aries_keypad_key_map[] = {
		{
			.gpio	= S5PV210_GPH2(6),
			.code	= KEY_POWER,
		},
#if defined(CONFIG_MACH_VENTURI)
#if !defined(CONFIG_VENTURI_USA)
		{
			.gpio	= S5PV210_GPH3(0),
			.code	= KEY_HOME,
		},
#endif
		{
			.gpio	= S5PV210_GPH3(1),
			.code	= KEY_VOLUMEDOWN,
		},
		{
			.gpio	= S5PV210_GPH3(2),
			.code	= KEY_VOLUMEUP,
		}
#endif
};

static struct gpio_event_input_info aries_keypad_key_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = true,
	.debounce_time.tv64 = 5 * NSEC_PER_MSEC,
	.type = EV_KEY,
	.keymap = aries_keypad_key_map,
	.keymap_size = ARRAY_SIZE(aries_keypad_key_map)
};

static struct gpio_event_info *aries_input_info[] = {
	&aries_keypad_key_info.info,
};


static struct gpio_event_platform_data aries_input_data = {
	.names = {
		"s3c-keypad",
		NULL,
	},
	.info = aries_input_info,
	.info_count = ARRAY_SIZE(aries_input_info),
};

static struct platform_device aries_input_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev = {
		.platform_data = &aries_input_data,
	},
};

#ifdef CONFIG_S5P_ADC
static struct s3c_adc_mach_info s3c_adc_platform __initdata = {
	/* s5pc110 support 12-bit resolution */
	.delay		= 10000,
	.presc		= 65,
	.resolution	= 12,
};
#endif

#ifdef CONFIG_SAMSUNG_JACK
/* There is a only common mic bias gpio in aries H/W */
#ifdef CONFIG_SND_SMDKC110_MC1N2
extern void McDrv_Ctrl_MICBIAS2(int en);
#endif

static DEFINE_SPINLOCK(mic_bias_lock);
static bool jack_mic_bias;
static void set_shared_mic_bias(void)
{
#ifdef CONFIG_SND_SMDKC110_MC1N2
        McDrv_Ctrl_MICBIAS2(jack_mic_bias);
#endif
}

static void sec_jack_set_micbias_state(bool on)
{
	unsigned long flags;

#ifndef CONFIG_SND_SMDKC110_MC1N2
	spin_lock_irqsave(&mic_bias_lock, flags);
#endif
	jack_mic_bias = on;
	set_shared_mic_bias();
#ifndef CONFIG_SND_SMDKC110_MC1N2
	spin_unlock_irqrestore(&mic_bias_lock, flags);
#endif
}
#endif

#ifdef CONFIG_VIDEO_CE147
/*
 * Guide for Camera Configuration for Jupiter board
 * ITU CAM CH A: CE147
*/

static struct regulator *cam_isp_core_regulator;/*buck4*/
static struct regulator *cam_isp_host_regulator;/*15*/
static struct regulator *cam_af_regulator;/*11*/
static struct regulator *cam_sensor_core_regulator;/*12*/
static struct regulator *cam_vga_vddio_regulator;/*13*/
static struct regulator *cam_vga_dvdd_regulator;/*14*/
static struct regulator *cam_vga_avdd_regulator;/*16*/
static bool ce147_powered_on;

static int ce147_regulator_init(void)
{
/*BUCK 4*/
	if (IS_ERR_OR_NULL(cam_isp_core_regulator)) {
		cam_isp_core_regulator = regulator_get(NULL, "cam_isp_core");
		if (IS_ERR_OR_NULL(cam_isp_core_regulator)) {
			pr_err("failed to get cam_isp_core regulator");
			return -EINVAL;
		}
	}
/*ldo 11*/
	if (IS_ERR_OR_NULL(cam_af_regulator)) {
		cam_af_regulator = regulator_get(NULL, "cam_af");
		if (IS_ERR_OR_NULL(cam_af_regulator)) {
			pr_err("failed to get cam_af regulator");
			return -EINVAL;
		}
	}
/*ldo 12*/
	if (IS_ERR_OR_NULL(cam_sensor_core_regulator)) {
		cam_sensor_core_regulator = regulator_get(NULL, "cam_sensor");
		if (IS_ERR_OR_NULL(cam_sensor_core_regulator)) {
			pr_err("failed to get cam_sensor regulator");
			return -EINVAL;
		}
	}
/*ldo 13*/
	if (IS_ERR_OR_NULL(cam_vga_vddio_regulator)) {
		cam_vga_vddio_regulator = regulator_get(NULL, "vga_vddio");
		if (IS_ERR_OR_NULL(cam_vga_vddio_regulator)) {
			pr_err("failed to get vga_vddio regulator");
			return -EINVAL;
		}
	}
/*ldo 14*/
	if (IS_ERR_OR_NULL(cam_vga_dvdd_regulator)) {
		cam_vga_dvdd_regulator = regulator_get(NULL, "vga_dvdd");
		if (IS_ERR_OR_NULL(cam_vga_dvdd_regulator)) {
			pr_err("failed to get vga_dvdd regulator");
			return -EINVAL;
		}
	}
/*ldo 15*/
	if (IS_ERR_OR_NULL(cam_isp_host_regulator)) {
		cam_isp_host_regulator = regulator_get(NULL, "cam_isp_host");
		if (IS_ERR_OR_NULL(cam_isp_host_regulator)) {
			pr_err("failed to get cam_isp_host regulator");
			return -EINVAL;
		}
	}
/*ldo 16*/
	if (IS_ERR_OR_NULL(cam_vga_avdd_regulator)) {
		cam_vga_avdd_regulator = regulator_get(NULL, "vga_avdd");
		if (IS_ERR_OR_NULL(cam_vga_avdd_regulator)) {
			pr_err("failed to get vga_avdd regulator");
			return -EINVAL;
		}
	}
	pr_debug("cam_isp_core_regulator = %p\n", cam_isp_core_regulator);
	pr_debug("cam_isp_host_regulator = %p\n", cam_isp_host_regulator);
	pr_debug("cam_af_regulator = %p\n", cam_af_regulator);
	pr_debug("cam_sensor_core_regulator = %p\n", cam_sensor_core_regulator);
	pr_debug("cam_vga_vddio_regulator = %p\n", cam_vga_vddio_regulator);
	pr_debug("cam_vga_dvdd_regulator = %p\n", cam_vga_dvdd_regulator);
	pr_debug("cam_vga_avdd_regulator = %p\n", cam_vga_avdd_regulator);
	return 0;
}

static void ce147_init(void)
{
	/* CAM_IO_EN - GPB(7) */
	if (gpio_request(GPIO_GPB7, "GPB7") < 0)
		pr_err("failed gpio_request(GPB7) for camera control\n");
	/* CAM_MEGA_nRST - GPJ1(5) */
	if (gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1") < 0)
		pr_err("failed gpio_request(GPJ1) for camera control\n");
	/* CAM_MEGA_EN - GPJ0(6) */
	if (gpio_request(GPIO_CAM_MEGA_EN, "GPJ0") < 0)
		pr_err("failed gpio_request(GPJ0) for camera control\n");
}

static int ce147_ldo_en(bool en)
{
	int err = 0;
	int result;

	if (IS_ERR_OR_NULL(cam_isp_core_regulator) ||
		IS_ERR_OR_NULL(cam_isp_host_regulator) ||
		IS_ERR_OR_NULL(cam_af_regulator) || //) {// ||
		IS_ERR_OR_NULL(cam_sensor_core_regulator) ||
		IS_ERR_OR_NULL(cam_vga_vddio_regulator) ||
		IS_ERR_OR_NULL(cam_vga_dvdd_regulator) ||
		IS_ERR_OR_NULL(cam_vga_avdd_regulator)) {
		pr_err("Camera regulators not initialized\n");
		return -EINVAL;
	}

	if (!en)
		goto off;

	/* Turn CAM_ISP_CORE_1.2V(VDD_REG) on BUCK 4*/
	err = regulator_enable(cam_isp_core_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_isp_core\n");
		goto off;
	}
	mdelay(1);

	/* Turn CAM_AF_2.8V or 3.0V on ldo 11*/
	err = regulator_enable(cam_af_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_af\n");
		goto off;
	}
	udelay(50);

	/*ldo 12*/
	err = regulator_enable(cam_sensor_core_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_sensor\n");
		goto off;
	}
	udelay(50);

	/*ldo 13*/
	err = regulator_enable(cam_vga_vddio_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_vga_vddio\n");
		goto off;
	}
	udelay(50);

	/*ldo 14*/
	err = regulator_enable(cam_vga_dvdd_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_vga_dvdd\n");
		goto off;
	}
	udelay(50);

	/* Turn CAM_ISP_HOST_2.8V(VDDIO) on ldo 15*/
	err = regulator_enable(cam_isp_host_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_isp_core\n");
		goto off;
	}
	udelay(50);

	/*ldo 16*/
	err = regulator_enable(cam_vga_avdd_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_vga_avdd\n");
		goto off;
	}
	udelay(50);
	
	/* Turn CAM_SENSOR_A_2.8V(VDDA) on */
	gpio_set_value(GPIO_GPB7, 1);
	mdelay(1);

	return 0;

off:
	result = err;

	gpio_direction_output(GPIO_GPB7, 1);
	gpio_set_value(GPIO_GPB7, 0);

	/* ldo 11 */
	err = regulator_disable(cam_af_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	/* ldo 12 */
	err = regulator_disable(cam_sensor_core_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_sensor\n");
		result = err;
	}
	/* ldo 13 */
	err = regulator_disable(cam_vga_vddio_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_vga_vddio\n");
		result = err;
	}
	/* ldo 14 */
	err = regulator_disable(cam_vga_dvdd_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_vga_dvdd\n");
		result = err;
	}
	/* ldo 15 */
	err = regulator_disable(cam_isp_host_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	/* ldo 16 */
	err = regulator_disable(cam_vga_avdd_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_vga_avdd\n");
		result = err;
	}
	/* BUCK 4 */
	err = regulator_disable(cam_isp_core_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	return result;
}

static int ce147_power_on(void)
{	
	int err;
	bool TRUE = true;

	if (ce147_regulator_init()) {
			pr_err("Failed to initialize camera regulators\n");
			return -EINVAL;
	}
	
	ce147_init();

	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");

		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}
	
	ce147_ldo_en(TRUE);

	mdelay(1);

	// CAM_VGA_nSTBY  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	mdelay(1);

	// Mclk enable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0x02));

	mdelay(1);

	// CAM_VGA_nRST  HIGH		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);

	gpio_set_value(GPIO_CAM_VGA_nRST, 1);	

	mdelay(1);

	// CAM_VGA_nSTBY  LOW	
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	// CAM_MEGA_EN HIGH
	gpio_direction_output(GPIO_CAM_MEGA_EN, 0);

	gpio_set_value(GPIO_CAM_MEGA_EN, 1);

	mdelay(1);

	// CAM_MEGA_nRST HIGH
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);

	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);

	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_GPB7);

	mdelay(5);

	return 0;
}


static int ce147_power_off(void)
{
	int err;
	bool FALSE = false;

	/* CAM_IO_EN - GPB(7) */
	err = gpio_request(GPIO_GPB7, "GPB7");
	
	if(err) {
		printk(KERN_ERR "failed to request GPB7 for camera control\n");
	
		return err;
	}

	/* CAM_MEGA_EN - GPJ0(6) */
	err = gpio_request(GPIO_CAM_MEGA_EN, "GPJ0");

	if(err) {
		printk(KERN_ERR "failed to request GPJ0 for camera control\n");
	
		return err;
	}

	/* CAM_MEGA_nRST - GPJ1(5) */
	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ1");
	
	if(err) {
		printk(KERN_ERR "failed to request GPJ1 for camera control\n");
	
		return err;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");

	if (err) {
		printk(KERN_ERR "failed to request GPB2 for camera control\n");

		return err;
	}
	/* CAM_VGA_nSTBY - GPB(0)  */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");

	if (err) {
		printk(KERN_ERR "failed to request GPB0 for camera control\n");

		return err;
	}

	// CAM_VGA_nSTBY  LOW	
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);

	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	mdelay(1);

	// CAM_VGA_nRST  LOW		
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	mdelay(1);

	// CAM_MEGA_nRST - GPJ1(5) LOW
	gpio_direction_output(GPIO_CAM_MEGA_nRST, 1);
	
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
	
	mdelay(1);

	// Mclk disable
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);
	
	mdelay(1);

	// CAM_MEGA_EN - GPJ0(6) LOW
	gpio_direction_output(GPIO_CAM_MEGA_EN, 1);
	
	gpio_set_value(GPIO_CAM_MEGA_EN, 0);

	mdelay(1);

	ce147_ldo_en(FALSE);

	mdelay(1);
	
	gpio_free(GPIO_CAM_MEGA_EN);
	gpio_free(GPIO_CAM_MEGA_nRST);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_GPB7);

	return 0;
}


static int ce147_power_en(int onoff)
{
	int bd_level;
	int err = 0;
#if 0
	if(onoff){
		ce147_ldo_en(true);
		s3c_gpio_cfgpin(S5PV210_GPE1(3), S5PV210_GPE1_3_CAM_A_CLKOUT);
		ce147_cam_en(true);
		ce147_cam_nrst(true);
	} else {
		ce147_cam_en(false);
		ce147_cam_nrst(false);
		s3c_gpio_cfgpin(S5PV210_GPE1(3), 0);
		ce147_ldo_en(false);
	}

	return 0;
#endif

	if (onoff != ce147_powered_on) {
		if (onoff)
			err = ce147_power_on();
		else {
			err = ce147_power_off();
			s3c_i2c0_force_stop();
		}
		if (!err)
			ce147_powered_on = onoff;
	}

	return 0;
}

static int smdkc110_cam1_power(int onoff)
{
	int err;
	/* Implement on/off operations */

	/* CAM_VGA_nSTBY - GPB(0) */
	err = gpio_request(S5PV210_GPB(0), "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPB(0), 0);
	
	mdelay(1);

	gpio_direction_output(S5PV210_GPB(0), 1);

	mdelay(1);

	gpio_set_value(S5PV210_GPB(0), 1);

	mdelay(1);

	gpio_free(S5PV210_GPB(0));
	
	mdelay(1);

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(S5PV210_GPB(2), "GPB");

	if (err) {
		printk(KERN_ERR "failed to request GPB for camera control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPB(2), 0);

	mdelay(1);

	gpio_direction_output(S5PV210_GPB(2), 1);

	mdelay(1);

	gpio_set_value(S5PV210_GPB(2), 1);

	mdelay(1);

	gpio_free(S5PV210_GPB(2));

	return 0;
}

/*
 * Guide for Camera Configuration for Jupiter board
 * ITU CAM CH A: CE147
*/

/* External camera module setting */
static struct ce147_platform_data ce147_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
	.power_en = ce147_power_en,
};

static struct i2c_board_info  ce147_i2c_info = {
	I2C_BOARD_INFO("CE147", 0x78>>1),
	.platform_data = &ce147_plat,
};

static struct s3c_platform_camera ce147 = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &ce147_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam",//"sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	// Polarity 
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
	.cam_power	= ce147_power_en,
};
#endif

/*
 * Guide for Camera Configuration for Crespo board
 * ITU CAM CH A: LSI s5k4ecgx
 */

#ifdef CONFIG_VIDEO_S5K4ECGX
static struct regulator *cam_isp_core_regulator;/*buck4*/
static struct regulator *cam_isp_host_regulator;/*15*/
static struct regulator *cam_vga_avdd_regulator;/*16*/
static struct regulator *cam_af_regulator;/*11*/
static struct regulator *cam_vga_vddio_regulator;/*13*/
static int s5k4ecgx_powered_on = 0;

static int s5k4ecgx_regulator_init(void)
{
	printk("%s: start\n", __func__);
/*BUCK 4*/
	
	if (IS_ERR_OR_NULL(cam_isp_core_regulator)) {
		cam_isp_core_regulator = regulator_get(NULL, "cam_soc_core");
		if (IS_ERR_OR_NULL(cam_isp_core_regulator)) {
			pr_err("failed to get cam_isp_core regulator");
			return -EINVAL;
		}
	}
	
/*ldo 15*/
	if (IS_ERR_OR_NULL(cam_isp_host_regulator)) {
		cam_isp_host_regulator = regulator_get(NULL, "cam_soc_a");
		if (IS_ERR_OR_NULL(cam_isp_host_regulator)) {
			pr_err("failed to get cam_isp_host regulator");
			return -EINVAL;
		}
	}
/*ldo 13*/
	if (IS_ERR_OR_NULL(cam_vga_vddio_regulator)) {
		cam_vga_vddio_regulator = regulator_get(NULL, "vga_core");
		if (IS_ERR_OR_NULL(cam_vga_vddio_regulator)) {
			pr_err("failed to get vga_vddio regulator");
			return -EINVAL;
		}
	}
/*ldo 16*/
	if (IS_ERR_OR_NULL(cam_vga_avdd_regulator)) {
		cam_vga_avdd_regulator = regulator_get(NULL, "cam_soc_io");
		if (IS_ERR_OR_NULL(cam_vga_avdd_regulator)) {
			pr_err("failed to get vga_avdd regulator");
			return -EINVAL;
		}
	}
/*ldo 11*/
	if (IS_ERR_OR_NULL(cam_af_regulator)) {
		cam_af_regulator = regulator_get(NULL, "cam_af");
		if (IS_ERR_OR_NULL(cam_af_regulator)) {
			pr_err("failed to get cam_af regulator");
			return -EINVAL;
		}
	}
	pr_debug("cam_isp_host_regulator = %p\n", cam_isp_host_regulator);
	pr_debug("cam_vga_avdd_regulator = %p\n", cam_vga_avdd_regulator);
	pr_debug("cam_af_regulator = %p\n", cam_af_regulator);
	pr_debug("cam_vga_vddio_regulator = %p\n", cam_vga_vddio_regulator);
	return 0;
}

static void s5k4ecgx_gpio_init(void)
{
	printk("%s: start\n", __func__);
	/* CAM_MEGA_nRST - GPJ1(5) */
	if (gpio_request(GPIO_CAM_MEGA_nRST, "GPJ15") < 0)
		pr_err("Failed to gpio_request GPJ15 GPIO\n");
	/* GPIO_CAM_nSTBY - GPJ0(6) */
	if (gpio_request(GPIO_CAM_nSTBY, "GPJ06") < 0)
		pr_err("Failed to gpio_request GPJ06 GPIO\n");
}

static int s5k4ecgx_ldo_en(bool en)
{
	int err = 0;
	int result;
	printk("%s: start\n", __func__);

	if (
		IS_ERR_OR_NULL(cam_isp_host_regulator) ||
		IS_ERR_OR_NULL(cam_vga_avdd_regulator) ||
		IS_ERR_OR_NULL(cam_af_regulator) || //) {// ||
		IS_ERR_OR_NULL(cam_vga_vddio_regulator)) { 
		pr_err("Camera regulators not initialized\n");
		return -EINVAL;
	}

	if (!en)
		goto off;

	s3c_gpio_cfgpin(S5PV210_GPD0(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPD0(1), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPD0(1), 1);

	// Turn CAM_ISP_CORE_1.2V(VDD_REG) on BUCK 4
	err = regulator_enable(cam_isp_core_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_isp_core\n");
		goto off;
	}
	udelay(50);
	
	/* Turn CAM_ISP_HOST_2.8V(VDDIO) on ldo 15*/
	err = regulator_enable(cam_isp_host_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_isp_core\n");
		goto off;
	}
	udelay(50);

	/*ldo 13*/
	err = regulator_enable(cam_vga_vddio_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_vga_vddio\n");
		goto off;
	}
	udelay(50);
	
	/*ldo 16*/
	err = regulator_enable(cam_vga_avdd_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_vga_avdd\n");
		goto off;
	}
	udelay(50);

	/* Turn CAM_AF_2.8V or 3.0V on ldo 11*/
	err = regulator_enable(cam_af_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_af\n");
		goto off;
	}
	mdelay(10);

	return 0;

off:
	result = err;
	
	/* ldo 16 */
	err = regulator_disable(cam_vga_avdd_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_vga_avdd\n");
		result = err;
	}

	/* ldo 13 */
	err = regulator_disable(cam_vga_vddio_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_vga_vddio\n");
		result = err;
	}
	/* ldo 15 */
	err = regulator_disable(cam_isp_host_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	/* ldo 11 */
	err = regulator_disable(cam_af_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	/* BUCK 4 */
	
	err = regulator_disable(cam_isp_core_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	
	s3c_gpio_cfgpin(S5PV210_GPD0(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPD0(1), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPD0(1), 0);

	return result;
}

static int s5k4ecgx_power_on(void)
{
	int err;

	printk("%s: start\n", __func__);

	if (s5k4ecgx_regulator_init()) {
			pr_err("Failed to initialize camera regulators\n");
			return -EINVAL;
	}

	s5k4ecgx_gpio_init();

	// CAM_VGA_nSTBY - GPJ2(6)
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPJ26");

	if (err) {
		printk(KERN_ERR "failed to request GPJ26 for camera control\n");

		return err;
	}

	// CAM_VGA_nRST - GPJ2(7)
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPJ27");

	if (err) {
		printk(KERN_ERR "failed to request GPJ27 for camera control\n");

		return err;
	}
	
	s5k4ecgx_ldo_en(true);

	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);
	mdelay(6);
	
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0x02));

	mdelay(4);

	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	gpio_set_value(GPIO_CAM_VGA_nRST, 1);

	mdelay(8);

	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	udelay(50);

	gpio_direction_output(GPIO_CAM_nSTBY, 0);
	gpio_set_value(GPIO_CAM_nSTBY, 1);

	udelay(50);

	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);
	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);

	msleep(50);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_nSTBY);
	gpio_free(GPIO_CAM_MEGA_nRST);

	return 0;
}

static int s5k4ecgx_power_off(void)
{
	int err;
	printk("%s: start\n", __func__);

	err = gpio_request(GPIO_CAM_nSTBY, "GPJ06");
	if(err) {
		printk(KERN_ERR "Failed to requst GPJ06 GPIO\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ15");
	if(err) {
		printk(KERN_ERR "Failed to requst GPJ15 GPIO\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPJ26");
	if(err) {
		printk(KERN_ERR "Failed to requst GPJ26");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nRST, "GPJ27");
	if(err) {
		printk(KERN_ERR "Failed to request GPJ27 GPIO\n");
		return err;
	}

	gpio_direction_output(GPIO_CAM_MEGA_nRST, 1);
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);
	udelay(70);

	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);
	udelay(50);
	
	gpio_direction_output(GPIO_CAM_nSTBY, 1);
	gpio_set_value(GPIO_CAM_nSTBY, 0);
	udelay(50);

	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);
	udelay(50);

	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);
	udelay(50);

	s5k4ecgx_ldo_en(false);
	
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_nSTBY);
	gpio_free(GPIO_CAM_MEGA_nRST);
	
	return 0;
}

static int s5k4ecgx_power_en(int onoff)
{
	int err = 0;
	printk("%s: start. onoff = %d\n", __func__, onoff);
	if (onoff != s5k4ecgx_powered_on) {
		if (onoff)
			err = s5k4ecgx_power_on();
		else {
			err = s5k4ecgx_power_off();
			s3c_i2c0_force_stop();
		}
		if (!err)
			s5k4ecgx_powered_on = onoff;
	}
	return 0;
}

static  int s5k4ecgx_flash(int flash_mode)
{
	int i = 0; 
	int err=0;
	int lux_val = 0;
	printk("%s: start\n", __func__);

	err = gpio_request(GPIO_CAM_FLASH_EN, "GPIO_CAM_FLASH_EN");
	if (err) {
		pr_err("failed to request GPIO_FLASH_EN for camera control\n");
		return err; 
	}    

	err = gpio_request(GPIO_CAM_FLASH_EN_SET, "GPIO_CAM_FLASH_EN_SET");
	if (err) {
		pr_err("failed to request GPIO_CAM_FLASH_EN_SET for camera control\n");
		return err; 
	}    

	gpio_direction_output(GPIO_CAM_FLASH_EN, 0);
	gpio_direction_output(GPIO_CAM_FLASH_EN_SET, 0);
	msleep(1);

	switch(flash_mode) {

		case 1 :  /* FLASH ON  */
			gpio_set_value(GPIO_CAM_FLASH_EN, 1);
			udelay(100);
			lux_val = 2; 
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);

			break;

		case 2 :  /* FLASH ON in AF */

#if 0 /* For the test, hs43.jeon */
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);
#else
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);

			/* data 9 in address 0 */
			lux_val = 15;
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);
			/* address 3 in MovieMode */
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			udelay(1);

			lux_val = 20;
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);

			/* data 1 in address 3 */

			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			udelay(1);
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);

			msleep(1);
#endif
			break;

		case 3 :  /* TORCH ON */
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);                                                                                                                                                  
			/* data 14 in address 0 of MovieMode 30mA */
			/* data 7 in address 0 of Moivemode 69mA */
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			udelay(1);

			lux_val = 7; /* rising edge : 7 */
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);

			break;

		default : /* FLASH OFF */
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			break;

	}

	gpio_free(GPIO_CAM_FLASH_EN_SET);
	gpio_free(GPIO_CAM_FLASH_EN);

	return err;
}

/*
 * Guide for Camera Configuration for Venturi board
 * ITU CAM CH A: S5K4ECGX
*/

/* External camera module setting */
static struct s5k4ecgx_platform_data s5k4ecgx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	//.is_mipi = 0,
	.flash_onoff = s5k4ecgx_flash,
};

static struct i2c_board_info  s5k4ecgx_i2c_info = {
	I2C_BOARD_INFO("S5K4ECGX", 0x5A>>1),
	.platform_data = &s5k4ecgx_plat,
};

static struct s3c_platform_camera s5k4ecgx = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5k4ecgx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam",//"sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	// Polarity 
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
	.cam_power	= s5k4ecgx_power_en,
};

#endif

#ifdef CONFIG_VIDEO_S5K5CCGX
static struct regulator *cam_isp_core_regulator;/*buck4*/
static struct regulator *cam_isp_host_regulator;/*15*/
static struct regulator *cam_vga_avdd_regulator;/*16*/
static struct regulator *cam_af_regulator;/*11*/
static struct regulator *cam_vga_vddio_regulator;/*13*/
static int s5k5ccgx_powered_on = 0;

static int s5k5ccgx_regulator_init(void)
{
	printk("%s: start\n", __func__);
/*BUCK 4*/
	
	if (IS_ERR_OR_NULL(cam_isp_core_regulator)) {
		cam_isp_core_regulator = regulator_get(NULL, "cam_soc_core");
		if (IS_ERR_OR_NULL(cam_isp_core_regulator)) {
			pr_err("failed to get cam_isp_core regulator");
			return -EINVAL;
		}
	}
	
/*ldo 15*/
	if (IS_ERR_OR_NULL(cam_isp_host_regulator)) {
		cam_isp_host_regulator = regulator_get(NULL, "cam_soc_a");
		if (IS_ERR_OR_NULL(cam_isp_host_regulator)) {
			pr_err("failed to get cam_isp_host regulator");
			return -EINVAL;
		}
	}
/*ldo 16*/
	if (IS_ERR_OR_NULL(cam_vga_avdd_regulator)) {
		cam_vga_avdd_regulator = regulator_get(NULL, "cam_soc_io");
		if (IS_ERR_OR_NULL(cam_vga_avdd_regulator)) {
			pr_err("failed to get vga_avdd regulator");
			return -EINVAL;
		}
	}
/*ldo 11*/
	if (IS_ERR_OR_NULL(cam_af_regulator)) {
		cam_af_regulator = regulator_get(NULL, "cam_af");
		if (IS_ERR_OR_NULL(cam_af_regulator)) {
			pr_err("failed to get cam_af regulator");
			return -EINVAL;
		}
	}
/*ldo 13*/
	if (IS_ERR_OR_NULL(cam_vga_vddio_regulator)) {
		cam_vga_vddio_regulator = regulator_get(NULL, "vga_core");
		if (IS_ERR_OR_NULL(cam_vga_vddio_regulator)) {
			pr_err("failed to get vga_vddio regulator");
			return -EINVAL;
		}
	}
//	pr_debug("cam_isp_core_regulator = %p\n", cam_isp_core_regulator);
	pr_debug("cam_isp_host_regulator = %p\n", cam_isp_host_regulator);
	pr_debug("cam_vga_avdd_regulator = %p\n", cam_vga_avdd_regulator);
	pr_debug("cam_af_regulator = %p\n", cam_af_regulator);
	pr_debug("cam_vga_vddio_regulator = %p\n", cam_vga_vddio_regulator);
	return 0;
}

static void s5k5ccgx_gpio_init(void)
{
	printk("%s: start\n", __func__);
	/* CAM_MEGA_nRST - GPJ1(5) */
	if (gpio_request(GPIO_CAM_MEGA_nRST, "GPJ15") < 0)
		pr_err("Failed to gpio_request GPJ15 GPIO\n");
	/* GPIO_CAM_nSTBY - GPJ0(6) */
	if (gpio_request(GPIO_CAM_nSTBY, "GPJ06") < 0)
		pr_err("Failed to gpio_request GPJ06 GPIO\n");
}

static int s5k5ccgx_ldo_en(bool en)
{
	int err = 0;
	int result;
	printk("%s: start\n", __func__);

	if (/*IS_ERR_OR_NULL(cam_isp_core_regulator) ||*/
		IS_ERR_OR_NULL(cam_isp_host_regulator) ||
		IS_ERR_OR_NULL(cam_vga_avdd_regulator) ||
		IS_ERR_OR_NULL(cam_af_regulator) || //) {// ||
		IS_ERR_OR_NULL(cam_vga_vddio_regulator)) { 
		pr_err("Camera regulators not initialized\n");
		return -EINVAL;
	}

	if (!en)
		goto off;

	// Turn CAM_ISP_CORE_1.2V(VDD_REG) on BUCK 4
	err = regulator_enable(cam_isp_core_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_isp_core\n");
		goto off;
	}
	udelay(50);
	
	/* Turn CAM_ISP_HOST_2.8V(VDDIO) on ldo 15*/
	err = regulator_enable(cam_isp_host_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_isp_core\n");
		goto off;
	}
	udelay(50);

	/*ldo 16*/
	err = regulator_enable(cam_vga_avdd_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_vga_avdd\n");
		goto off;
	}
	udelay(50);

	/* Turn CAM_AF_2.8V or 3.0V on ldo 11*/
	err = regulator_enable(cam_af_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_af\n");
		goto off;
	}

	/*ldo 13*/
	err = regulator_enable(cam_vga_vddio_regulator);
	if (err) {
		pr_err("Failed to enable regulator cam_vga_vddio\n");
		goto off;
	}
	udelay(20);

	return 0;

off:
	result = err;
	
	/* ldo 16 */
	err = regulator_disable(cam_vga_avdd_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_vga_avdd\n");
		result = err;
	}

	/* ldo 11 */
	err = regulator_disable(cam_af_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	/* BUCK 4 */
	
	err = regulator_disable(cam_isp_core_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	
	/* ldo 13 */
	err = regulator_disable(cam_vga_vddio_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_vga_vddio\n");
		result = err;
	}
	/* ldo 15 */
	err = regulator_disable(cam_isp_host_regulator);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_core\n");
		result = err;
	}
	return result;
}

static int s5k5ccgx_power_on(void)
{
	int err;

	printk("%s: start\n", __func__);

	if (s5k5ccgx_regulator_init()) {
			pr_err("Failed to initialize camera regulators\n");
			return -EINVAL;
	}

	s5k5ccgx_gpio_init();

	// CAM_VGA_nSTBY - GPJ2(6)
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPJ26");

	if (err) {
		printk(KERN_ERR "failed to request GPJ26 for camera control\n");

		return err;
	}

	// CAM_VGA_nRST - GPJ2(7)
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPJ27");

	if (err) {
		printk(KERN_ERR "failed to request GPJ27 for camera control\n");

		return err;
	}

	/*
	//GPIO_CAM_PWR_CTRL enable
	err = gpio_request(S5PV210_GPD0(1), "GPD01");
	if(err) {
		printk(KERN_ERR "Failed to request GPD01 GPIO\n");
		return err;
	}
	gpio_direction_output(S5PV210_GPD0(1),  1);
	gpio_set_value(S5PV210_GPD0(1), 1);
	*/
	
	s5k5ccgx_ldo_en(true);

	udelay(20);

	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0x02));

	mdelay(5);

	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	gpio_set_value(GPIO_CAM_VGA_nRST, 1);

	mdelay(7);

	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	udelay(15);

	gpio_direction_output(GPIO_CAM_nSTBY, 0);
	gpio_set_value(GPIO_CAM_nSTBY, 1);

	udelay(20);

	gpio_direction_output(GPIO_CAM_MEGA_nRST, 0);
	gpio_set_value(GPIO_CAM_MEGA_nRST, 1);

	msleep(50);

	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_nSTBY);
	gpio_free(GPIO_CAM_MEGA_nRST);

	return 0;
}

static int s5k5ccgx_power_off(void)
{
	int err;
	printk("%s: start\n", __func__);

	err = gpio_request(GPIO_CAM_nSTBY, "GPJ06");
	if(err) {
		printk(KERN_ERR "Failed to requst GPJ06 GPIO\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_MEGA_nRST, "GPJ15");
	if(err) {
		printk(KERN_ERR "Failed to requst GPJ15 GPIO\n");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPJ26");
	if(err) {
		printk(KERN_ERR "Failed to requst GPJ26");
		return err;
	}

	err = gpio_request(GPIO_CAM_VGA_nRST, "GPJ27");
	if(err) {
		printk(KERN_ERR "Failed to request GPJ27 GPIO\n");
		return err;
	}

	gpio_direction_output(GPIO_CAM_MEGA_nRST, 1);
	gpio_set_value(GPIO_CAM_MEGA_nRST, 0);

	udelay(50);

	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	gpio_direction_output(GPIO_CAM_nSTBY, 1);
	gpio_set_value(GPIO_CAM_nSTBY, 0);

	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);

	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	s5k5ccgx_ldo_en(false);
/*	
	//GPIO_CAM_PWR_CTRL disable
	gpio_direction_output(S5PV210_GPD0(1), 1);
	gpio_set_value(S5PV210_GPD0(1), 0);
	gpio_free(S5PV210_GPD0(1));
*/
	gpio_free(GPIO_CAM_VGA_nSTBY);
	gpio_free(GPIO_CAM_VGA_nRST);

	gpio_free(GPIO_CAM_nSTBY);
	gpio_free(GPIO_CAM_MEGA_nRST);
/*
	s3c_gpio_cfgpin(S5PV210_GPD0(1), S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(S5PV210_GPD0(1), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5PV210_GPD0(1), 0);
*/	
	return 0;
}

static int s5k5ccgx_power_en(int onoff)
{
	int err = 0;
	printk("%s: start. onoff = %d\n", __func__, onoff);
	if (onoff != s5k5ccgx_powered_on) {
		if (onoff)
			err = s5k5ccgx_power_on();
		else {
			err = s5k5ccgx_power_off();
			s3c_i2c0_force_stop();
		}
		if (!err)
			s5k5ccgx_powered_on = onoff;
	}
	return 0;
}

static  int s5k5ccgx_flash(int flash_mode)
{
	int i = 0; 
	int err=0;
	int lux_val = 0;
	printk("%s: start\n", __func__);

	err = gpio_request(GPIO_CAM_FLASH_EN, "GPIO_CAM_FLASH_EN");
	if (err) {
		pr_err("failed to request GPIO_FLASH_EN for camera control\n");
		return err; 
	}    

	err = gpio_request(GPIO_CAM_FLASH_EN_SET, "GPIO_CAM_FLASH_EN_SET");
	if (err) {
		pr_err("failed to request GPIO_CAM_FLASH_EN_SET for camera control\n");
		return err; 
	}    

	gpio_direction_output(GPIO_CAM_FLASH_EN, 0);
	gpio_direction_output(GPIO_CAM_FLASH_EN_SET, 0);
	msleep(1);

	switch(flash_mode) {

		case 1 :  /* FLASH ON  */
			gpio_set_value(GPIO_CAM_FLASH_EN, 1);
			udelay(100);
			lux_val = 2; 
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);

			break;

		case 2 :  /* FLASH ON in AF */

#if 0 /* For the test, hs43.jeon */
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);
#else
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);

			/* data 9 in address 0 */
			lux_val = 15;
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);
			/* address 3 in MovieMode */
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			udelay(1);

			lux_val = 20;
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);

			/* data 1 in address 3 */

			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			udelay(1);
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);

			msleep(1);
#endif
			break;

		case 3 :  /* TORCH ON */
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);                                                                                                                                                  
			/* data 14 in address 0 of MovieMode 30mA */
			/* data 7 in address 0 of Moivemode 69mA */
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			udelay(1);

			lux_val = 7; /* rising edge : 7 */
			for (i = lux_val; i > 1; i--) {
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
				udelay(1);
				gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
				udelay(1);
			}
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 1);
			msleep(1);

			break;

		default : /* FLASH OFF */
			gpio_set_value(GPIO_CAM_FLASH_EN, 0);
			gpio_set_value(GPIO_CAM_FLASH_EN_SET, 0);
			break;

	}

	gpio_free(GPIO_CAM_FLASH_EN_SET);
	gpio_free(GPIO_CAM_FLASH_EN);

	return err;
}

/*
 * Guide for Camera Configuration for Venturi board
 * ITU CAM CH A: S5K5CCGX
*/

/* External camera module setting */
static struct s5k5ccgx_platform_data s5k5ccgx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
	.flash_onoff = s5k5ccgx_flash,
	.power_en = s5k5ccgx_power_en,
};

static struct i2c_board_info  s5k5ccgx_i2c_info = {
	I2C_BOARD_INFO("S5K5CCGX", 0x78>>1),
	.platform_data = &s5k5ccgx_plat,
};

static struct s3c_platform_camera s5k5ccgx = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5k5ccgx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam",//"sclk_cam0",
	.clk_rate	= 24000000,
	.line_length	= 1920,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	// Polarity 
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
	.cam_power	= s5k5ccgx_power_en,
};

#endif

#ifdef CONFIG_VIDEO_S5KA3DFX
/* External camera module setting */
static DEFINE_MUTEX(s5ka3dfx_lock);
static struct regulator *s5ka3dfx_vga_buck4;
static struct regulator *s5ka3dfx_vga_avdd;
static struct regulator *s5ka3dfx_vga_vddio;
static struct regulator *s5ka3dfx_cam_isp_host;
static struct regulator *s5ka3dfx_vga_dvdd;
static bool s5ka3dfx_powered_on;

static int s5ka3dfx_request_gpio(void)
{
	int err;

	/* CAM_VGA_nSTBY - GPB(0) */
	err = gpio_request(GPIO_CAM_VGA_nSTBY, "GPB0");
	if (err) {
		pr_err("Failed to request GPB0 for camera control\n");
		return -EINVAL;
	}

	/* CAM_VGA_nRST - GPB(2) */
	err = gpio_request(GPIO_CAM_VGA_nRST, "GPB2");
	if (err) {
		pr_err("Failed to request GPB2 for camera control\n");
		gpio_free(GPIO_CAM_VGA_nSTBY);
		return -EINVAL;
	}
	/* CAM_IO_EN - GPB(7) */
	/*
	err = gpio_request(GPIO_GPB7, "GPB7");

	if(err) {
		pr_err("Failed to request GPB2 for camera control\n");
		gpio_free(GPIO_CAM_VGA_nSTBY);
		gpio_free(GPIO_CAM_VGA_nRST);
		return -EINVAL;
	}
*/
	return 0;
}

static int s5ka3dfx_power_init(void)
{
	printk("%s: start\n", __func__);
	/* BUCK 4 */
	if (IS_ERR_OR_NULL(s5ka3dfx_vga_buck4))
		s5ka3dfx_vga_buck4 = regulator_get(NULL, "cam_soc_core");

	if (IS_ERR_OR_NULL(s5ka3dfx_vga_buck4)) {
		pr_err("Failed to get regulator vga_avdd\n");
		return -EINVAL;
	}
	/* ldo 15 */
	if (IS_ERR_OR_NULL(s5ka3dfx_vga_vddio))
		s5ka3dfx_vga_vddio = regulator_get(NULL, "cam_soc_a");

	if (IS_ERR_OR_NULL(s5ka3dfx_vga_vddio)) {
		pr_err("Failed to get regulator vga_vddio\n");
		return -EINVAL;
	}

	/* ldo 16 */
	if (IS_ERR_OR_NULL(s5ka3dfx_cam_isp_host))
		s5ka3dfx_cam_isp_host = regulator_get(NULL, "vga_core");

	if (IS_ERR_OR_NULL(s5ka3dfx_cam_isp_host)) {
		pr_err("Failed to get regulator cam_isp_host\n");
		return -EINVAL;
	}

	/* ldo 13 */
	if (IS_ERR_OR_NULL(s5ka3dfx_vga_dvdd))
		s5ka3dfx_vga_dvdd = regulator_get(NULL, "cam_soc_io");

	if (IS_ERR_OR_NULL(s5ka3dfx_vga_dvdd)) {
		pr_err("Failed to get regulator vga_dvdd\n");
		return -EINVAL;
	}

	return 0;
}

static int s5ka3dfx_power_on(void)
{
	int err = 0;
	int result;

	if (s5ka3dfx_power_init()) {
		pr_err("Failed to get all regulator\n");
		return -EINVAL;
	}

	s5ka3dfx_request_gpio();
	/* Turn VGA_AVDD_2.8V on */
	err = regulator_enable(s5ka3dfx_vga_buck4);
	if (err) {
		pr_err("Failed to enable regulator vga_avdd\n");
		return -EINVAL;
	}
	msleep(3);
	// Turn CAM_ISP_SYS_2.8V on
	/*
	gpio_direction_output(GPIO_GPB7, 0);
	gpio_set_value(GPIO_GPB7, 1);
	*/

	mdelay(1);

	/* Turn VGA_VDDIO_2.8V on : ldo 15 */
	err = regulator_enable(s5ka3dfx_vga_vddio);
	if (err) {
		pr_err("Failed to enable regulator vga_vddio\n");
		return -EINVAL;//goto off_vga_vddio;
	}
	udelay(20);

	/* Turn VGA_DVDD_1.8V on : ldo 16 */
	err = regulator_enable(s5ka3dfx_vga_dvdd);
	if (err) {
		pr_err("Failed to enable regulator vga_dvdd\n");
		goto off_vga_dvdd;
	}
	udelay(100);

	udelay(10);
	/* Turn CAM_ISP_HOST_2.8V on : ldo 13*/
	err = regulator_enable(s5ka3dfx_cam_isp_host);
	if (err) {
		pr_err("Failed to enable regulator cam_isp_host\n");
		goto off_cam_isp_host;
	}
	udelay(150);

	/* CAM_VGA_nSTBY HIGH */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 0);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 1);

	/* Mclk enable */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, S3C_GPIO_SFN(0x02));
	udelay(430);

	/* CAM_VGA_nRST HIGH */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 0);
	gpio_set_value(GPIO_CAM_VGA_nRST, 1);
	mdelay(5);

	return 0;
off_cam_isp_host:
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);
	udelay(1);
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);
	udelay(1);
	err = regulator_disable(s5ka3dfx_vga_dvdd);
	if (err) {
		pr_err("Failed to disable regulator vga_dvdd\n");
		result = err;
	}
off_vga_dvdd:
	err = regulator_disable(s5ka3dfx_vga_vddio);
	if (err) {
		pr_err("Failed to disable regulator vga_vddio\n");
		result = err;
	}
off_vga_vddio:
	err = regulator_disable(s5ka3dfx_vga_buck4);
	if (err) {
		pr_err("Failed to disable regulator vga_avdd\n");
		result = err;
	}

	return result;
}

static int s5ka3dfx_power_off(void)
{
	int err;

	if (/*!s5ka3dfx_vga_avdd ||*/ !s5ka3dfx_vga_vddio ||
		!s5ka3dfx_cam_isp_host || !s5ka3dfx_vga_dvdd) {
		pr_err("Faild to get all regulator\n");
		return -EINVAL;
	}

	/* Turn CAM_ISP_HOST_2.8V off */
	err = regulator_disable(s5ka3dfx_cam_isp_host);
	if (err) {
		pr_err("Failed to disable regulator cam_isp_host\n");
		return -EINVAL;
	}

	/* CAM_VGA_nRST LOW */
	gpio_direction_output(GPIO_CAM_VGA_nRST, 1);
	gpio_set_value(GPIO_CAM_VGA_nRST, 0);
	udelay(430);

	/* Mclk disable */
	s3c_gpio_cfgpin(GPIO_CAM_MCLK, 0);

	udelay(1);

	/* Turn VGA_VDDIO_2.8V off */
	err = regulator_disable(s5ka3dfx_vga_vddio);
	if (err) {
		pr_err("Failed to disable regulator vga_vddio\n");
		return -EINVAL;
	}

	/* Turn VGA_DVDD_1.8V off */
	err = regulator_disable(s5ka3dfx_vga_dvdd);
	if (err) {
		pr_err("Failed to disable regulator vga_dvdd\n");
		return -EINVAL;
	}

	/* CAM_VGA_nSTBY LOW */
	gpio_direction_output(GPIO_CAM_VGA_nSTBY, 1);
	gpio_set_value(GPIO_CAM_VGA_nSTBY, 0);

	udelay(1);

	/* Turn VGA_AVDD_2.8V off */
	err = regulator_disable(s5ka3dfx_vga_buck4);
	if (err) {
		pr_err("Failed to disable regulator vga_avdd\n");
		return -EINVAL;
	}

//	gpio_free(GPIO_GPB7);
	gpio_free(GPIO_CAM_VGA_nRST);
	gpio_free(GPIO_CAM_VGA_nSTBY);

	return err;
}

static int s5ka3dfx_power_en(int onoff)
{
	int err = 0;
	mutex_lock(&s5ka3dfx_lock);
	/* we can be asked to turn off even if we never were turned
	 * on if something odd happens and we are closed
	 * by camera framework before we even completely opened.
	 */
	if (onoff != s5ka3dfx_powered_on) {
		if (onoff)
			err = s5ka3dfx_power_on();
		else {
			err = s5ka3dfx_power_off();
			s3c_i2c0_force_stop();
		}
		if (!err)
			s5ka3dfx_powered_on = onoff;
	}
	mutex_unlock(&s5ka3dfx_lock);

	return err;
}

static struct s5ka3dfx_platform_data s5ka3dfx_plat = {
	.default_width = 640,
	.default_height = 480,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,

	.cam_power = s5ka3dfx_power_en,
};

static struct i2c_board_info s5ka3dfx_i2c_info = {
	I2C_BOARD_INFO("S5KA3DFX", 0xc4>>1),
	.platform_data = &s5ka3dfx_plat,
};

static struct s3c_platform_camera s5ka3dfx = {
	.id		= CAMERA_PAR_A,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.i2c_busnum	= 0,
	.info		= &s5ka3dfx_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_name	= "sclk_cam",
	.clk_rate	= 24000000,
	.line_length	= 480,
	.width		= 640,
	.height		= 480,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 640,
		.height	= 480,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized	= 0,
	.cam_power	= s5ka3dfx_power_en,
};
#endif

/* Interface setting */
static struct s3c_platform_fimc fimc_plat_lsi = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_fimc",
	.lclk_name	= "fimc",
	.clk_rate	= 166750000,
	.default_cam	= CAMERA_PAR_A,
	.camera		= {
#ifdef CONFIG_VIDEO_S5K5CCGX
		&s5k5ccgx,
#endif
#ifdef CONFIG_VIDEO_S5K4ECGX
		&s5k4ecgx,
#endif
		&s5ka3dfx,
	},
	.hw_ver		= 0x43,
};

#ifdef CONFIG_VIDEO_JPEG_V2
static struct s3c_platform_jpeg jpeg_plat __initdata = {
	.max_main_width	= 800,
	.max_main_height	= 480,
	.max_thumb_width	= 320,
	.max_thumb_height	= 240,
};
#endif

/* I2C0 */
static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs4[] __initdata = {
#ifdef CONFIG_SND_SMDKC110_MC1N2
	{
		I2C_BOARD_INFO("mc1n2", 0x3A),
	},
#endif
};

/* I2C1 */
static struct i2c_board_info i2c_devs1[] __initdata = {
	{
		I2C_BOARD_INFO("s5p_ddc", (0x74>>1)),
	},
};

#ifdef CONFIG_TOUCHSCREEN_QT602240
/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
	{
		I2C_BOARD_INFO("qt602240_ts", 0x4a),
		.irq = IRQ_EINT_GROUP(18, 5),
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_CYTMA340
static struct i2c_board_info i2c_devs2[] __initdata = {
	{ 
		I2C_BOARD_INFO("cytma340", 0x20), 
	},	
};
#endif

#ifdef CONFIG_TOUCHSCREEN_MXT224
static void mxt224_power_on(void)
{
	gpio_direction_output(GPIO_TOUCH_EN, 1);

	mdelay(40);
}

static void mxt224_power_off(void)
{
	gpio_direction_output(GPIO_TOUCH_EN, 0);
}

#define MXT224_MAX_MT_FINGERS 5

static u8 t7_config[] = {GEN_POWERCONFIG_T7,
				64, 255, 50};
static u8 t8_config[] = {GEN_ACQUISITIONCONFIG_T8,
				7, 0, 5, 0, 0, 0, 9, 35};
static u8 t9_config[] = {TOUCH_MULTITOUCHSCREEN_T9,
				139, 0, 0, 19, 11, 0, 33, 30, 0, 1, 0, 0, 1,
				46, MXT224_MAX_MT_FINGERS, 5, 14, 10, 255, 3,
				255, 3, 18, 18, 10, 10, 141, 65, 143, 110, 18};
static u8 t18_config[] = {SPT_COMCONFIG_T18,
				0, 1};
static u8 t20_config[] = {PROCI_GRIPFACESUPPRESSION_T20,
				7, 0, 0, 0, 0, 0, 0, 80, 40, 4, 35, 10};
static u8 t22_config[] = {PROCG_NOISESUPPRESSION_T22,
				5, 0, 0, 0, 0, 0, 0, 3, 30, 0, 0, 29, 34, 39,
				49, 58, 3};
static u8 t28_config[] = {SPT_CTECONFIG_T28,
				1, 0, 3, 16, 63, 60};
static u8 end_config[] = {RESERVED_T255};

static const u8 *mxt224_config[] = {
	t7_config,
	t8_config,
	t9_config,
	t18_config,
	t20_config,
	t22_config,
	t28_config,
	end_config,
};

static struct mxt224_platform_data mxt224_data = {
	.max_finger_touches = MXT224_MAX_MT_FINGERS,
	.gpio_read_done = GPIO_TOUCH_INT,
	.config = mxt224_config,
	.min_x = 0,
	.max_x = 1023,
	.min_y = 0,
	.max_y = 1023,
	.min_z = 0,
	.max_z = 255,
	.min_w = 0,
	.max_w = 30,
	.power_on = mxt224_power_on,
	.power_off = mxt224_power_off,
};

/* I2C2 */
static struct i2c_board_info i2c_devs2[] __initdata = {
	{
		I2C_BOARD_INFO(MXT224_DEV_NAME, 0x4a),
		.platform_data = &mxt224_data,
		.irq = IRQ_EINT_GROUP(18, 5),
	},
};
#endif

#if defined(CONFIG_KEYPAD_CYPRESS_TOUCH)
/* I2C10 */
static struct i2c_board_info i2c_devs10[] __initdata = {
	{
		I2C_BOARD_INFO(CYPRESS_TOUCHKEY_DEV_NAME, 0x20),
		.platform_data = &touchkey_data,
		.irq = (IRQ_EINT_GROUP22_BASE + 1),
	},
};
#endif

static struct i2c_board_info i2c_devs5[] __initdata = {
/* I2C5 */
#ifdef CONFIG_SENSORS_BMA222
	{
		I2C_BOARD_INFO("bma222",0x08), // [HSS]  0X18 => 0X08 (2010.09.29)
	},
#else
	{
		I2C_BOARD_INFO("bma023", 0x38),
	},
#endif	
};

static struct i2c_board_info i2c_devs8[] __initdata = {
	{
		I2C_BOARD_INFO("Si4709", 0x20 >> 1),
		.irq = (IRQ_EINT_GROUP20_BASE + 4), /* J2_4 */
	},
};

static int fsa9480_init_flag = 0;
static bool mtp_off_status;
static bool jig_status = false;

static void fsa9480_jig_cb(bool attached)
{
	printk("%s: jig callback called! (%d)\n", __func__, attached);
	jig_status = attached;
	return;
}

static void fsa9480_usb_cb(bool attached)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);

	if (gadget) {
		if (attached)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	}

	mtp_off_status = false;

	set_cable_status = attached ? CABLE_TYPE_USB : CABLE_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

static void fsa9480_charger_cb(bool attached)
{
	set_cable_status = attached ? CABLE_TYPE_AC : CABLE_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

static struct switch_dev switch_dock = {
	.name = "dock",
};

static void fsa9480_deskdock_cb(bool attached)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);

	if (attached)
		switch_set_state(&switch_dock, 1);
	else
		switch_set_state(&switch_dock, 0);

	if (gadget) {
		if (attached)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	}

	mtp_off_status = false;

	set_cable_status = attached ? CABLE_TYPE_MISC : CABLE_TYPE_NONE;
	if (charger_callbacks && charger_callbacks->set_cable)
		charger_callbacks->set_cable(charger_callbacks, set_cable_status);
}

static void fsa9480_cardock_cb(bool attached)
{
	if (attached)
		switch_set_state(&switch_dock, 2);
	else
		switch_set_state(&switch_dock, 0);
}

static void fsa9480_reset_cb(void)
{
	int ret;

	/* for CarDock, DeskDock */
	ret = switch_dev_register(&switch_dock);
	if (ret < 0)
		pr_err("Failed to register dock switch. %d\n", ret);
}

static void fsa9480_set_init_flag(void)
{
	fsa9480_init_flag = 1;
}

static struct fsa9480_platform_data fsa9480_pdata = {
	.jig_cb = fsa9480_jig_cb,
	.usb_cb = fsa9480_usb_cb,
	.charger_cb = fsa9480_charger_cb,
	.deskdock_cb = fsa9480_deskdock_cb,
	.cardock_cb = fsa9480_cardock_cb,
	.reset_cb = fsa9480_reset_cb,
	.set_init_flag = fsa9480_set_init_flag,
};

static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("fsa9480", 0x4A >> 1),
		.platform_data = &fsa9480_pdata,
		.irq = IRQ_EINT(23),
	},
};

static struct i2c_board_info i2c_devs6[] __initdata = {
#ifdef CONFIG_REGULATOR_MAX8998
	{
		/* The address is 0xCC used since SRAD = 0 */
		I2C_BOARD_INFO("max8998", (0xCC >> 1)),
		.platform_data	= &max8998_pdata,
		.irq		= IRQ_EINT7,
	}, {
		I2C_BOARD_INFO("rtc_max8998", (0x0D >> 1)),
	},
#endif
};

#ifdef CONFIG_PN544
static struct pn544_i2c_platform_data pn544_pdata = {
	.irq_gpio = NFC_IRQ,
	.ven_gpio = NFC_EN,
	.firm_gpio = NFC_FIRM,
};
static struct i2c_board_info i2c_devs14[] __initdata = {
	{
		I2C_BOARD_INFO("pn544", 0x2b),
		.irq = IRQ_EINT(12),
		.platform_data = &pn544_pdata,
	},
};
#endif

static int max17040_power_supply_register(struct device *parent,
	struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = psy;
	return 0;
}

static void max17040_power_supply_unregister(struct power_supply *psy)
{
	aries_charger.psy_fuelgauge = NULL;
}

static struct max17040_platform_data max17040_pdata = {
	.power_supply_register = max17040_power_supply_register,
	.power_supply_unregister = max17040_power_supply_unregister,
	.rcomp_value = 0xB000,
};

static struct i2c_board_info i2c_devs9[] __initdata = {
	{
		I2C_BOARD_INFO("max17040", (0x6D >> 1)),
		.platform_data = &max17040_pdata,
	},
};

#if defined(CONFIG_OPTICAL_GP2A)
static void gp2a_gpio_init(void)
{
	int ret = gpio_request(GPIO_PS_ON, "gp2a_power_supply_on");
	if (ret)
		printk(KERN_ERR "Failed to request gpio gp2a power supply.\n");
}

static int gp2a_power(bool on)
{
	/* this controls the power supply rail to the gp2a IC */
	gpio_direction_output(GPIO_PS_ON, on);
	return 0;
}

static int gp2a_light_adc_value(void)
{
	return s3c_adc_get_adc_data(9);
}

static struct gp2a_platform_data gp2a_pdata = {
	.power = gp2a_power,
	.p_out = GPIO_PS_VOUT,
	.light_adc_value = gp2a_light_adc_value
};

static struct i2c_board_info i2c_devs11[] __initdata = {
	{
		I2C_BOARD_INFO("gp2a", (0x88 >> 1)),
		.platform_data = &gp2a_pdata,
	},
};
#endif


static struct i2c_board_info i2c_devs12[] __initdata = {
#ifdef CONFIG_SENSORS_MMC328X
	{
		I2C_BOARD_INFO(MMC328X_I2C_NAME,  MMC328X_I2C_ADDR),
	},
#else
	{
		I2C_BOARD_INFO("yas529", 0x2e),
	},
#endif
};

static struct resource ram_console_resource[] = {
	{
		.flags = IORESOURCE_MEM,
	}
};

static struct platform_device ram_console_device = {
	.name = "ram_console",
	.id = -1,
	.num_resources = ARRAY_SIZE(ram_console_resource),
	.resource = ram_console_resource,
};

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
	.name = "pmem",
	.no_allocator = 1,
	.cached = 1,
	.start = 0,
	.size = 0,
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name = "pmem_gpu1",
	.no_allocator = 1,
	.cached = 1,
	.buffered = 1,
	.start = 0,
	.size = 0,
};

static struct android_pmem_platform_data pmem_adsp_pdata = {
	.name = "pmem_adsp",
	.no_allocator = 1,
	.cached = 1,
	.buffered = 1,
	.start = 0,
	.size = 0,
};

static struct platform_device pmem_device = {
	.name = "android_pmem",
	.id = 0,
	.dev = { .platform_data = &pmem_pdata },
};

static struct platform_device pmem_gpu1_device = {
	.name = "android_pmem",
	.id = 1,
	.dev = { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { .platform_data = &pmem_adsp_pdata },
};

static void __init android_pmem_set_platdata(void)
{
	pmem_pdata.start = (u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM, 0);
	pmem_pdata.size = (u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM, 0);

	pmem_gpu1_pdata.start =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_GPU1, 0);
	pmem_gpu1_pdata.size =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_GPU1, 0);

	pmem_adsp_pdata.start =
		(u32)s5p_get_media_memory_bank(S5P_MDEV_PMEM_ADSP, 0);
	pmem_adsp_pdata.size =
		(u32)s5p_get_media_memsize_bank(S5P_MDEV_PMEM_ADSP, 0);
}
#endif

#ifdef CONFIG_CPU_FREQ
static struct s5pv210_cpufreq_voltage smdkc110_cpufreq_volt[] = {
	{
 		.freq	= 1700000,
		.varm	= 1475000,
 		.vint	= 1225000,
 	}, {
 		.freq	= 1600000,
		.varm	= 1425000,
 		.vint	= 1200000,
 	}, {
 		.freq	= 1520000,
		.varm	= 1400000,
 		.vint	= 1200000,
 	}, {
 		.freq	= 1440000,
		.varm	= 1400000,
 		.vint	= 1150000,
 	}, {
 		.freq	= 1400000,
		.varm	= 1350000,
 		.vint	= 1125000,
 	}, {
 		.freq	= 1320000,
		.varm	= 1300000,
 		.vint	= 1125000,
 	}, {
 		.freq	= 1200000,
		.varm	= 1285000,
 		.vint	= 1125000,
 	}, {
		.freq	= 1000000,
		.varm	= 1275000,
		.vint	= 1100000,
	}, {
		.freq	=  800000,
		.varm	= 1200000,
		.vint	= 1100000,
	}, {
		.freq	=  400000,
		.varm	= 1050000,
		.vint	= 1100000,
	},
};

static struct s5pv210_cpufreq_data smdkc110_cpufreq_plat = {
	.volt	= smdkc110_cpufreq_volt,
	.size	= ARRAY_SIZE(smdkc110_cpufreq_volt),
};
#endif

static bool sec_bat_get_jig_status(void)
{
	return jig_status;
}

static enum cable_type_t sec_bat_get_cable_status(void)
{
	return set_cable_status;
}

static struct sec_bat_platform_data sec_bat_pdata = {
	.register_callbacks	= &max8998_charger_register_callbacks,
	.jig_cb = sec_bat_get_jig_status,
	.cable_cb = sec_bat_get_cable_status,
};

struct platform_device sec_device_battery = {
	.name	= "sec-battery",
	.id	= -1,
	.dev.platform_data = &sec_bat_pdata,
};

static int sec_switch_get_cable_status(void)
{
	return mtp_off_status ? CABLE_TYPE_NONE : set_cable_status;
}

static int sec_switch_get_phy_init_status(void)
{
	return fsa9480_init_flag;
}

static void sec_switch_set_vbus_status(u8 mode)
{
	if (mode == USB_VBUS_ALL_OFF)
		mtp_off_status = true;

	if (charger_callbacks && charger_callbacks->set_esafe)
		charger_callbacks->set_esafe(charger_callbacks, mode);
}

static void sec_switch_set_usb_gadget_vbus(bool en)
{
	struct usb_gadget *gadget = platform_get_drvdata(&s3c_device_usbgadget);

	if (gadget) {
		if (en)
			usb_gadget_vbus_connect(gadget);
		else
			usb_gadget_vbus_disconnect(gadget);
	}
}

static struct sec_switch_platform_data sec_switch_pdata = {
	.set_vbus_status = sec_switch_set_vbus_status,
	.set_usb_gadget_vbus = sec_switch_set_usb_gadget_vbus,
	.get_cable_status = sec_switch_get_cable_status,
	.get_phy_init_status = sec_switch_get_phy_init_status,
};

struct platform_device sec_device_switch = {
	.name	= "sec_switch",
	.id	= 1,
	.dev	= {
		.platform_data	= &sec_switch_pdata,
	}
};

static struct platform_device sec_device_rfkill = {
	.name	= "bt_rfkill",
	.id	= -1,
};

static struct platform_device sec_device_btsleep = {
	.name	= "bt_sleep",
	.id	= -1,
};

#ifdef CONFIG_SAMSUNG_JACK
static struct sec_jack_zone sec_jack_zones[] = {
	{
		/* adc == 0, unstable zone, default to 3pole if it stays
		 * in this range for 300ms (15ms delays, 20 samples)
		 */
		.adc_high = 0,
		.delay_ms = 15,
		.check_count = 20,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 0 < adc <= 2500, unstable zone, default to 3pole if it stays
		 * in this range for 800ms (10ms delays, 80 samples)
		 */
		.adc_high = 2500,
		.delay_ms = 10,
		.check_count = 80,
		.jack_type = SEC_HEADSET_3POLE,
	},
	{
		/* 2500 < adc <= 3200, 4 pole zone, default to 4pole if it
		 * stays in this range for 100ms (10ms delays, 10 samples)
		 */
		.adc_high = 3200,
		.delay_ms = 10,
		.check_count = 10,
		.jack_type = SEC_HEADSET_4POLE,
	},
	{
		/* adc > 3100, unstable zone, default to 3pole if it stays
		 * in this range for two seconds (10ms delays, 200 samples)
		 */
		.adc_high = 0x7fffffff,
		.delay_ms = 10,
		.check_count = 200,
		.jack_type = SEC_HEADSET_3POLE,
	},
};

/* Only support one button of earjack in S1_EUR HW.
 * If your HW supports 3-buttons earjack made by Samsung and HTC,
 * add some zones here.
 */
static struct sec_jack_buttons_zone sec_jack_buttons_zones[] = {
	{
		/* 0 <= adc <= 150, stable zone */
		.code		= KEY_MEDIA,
		.adc_low	= 0,
		.adc_high	= 150,
	},
	{
		/* 200 <= adc <= 400, stable zone */
		.code		= KEY_VOLUMEUP,
		.adc_low	= 200,
		.adc_high	= 400,
	},
	{
		/* 600 <= adc <= 800, stable zone */
		.code		= KEY_VOLUMEDOWN,
		.adc_low	= 600,
		.adc_high	= 800,
	},	
};

static int sec_jack_get_adc_value(void)
{
	return s3c_adc_get_adc_data(3);
}

struct sec_jack_platform_data sec_jack_pdata = {
	.set_micbias_state = sec_jack_set_micbias_state,
	.get_adc_value = sec_jack_get_adc_value,
	.zones = sec_jack_zones,
	.num_zones = ARRAY_SIZE(sec_jack_zones),
	.buttons_zones = sec_jack_buttons_zones,
	.num_buttons_zones = ARRAY_SIZE(sec_jack_buttons_zones),
	.det_gpio = GPIO_DET_35,
	.send_end_gpio = GPIO_EAR_SEND_END,
};

static struct platform_device sec_device_jack = {
	.name			= "sec_jack",
	.id			= 1, /* will be used also for gpio_event id */
	.dev.platform_data	= &sec_jack_pdata,
};
#else
static struct sec_jack_port sec_jack_port[] = {
	{
		{ // HEADSET detect info
			.eint       =IRQ_EINT6,
			.gpio       = GPIO_DET_35,   
			.gpio_af    = GPIO_DET_35_AF  ,
			.low_active     = 0
		},
		{ // SEND/END info
			.eint       = IRQ_EINT(30),
			.gpio       = GPIO_EAR_SEND_END,
			.gpio_af    = GPIO_EAR_SEND_END_AF,
			.low_active = 1
		}
	}
};  

static struct sec_jack_platform_data sec_jack_data = {
	.port           = sec_jack_port,
	.nheadsets      = ARRAY_SIZE(sec_jack_port),
};  

static struct platform_device sec_device_jack = {
	.name           = "sec_jack",
	.id             = -1,
	.dev            = {
		.platform_data  = &sec_jack_data,
	},
};  
#endif
#define S5PV210_PS_HOLD_CONTROL_REG (S3C_VA_SYS+0xE81C)
static void aries_power_off(void)
{
	int err;
	int mode = REBOOT_MODE_NONE;
	char reset_mode = 'r';
	int phone_wait_cnt = 0;

	/* Change this API call just before power-off to take the dump. */
	/* kernel_sec_clear_upload_magic_number(); */

	while (1) {
		/* Check reboot charging */
		if (maxim_chg_status()) {
			/* watchdog reset */
			pr_info("%s: charger connected, rebooting\n", __func__);
			mode = REBOOT_MODE_CHARGING;
			if (sec_set_param_value)
				sec_set_param_value(__REBOOT_MODE, &mode);
			//kernel_sec_clear_upload_magic_number();
			//kernel_sec_hw_reset(1);
			arch_reset('r', NULL);
			pr_crit("%s: waiting for reset!\n", __func__);
			while (1);
		}

		//kernel_sec_clear_upload_magic_number();
		/* wait for power button release */
		if (gpio_get_value(GPIO_nPOWER)) {
			pr_info("%s: set PS_HOLD low\n", __func__);

			/* PS_HOLD high  PS_HOLD_CONTROL, R/W, 0xE010_E81C */
			writel(readl(S5PV210_PS_HOLD_CONTROL_REG) & 0xFFFFFEFF,
			       S5PV210_PS_HOLD_CONTROL_REG);

			pr_crit("%s: should not reach here!\n", __func__);
		}

		/* if power button is not released, wait and check TA again */
		pr_info("%s: PowerButton is not released.\n", __func__);
		mdelay(1000);
	}
}

static void config_gpio_table(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		if (gpio_table[i][2] != S3C_GPIO_SETPIN_NONE)
			gpio_set_value(gpio, gpio_table[i][2]);
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
	}
}

static void config_sleep_gpio_table(int array_size, unsigned int (*gpio_table)[3])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_slp_cfgpin(gpio, gpio_table[i][1]);
		s3c_gpio_slp_setpull_updown(gpio, gpio_table[i][2]);
	}
}

static void config_init_gpio(void)
{
	config_gpio_table(ARRAY_SIZE(initial_gpio_table), initial_gpio_table);
}

void config_sleep_gpio(void)
{
	config_gpio_table(ARRAY_SIZE(sleep_alive_gpio_table), sleep_alive_gpio_table);
	config_sleep_gpio_table(ARRAY_SIZE(sleep_gpio_table), sleep_gpio_table);

#if defined(CONFIG_OPTICAL_GP2A)
	if (gpio_get_value(GPIO_PS_ON)) {
		s3c_gpio_slp_setpull_updown(GPIO_ALS_SDA_28V, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_setpull_updown(GPIO_ALS_SCL_28V, S3C_GPIO_PULL_NONE);
	} else {
		s3c_gpio_setpull(GPIO_PS_VOUT, S3C_GPIO_PULL_DOWN);
	}
#endif

	printk(KERN_DEBUG "SLPGPIO : BT(%d) WLAN(%d) BT+WIFI(%d)\n",
		gpio_get_value(GPIO_BT_nRST), gpio_get_value(GPIO_WLAN_nRST), gpio_get_value(GPIO_WLAN_BT_EN));

#if !defined(CONFIG_SND_SMDKC110_MC1N2)
	printk(KERN_DEBUG "SLPGPIO : CODEC_LDO_EN(%d) MICBIAS_EN(%d) GPIO_MICBIAS_EN2(%d)\n",
		gpio_get_value(GPIO_CODEC_LDO_EN), gpio_get_value(GPIO_MICBIAS_EN), gpio_get_value(GPIO_MICBIAS_EN2));
#endif

#if defined(CONFIG_OPTICAL_GP2A)
	printk(KERN_DEBUG "SLPGPIO : PS_ON(%d) FM_RST(%d) UART_SEL(%d)\n",
		gpio_get_value(GPIO_PS_ON), gpio_get_value(GPIO_FM_RST), gpio_get_value(GPIO_UART_SEL));
#endif
}
EXPORT_SYMBOL(config_sleep_gpio);

static unsigned int wlan_sdio_on_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, GPIO_WLAN_SDIO_D0_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, GPIO_WLAN_SDIO_D1_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, GPIO_WLAN_SDIO_D2_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, GPIO_WLAN_SDIO_D3_AF, GPIO_LEVEL_NONE,
		S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_off_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static int wlan_power_en(int onoff)
{
	if (onoff) {
		s3c_gpio_cfgpin(GPIO_WLAN_HOST_WAKE,
				S3C_GPIO_SFN(GPIO_WLAN_HOST_WAKE_AF));
		s3c_gpio_setpull(GPIO_WLAN_HOST_WAKE, S3C_GPIO_PULL_DOWN);

		s3c_gpio_cfgpin(GPIO_WLAN_WAKE,
				S3C_GPIO_SFN(GPIO_WLAN_WAKE_AF));
		s3c_gpio_setpull(GPIO_WLAN_WAKE, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_WAKE, GPIO_LEVEL_LOW);

		s3c_gpio_cfgpin(GPIO_WLAN_nRST,
				S3C_GPIO_SFN(GPIO_WLAN_nRST_AF));
		s3c_gpio_setpull(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);

		s3c_gpio_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(GPIO_WLAN_BT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN,
					S3C_GPIO_PULL_NONE);

		msleep(200);
	} else {
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_WLAN_nRST, S3C_GPIO_PULL_NONE);

		if (gpio_get_value(GPIO_BT_nRST) == 0) {
			gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);
			s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
			s3c_gpio_slp_setpull_updown(GPIO_WLAN_BT_EN,
						S3C_GPIO_PULL_NONE);
		}
	}
	return 0;
}

static int wlan_reset_en(int onoff)
{
	gpio_set_value(GPIO_WLAN_nRST,
			onoff ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW);
	return 0;
}

static int wlan_carddetect_en(int onoff)
{
	u32 i;
	u32 sdio;

	if (onoff) {
		for (i = 0; i < ARRAY_SIZE(wlan_sdio_on_table); i++) {
			sdio = wlan_sdio_on_table[i][0];
			s3c_gpio_cfgpin(sdio,
					S3C_GPIO_SFN(wlan_sdio_on_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_on_table[i][3]);
			if (wlan_sdio_on_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_on_table[i][2]);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(wlan_sdio_off_table); i++) {
			sdio = wlan_sdio_off_table[i][0];
			s3c_gpio_cfgpin(sdio,
				S3C_GPIO_SFN(wlan_sdio_off_table[i][1]));
			s3c_gpio_setpull(sdio, wlan_sdio_off_table[i][3]);
			if (wlan_sdio_off_table[i][2] != GPIO_LEVEL_NONE)
				gpio_set_value(sdio, wlan_sdio_off_table[i][2]);
		}
	}
	udelay(5);

	sdhci_s3c_force_presence_change(&s3c_device_hsmmc3);
	msleep(500); /* wait for carddetect */
	return 0;
}

static struct resource wifi_resources[] = {
	[0] = {
		.name	= "bcm4329_wlan_irq",
		.start	= IRQ_EINT(20),
		.end	= IRQ_EINT(20),
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct wifi_mem_prealloc wifi_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

static void *aries_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;

	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wifi_mem_array[section].size < size)
		return NULL;

	return wifi_mem_array[section].mem_ptr;
}

#define DHD_SKB_HDRSIZE 		336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)
int __init aries_init_wifi_mem(void)
{
	int i;
	int j;

	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}
	
	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}
	
	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0 ; i < PREALLOC_WLAN_SEC_NUM ; i++) {
		wifi_mem_array[i].mem_ptr =
				kmalloc(wifi_mem_array[i].size, GFP_KERNEL);

		if (!wifi_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}
	return 0;

 err_mem_alloc:
	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		kfree(wifi_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

 err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}

/* Customized Locale table : OPTIONAL feature */
#define WLC_CNTRY_BUF_SZ	4
typedef struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];
	char custom_locale[WLC_CNTRY_BUF_SZ];
	int  custom_locale_rev;
} cntry_locales_custom_t;

static cntry_locales_custom_t aries_wifi_translate_custom_table[] = {
/* Table should be filled out based on custom platform regulatory requirement */
	{"",   "XY", 4},  /* universal */
	{"US", "US", 69}, /* input ISO "US" to : US regrev 69 */
	{"CA", "US", 69}, /* input ISO "CA" to : US regrev 69 */
	{"EU", "EU", 5},  /* European union countries */
	{"AT", "EU", 5},
	{"BE", "EU", 5},
	{"BG", "EU", 5},
	{"CY", "EU", 5},
	{"CZ", "EU", 5},
	{"DK", "EU", 5},
	{"EE", "EU", 5},
	{"FI", "EU", 5},
	{"FR", "EU", 5},
	{"DE", "EU", 5},
	{"GR", "EU", 5},
	{"HU", "EU", 5},
	{"IE", "EU", 5},
	{"IT", "EU", 5},
	{"LV", "EU", 5},
	{"LI", "EU", 5},
	{"LT", "EU", 5},
	{"LU", "EU", 5},
	{"MT", "EU", 5},
	{"NL", "EU", 5},
	{"PL", "EU", 5},
	{"PT", "EU", 5},
	{"RO", "EU", 5},
	{"SK", "EU", 5},
	{"SI", "EU", 5},
	{"ES", "EU", 5},
	{"SE", "EU", 5},
	{"GB", "EU", 5},  /* input ISO "GB" to : EU regrev 05 */
	{"IL", "IL", 0},
	{"CH", "CH", 0},
	{"TR", "TR", 0},
	{"NO", "NO", 0},
	{"KR", "XY", 3},
	{"AU", "XY", 3},
	{"CN", "XY", 3},  /* input ISO "CN" to : XY regrev 03 */
	{"TW", "XY", 3},
	{"AR", "XY", 3},
	{"MX", "XY", 3}
};

static void *aries_wifi_get_country_code(char *ccode)
{
	int size = ARRAY_SIZE(aries_wifi_translate_custom_table);
	int i;

	if (!ccode)
		return NULL;

	for (i = 0; i < size; i++)
		if (strcmp(ccode, aries_wifi_translate_custom_table[i].iso_abbrev) == 0)
			return &aries_wifi_translate_custom_table[i];
	return &aries_wifi_translate_custom_table[0];
}

static struct wifi_platform_data wifi_pdata = {
	.set_power		= wlan_power_en,
	.set_reset		= wlan_reset_en,
	.set_carddetect		= wlan_carddetect_en,
	.mem_prealloc		= aries_mem_prealloc,
	.get_country_code	= aries_wifi_get_country_code,
};

static struct platform_device sec_device_wifi = {
	.name			= "bcm4329_wlan",
	.id			= 1,
	.num_resources		= ARRAY_SIZE(wifi_resources),
	.resource		= wifi_resources,
	.dev			= {
		.platform_data = &wifi_pdata,
	},
};

static struct platform_device watchdog_device = {
	.name = "watchdog",
	.id = -1,
};

static struct platform_device *aries_devices[] __initdata = {
	&watchdog_device,
#ifdef CONFIG_FIQ_DEBUGGER
	&s5pv210_device_fiqdbg_uart2,
#endif
	&s5p_device_onenand,
#ifdef CONFIG_RTC_DRV_S3C
	&s5p_device_rtc,
#endif
	&aries_input_device,
//	&s3c_device_keypad,

	&s5pv210_device_iis0,
	&s5pv210_device_pcm1,
	&s3c_device_wdt,

#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif

#ifdef CONFIG_VIDEO_MFC50
	&s3c_device_mfc,
#endif
#ifdef	CONFIG_S5P_ADC
	&s3c_device_adc,
#endif
#ifdef CONFIG_VIDEO_FIMC
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
#endif

#ifdef CONFIG_VIDEO_JPEG_V2
	&s3c_device_jpeg,
#endif

	&s3c_device_g3d,
	&s3c_device_lcd,

	&s3c_device_spi_gpio,

	&sec_device_jack,

	&s3c_device_i2c0,
#if defined(CONFIG_S3C_DEV_I2C1)
	&s3c_device_i2c1,
#endif

#if defined(CONFIG_S3C_DEV_I2C2)
	&s3c_device_i2c2,
#endif
	&venturi_i2c4_device,
	&venturi_i2c5_device,  /* accel sensor */
	&venturi_i2c6_device,
	&venturi_i2c7_device,
	&venturi_i2c8_device,  /* FM radio */
#if defined(CONFIG_BATTERY_MAX17040)
	&venturi_i2c9_device,  /* max1704x:fuel_guage */
#endif
#if defined(CONFIG_OPTICAL_GP2A)
	&venturi_i2c11_device, /* optical sensor */
#endif
	&venturi_i2c12_device, /* magnetic sensor */
#ifdef CONFIG_PN544
	&venturi_i2c14_device, /* nfc sensor */
#endif

#ifdef CONFIG_USB_GADGET
	&s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID
	&s3c_device_android_usb,
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	&s3c_device_usb_mass_storage,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	&s3c_device_rndis,
#endif
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	&s3c_device_hsmmc3,
#endif
#ifdef CONFIG_VIDEO_TV20
        &s5p_device_tvout,
        //&s5p_device_hpd,
        //&s5p_device_cec,
#endif
	&sec_device_battery,
#if defined(CONFIG_KEYPAD_CYPRESS_TOUCH)
	&s3c_device_i2c10,
#endif
	&sec_device_switch,  // samsung switch driver

#ifdef CONFIG_S5PV210_POWER_DOMAIN
	&s5pv210_pd_audio,
	&s5pv210_pd_cam,
	&s5pv210_pd_tv,
	&s5pv210_pd_lcd,
	&s5pv210_pd_g3d,
	&s5pv210_pd_mfc,
#endif

#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_adsp_device,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
	&s3c_device_timer[2],
	&s3c_device_timer[3],
#endif

#ifdef CONFIG_CPU_FREQ
	&s5pv210_device_cpufreq,
#endif

// VenturiGB_Usys_jypark 2011.08.08 - DMB [[
#ifdef CONFIG_S3C64XX_DEV_SPI
 &s5pv210_device_spi0,
// &s5pv210_device_spi1,
#endif
// VenturiGB_Usys_jypark 2011.08.08 - DMB ]]

	&sec_device_rfkill,
	&sec_device_btsleep,
	&ram_console_device,

	//JPC&s5p_device_ace,
#ifdef CONFIG_SND_S5P_RP
	&s5p_device_rp,
#endif
	&sec_device_wifi,
};


static void __init aries_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s5pv210_gpiolib_init();
	s3c24xx_init_uarts(aries_uartcfgs, ARRAY_SIZE(aries_uartcfgs));
#ifndef CONFIG_S5P_HIGH_RES_TIMERS
	s5p_set_timer_source(S5P_PWM3, S5P_PWM4);
#endif

	s5p_reserve_bootmem(aries_media_devs,
 			ARRAY_SIZE(aries_media_devs), S5P_RANGE_MFC);
#ifdef CONFIG_MTD_ONENAND
	s5pc110_device_onenand.name = "s5pc110-onenand";
#endif
}

unsigned int pm_debug_scratchpad;

static unsigned int ram_console_start;
static unsigned int ram_console_size;

static void __init aries_fixup(struct machine_desc *desc,
		struct tag *tags, char **cmdline,
		struct meminfo *mi)
{
	mi->bank[0].start = 0x30000000;
	mi->bank[0].size = 256 * SZ_1M;

	mi->bank[1].start = 0x40000000;
	mi->bank[1].size = 255 * SZ_1M;

	mi->nr_banks = 2;

	ram_console_start = mi->bank[1].start + mi->bank[1].size;
	ram_console_size = SZ_1M - SZ_4K;

	pm_debug_scratchpad = ram_console_start + ram_console_size;
}

/* this function are used to detect s5pc110 chip version temporally */
int s5pc110_version ;

void _hw_version_check(void)
{
	void __iomem *phy_address ;
	int temp;

	phy_address = ioremap(0x40, 1);

	temp = __raw_readl(phy_address);

	if (temp == 0xE59F010C)
		s5pc110_version = 0;
	else
		s5pc110_version = 1;

	printk(KERN_INFO "S5PC110 Hardware version : EVT%d\n",
				s5pc110_version);

	iounmap(phy_address);
}

/*
 * Temporally used
 * return value 0 -> EVT 0
 * value 1 -> evt 1
 */

int hw_version_check(void)
{
	return s5pc110_version ;
}
EXPORT_SYMBOL(hw_version_check);

static void __init fsa9480_gpio_init(void)
{
	s3c_gpio_cfgpin(GPIO_USB_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_USB_SEL, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_JACK_nINT, S3C_GPIO_SFN(0xf));
	s3c_gpio_setpull(GPIO_JACK_nINT, S3C_GPIO_PULL_NONE);
}

static void __init setup_ram_console_mem(void)
{
	ram_console_resource[0].start = ram_console_start;
	ram_console_resource[0].end = ram_console_start + ram_console_size - 1;
}

static void __init sound_init(void)
{
	u32 reg;

	reg = __raw_readl(S5P_OTHERS);
	reg &= ~(0x3 << 8);
	reg |= 3 << 8;
	__raw_writel(reg, S5P_OTHERS);

	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~(0x1f << 12);
	reg |= 19 << 12;
	__raw_writel(reg, S5P_CLK_OUT);

	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~0x1;
	reg |= 0x1;
	__raw_writel(reg, S5P_CLK_OUT);

#if !defined(CONFIG_SND_SMDKC110_MC1N2)
	gpio_request(GPIO_MICBIAS_EN, "micbias_enable");
	gpio_request(GPIO_MICBIAS_EN2, "sub_micbias_enable");
#endif
	gpio_request(GPIO_MUTE_ON, "earpath_enable");
}

static void __init onenand_init()
{
	struct clk *clk = clk_get(NULL, "onenand");
	BUG_ON(!clk);
	clk_enable(clk);
}

static void __init aries_machine_init(void)
{
	setup_ram_console_mem();
	//s3c_usb_set_serial();
	platform_add_devices(aries_devices, ARRAY_SIZE(aries_devices));

	/* Find out S5PC110 chip version */
	_hw_version_check();

	pm_power_off = aries_power_off ;

	s3c_gpio_cfgpin(GPIO_HWREV_MODE0, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE0, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE1, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE1, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE2, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE2, S3C_GPIO_PULL_NONE);
	HWREV = gpio_get_value(GPIO_HWREV_MODE0);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE1) << 1);
	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE2) << 2);
	s3c_gpio_cfgpin(GPIO_HWREV_MODE3, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_HWREV_MODE3, S3C_GPIO_PULL_NONE);

	HWREV = HWREV | (gpio_get_value(GPIO_HWREV_MODE3) << 3);
	printk(KERN_INFO "HWREV is 0x%x\n", HWREV);

	/*initialise the gpio's*/
	config_init_gpio();

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif

	/* i2c */
	s3c_i2c0_set_platdata(NULL);
#ifdef CONFIG_S3C_DEV_I2C1
	s3c_i2c1_set_platdata(NULL);
#endif

#ifdef CONFIG_S3C_DEV_I2C2
	s3c_i2c2_set_platdata(NULL);
#endif
	/* H/W I2C lines */
	if (system_rev >= 0x05) {
		/* gyro sensor */
		i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
		/* magnetic and accel sensor */
		i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	}
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));

	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
	/* accel sensor */
	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5));
	i2c_register_board_info(6, i2c_devs6, ARRAY_SIZE(i2c_devs6));
	/* Touch Key */
//	touch_keypad_gpio_init();
//	i2c_register_board_info(10, i2c_devs10, ARRAY_SIZE(i2c_devs10));
	/* FSA9480 */
	fsa9480_gpio_init();
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7));

	/* FM Radio */
	i2c_register_board_info(8, i2c_devs8, ARRAY_SIZE(i2c_devs8));

	i2c_register_board_info(9, i2c_devs9, ARRAY_SIZE(i2c_devs9));

	/* optical sensor */
#if defined(CONFIG_OPTICAL_GP2A)
	gp2a_gpio_init();
	i2c_register_board_info(11, i2c_devs11, ARRAY_SIZE(i2c_devs11));
#endif
	/* magnetic sensor */

	i2c_register_board_info(12, i2c_devs12, ARRAY_SIZE(i2c_devs12));
	/* nfc sensor */
#ifdef CONFIG_PN544
	i2c_register_board_info(14, i2c_devs14, ARRAY_SIZE(i2c_devs14));
#endif

// VenturiGB_Usys_jypark 2011.08.08 - DMB [[
 /* spi */
#ifdef CONFIG_S3C64XX_DEV_SPI
    if (!gpio_request(S5PV210_GPB(1), "SPI_CS0")) {
//        s3cspi_set_slaves(BUSNUM(0), ARRAY_SIZE(s3c_slv_pdata_0), s3c_slv_pdata_0);
            gpio_direction_output(S5PV210_GPB(1), 1);
            s3c_gpio_cfgpin(S5PV210_GPB(1), S3C_GPIO_SFN(1));
            s3c_gpio_setpull(S5PV210_GPB(1), S3C_GPIO_PULL_UP);
            s5pv210_spi_set_info(0, S5PV210_SPI_SRCCLK_PCLK,
            ARRAY_SIZE(smdk_spi0_csi));
    }
/*    
    if (!gpio_request(S5PV210_GPB(5), "SPI_CS1")) {
        gpio_direction_output(S5PV210_GPB(5), 1);
        s3c_gpio_cfgpin(S5PV210_GPB(5), S3C_GPIO_SFN(1));
        s3c_gpio_setpull(S5PV210_GPB(5), S3C_GPIO_PULL_UP);
        s5pv210_spi_set_info(1, S5PV210_SPI_SRCCLK_PCLK,
        ARRAY_SIZE(smdk_spi1_csi));
    }
*/    
    spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));
#endif
// VenturiGB_Usys_jypark 2011.08.08 - DMB ]]

#ifdef CONFIG_FB_S3C_TL2796
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&tl2796_data);
#endif

#ifdef CONFIG_FB_S3C_HX8369
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&hx8369_data);
#endif

#if defined(CONFIG_S5P_ADC)
	s3c_adc_set_platdata(&s3c_adc_platform);
#endif

#if defined(CONFIG_PM)
	s3c_pm_init();
#endif

#ifdef CONFIG_VIDEO_FIMC
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat_lsi);
	s3c_fimc1_set_platdata(&fimc_plat_lsi);
	s3c_fimc2_set_platdata(&fimc_plat_lsi);
#endif

#ifdef CONFIG_VIDEO_JPEG_V2
	s3c_jpeg_set_platdata(&jpeg_plat);
#endif

#ifdef CONFIG_VIDEO_MFC50
	/* mfc */
	s3c_mfc_set_platdata(NULL);
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
	s5pv210_default_sdhci0();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
	s5pv210_default_sdhci1();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
	s5pv210_default_sdhci2();
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
	s5pv210_default_sdhci3();
#endif
#ifdef CONFIG_S5PV210_SETUP_SDHCI
	s3c_sdhci_set_platdata();
#endif

#ifdef CONFIG_CPU_FREQ
	s5pv210_cpufreq_set_platdata(&smdkc110_cpufreq_plat);
#endif

	regulator_has_full_constraints();

	register_reboot_notifier(&aries_reboot_notifier);

	aries_switch_init();

	gps_gpio_init();

	aries_init_wifi_mem();

//	onenand_init();

	#ifdef CONFIG_USB_ANDROID_SAMSUNG_COMPOSITE
/* soonyong.cho : This is for setting unique serial number */
	s3c_usb_set_serial();
	#endif

	if (gpio_is_valid(GPIO_MSENSE_nRST)) {
		if (gpio_request(GPIO_MSENSE_nRST, "GPB"))
			printk(KERN_ERR "Failed to request GPIO_MSENSE_nRST!\n");
		gpio_direction_output(GPIO_MSENSE_nRST, 1);
	}
	gpio_free(GPIO_MSENSE_nRST);
}

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void)
{
	/* USB PHY0 Enable */
	writel(readl(S5P_USB_PHY_CONTROL) | (0x1<<0),
			S5P_USB_PHY_CONTROL);
	writel((readl(S3C_USBOTG_PHYPWR) & ~(0x3<<3) & ~(0x1<<0)) | (0x1<<5),
			S3C_USBOTG_PHYPWR);
	writel((readl(S3C_USBOTG_PHYCLK) & ~(0x5<<2)) | (0x3<<0),
			S3C_USBOTG_PHYCLK);
	writel((readl(S3C_USBOTG_RSTCON) & ~(0x3<<1)) | (0x1<<0),
			S3C_USBOTG_RSTCON);
	msleep(1);
	writel(readl(S3C_USBOTG_RSTCON) & ~(0x7<<0),
			S3C_USBOTG_RSTCON);
	msleep(1);

	/* rising/falling time */
	writel(readl(S3C_USBOTG_PHYTUNE) | (0x1<<20),
			S3C_USBOTG_PHYTUNE);

	/* set DC level as 0xf (24%) */
	writel(readl(S3C_USBOTG_PHYTUNE) | 0xf, S3C_USBOTG_PHYTUNE);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(64)));

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	writel(readl(S3C_USBOTG_PHYPWR) | (0x3<<3),
			S3C_USBOTG_PHYPWR);
	writel(readl(S5P_USB_PHY_CONTROL) & ~(1<<0),
			S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "otg");
	clk_enable(otg_clk);

	if (readl(S5P_USB_PHY_CONTROL) & (0x1<<1))
		return;

	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) | (0x1<<1),
			S5P_USB_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
			& ~(0x1<<7) & ~(0x1<<6)) | (0x1<<8) | (0x1<<5),
			S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) & ~(0x1<<7)) | (0x3<<0),
			S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON)) | (0x1<<4) | (0x1<<3),
			S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON) & ~(0x1<<4) & ~(0x1<<3),
			S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);


void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR) | (0x1<<7)|(0x1<<6),
			S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USB_PHY_CONTROL) & ~(1<<1),
			S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(usb_host_phy_off);
#endif

MACHINE_START(SMDKC110, "venturi")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.fixup		= aries_fixup,
	.init_irq	= s5pv210_init_irq,
	.map_io		= aries_map_io,
	.init_machine	= aries_machine_init,
#ifdef CONFIG_S5P_HIGH_RES_TIMERS
        .timer          = &s5p_systimer,
#else
        .timer          = &s5p_timer,
#endif
MACHINE_END

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
	switch (port) {
	case 0:
		s3c_gpio_cfgpin(GPIO_BT_RXD, S3C_GPIO_SFN(GPIO_BT_RXD_AF));
		s3c_gpio_setpull(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_TXD, S3C_GPIO_SFN(GPIO_BT_TXD_AF));
		s3c_gpio_setpull(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_CTS, S3C_GPIO_SFN(GPIO_BT_CTS_AF));
		s3c_gpio_setpull(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_RTS, S3C_GPIO_SFN(GPIO_BT_RTS_AF));
		s3c_gpio_setpull(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_TXD, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_CTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_slp_cfgpin(GPIO_BT_RTS, S3C_GPIO_SLP_PREV);
		s3c_gpio_slp_setpull_updown(GPIO_BT_RTS, S3C_GPIO_PULL_NONE);
		break;
	case 1:
		s3c_gpio_cfgpin(GPIO_GPS_RXD, S3C_GPIO_SFN(GPIO_GPS_RXD_AF));
		s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
		s3c_gpio_cfgpin(GPIO_GPS_TXD, S3C_GPIO_SFN(GPIO_GPS_TXD_AF));
		s3c_gpio_setpull(GPIO_GPS_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_SFN(GPIO_GPS_CTS_AF));
		s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_SFN(GPIO_GPS_RTS_AF));
		s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_NONE);
		break;
	case 2:
		s3c_gpio_cfgpin(GPIO_AP_RXD, S3C_GPIO_SFN(GPIO_AP_RXD_AF));
		s3c_gpio_setpull(GPIO_AP_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_TXD, S3C_GPIO_SFN(GPIO_AP_TXD_AF));
		s3c_gpio_setpull(GPIO_AP_TXD, S3C_GPIO_PULL_NONE);
		break;
	case 3:
		s3c_gpio_cfgpin(GPIO_FLM_RXD, S3C_GPIO_SFN(GPIO_FLM_RXD_AF));
		s3c_gpio_setpull(GPIO_FLM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_FLM_TXD, S3C_GPIO_SFN(GPIO_FLM_TXD_AF));
		s3c_gpio_setpull(GPIO_FLM_TXD, S3C_GPIO_PULL_NONE);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);
