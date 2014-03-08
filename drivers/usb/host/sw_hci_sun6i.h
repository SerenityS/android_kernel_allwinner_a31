/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_hci_sun6i.h
*
* Author 		: yangnaitian
*
* Description 	: Include file for AW1623 HCI Host Controller Driver
*
* Notes         :
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*    yangnaitian      2011-5-24            1.0          create this file
*
*************************************************************************************
*/

#ifndef __SW_HCI_SUN6I_H__
#define __SW_HCI_SUN6I_H__

#include <linux/delay.h>
#include <linux/types.h>

#include <linux/io.h>
#include <linux/irq.h>

#define  DMSG_ERR(format,args...)   			printk("[sw_hci_sun6i]: "format,##args)
#define  DMSG_PRINT(format,args...)   	    	printk("[sw_hci_sun6i]: "format,##args)


#if 0
    #define DMSG_DEBUG         		DMSG_PRINT
#else
    #define DMSG_DEBUG(...)
#endif

#if 1
    #define DMSG_INFO         		DMSG_PRINT
#else
    #define DMSG_INFO(...)
#endif

#if	1
    #define DMSG_PANIC        		DMSG_ERR
#else
    #define DMSG_PANIC(...)
#endif


//---------------------------------------------------------------
//  宏 定义
//---------------------------------------------------------------
#define  USBC_Readb(reg)	                    (*(volatile unsigned char *)(reg))
#define  USBC_Readw(reg)	                    (*(volatile unsigned short *)(reg))
#define  USBC_Readl(reg)	                    (*(volatile unsigned long *)(reg))

#define  USBC_Writeb(value, reg)                (*(volatile unsigned char *)(reg) = (value))
#define  USBC_Writew(value, reg)	            (*(volatile unsigned short *)(reg) = (value))
#define  USBC_Writel(value, reg)	            (*(volatile unsigned long *)(reg) = (value))

#define  USBC_REG_test_bit_b(bp, reg)         	(USBC_Readb(reg) & (1 << (bp)))
#define  USBC_REG_test_bit_w(bp, reg)   	    (USBC_Readw(reg) & (1 << (bp)))
#define  USBC_REG_test_bit_l(bp, reg)   	    (USBC_Readl(reg) & (1 << (bp)))

#define  USBC_REG_set_bit_b(bp, reg) 			(USBC_Writeb((USBC_Readb(reg) | (1 << (bp))) , (reg)))
#define  USBC_REG_set_bit_w(bp, reg) 	 		(USBC_Writew((USBC_Readw(reg) | (1 << (bp))) , (reg)))
#define  USBC_REG_set_bit_l(bp, reg) 	 		(USBC_Writel((USBC_Readl(reg) | (1 << (bp))) , (reg)))

#define  USBC_REG_clear_bit_b(bp, reg)	 	 	(USBC_Writeb((USBC_Readb(reg) & (~ (1 << (bp)))) , (reg)))
#define  USBC_REG_clear_bit_w(bp, reg)	 	 	(USBC_Writew((USBC_Readw(reg) & (~ (1 << (bp)))) , (reg)))
#define  USBC_REG_clear_bit_l(bp, reg)	 	 	(USBC_Writel((USBC_Readl(reg) & (~ (1 << (bp)))) , (reg)))

//-----------------------------------------------------------------------
//   USB register
//-----------------------------------------------------------------------
#define SW_USB_EHCI_BASE_OFFSET		0x00
#define SW_USB_OHCI_BASE_OFFSET		0x400
#define SW_USB_EHCI_LEN      		0x58
#define SW_USB_OHCI_LEN      		0x58

#define SW_USB_PMU_IRQ_ENABLE		0x800

#define  SW_USB_HCI_DEBUG
#if defined (CONFIG_AW_FPGA_V4_PLATFORM) || defined (CONFIG_AW_FPGA_V7_PLATFORM)
#define SW_USB_FPGA
#endif

enum sw_usbc_type{
	SW_USB_UNKOWN = 0,
	SW_USB_EHCI,
	SW_USB_OHCI,
};

struct sw_hci_hcd{
	__u32 usbc_no;						/* usb controller number */
	__u32 irq_no;						/* interrupt number 	*/
	char hci_name[32];                  /* hci name             */

	struct resource	*usb_base_res;   	/* USB  resources 		*/
	struct resource	*usb_base_req;   	/* USB  resources 		*/
	void __iomem	*usb_vbase;			/* USB  base address 	*/

	void __iomem	* ehci_base;
	__u32 ehci_reg_length;
	void __iomem	* ohci_base;
	__u32 ohci_reg_length;

	struct resource	*sram_base_res;   	/* SRAM resources 		*/
	struct resource	*sram_base_req;   	/* SRAM resources 		*/
	void __iomem	*sram_vbase;		/* SRAM base address 	*/
	__u32 sram_reg_start;
	__u32 sram_reg_length;

	struct resource	*clock_base_res;   	/* clock resources 		*/
	struct resource	*clock_base_req;   	/* clock resources 		*/
	void __iomem	*clock_vbase;		/* clock base address 	*/
	__u32 clock_reg_start;
	__u32 clock_reg_length;

	struct resource	*gpio_base_res;   	/* gpio resources 		*/
	struct resource	*gpio_base_req;   	/* gpio resources 		*/
	void __iomem	*gpio_vbase;		/* gpio base address 	*/
	__u32 gpio_reg_start;
	__u32 gpio_reg_length;

	struct resource	*sdram_base_res;   	/* sdram resources 		*/
	struct resource	*sdram_base_req;   	/* sdram resources 		*/
	void __iomem	*sdram_vbase;		/* sdram base address 	*/
	__u32 sdram_reg_start;
	__u32 sdram_reg_length;

	struct platform_device *pdev;
	struct usb_hcd *hcd;

	struct clk	*ahb;				/* ahb clock handle 	*/
	struct clk	*mod_usb;			/* mod_usb otg clock handle 	*/
	struct clk	*mod_usbphy;			/* PHY0 clock handle 	*/
	__u32 clk_is_open;					/* is usb clock open 	*/

	script_item_u drv_vbus_gpio_set;
	script_item_u restrict_gpio_set;
	u32 drv_vbus_gpio_valid;
	u32 usb_restrict_valid;
	__u8 power_flag;                    /* flag. 是否供电       */

    __u8 used;                          /* flag. 控制器是否被使用 */
	__u8 probe;                         /* 控制器初始化 */
	__u8 host_init_state;				/* usb 控制器的初始化状态。0 : 不工作. 1 : 工作 */
	__u8 usb_restrict_flag;
	__u8 usbc_type;                     /* usb controller type  */
	__u8 not_suspend;                   /* flag. 不休眠         */

	int (* open_clock)(struct sw_hci_hcd *sw_hci, u32 ohci);
	int (* close_clock)(struct sw_hci_hcd *sw_hci, u32 ohci);
    void (* set_power)(struct sw_hci_hcd *sw_hci, int is_on);
	void (* port_configure)(struct sw_hci_hcd *sw_hci, u32 enable);
	void (* usb_passby)(struct sw_hci_hcd *sw_hci, u32 enable);
};

#endif   //__SW_HCI_SUN6I_H__




