/**
 * serial.c - common operations
 * date:    2012-2-13 8:42:56
 * author:  Aaron<leafy.myeh@allwinnertech.com>
 * history: V0.1
 */

#include "pm_types.h"
#include "pm.h"

//------------------------------------------------------------------------------
//return value defines
//------------------------------------------------------------------------------
#define	OK		(0)
#define	FAIL	(-1)
#define TRUE	(1)
#define	FALSE	(0)

#define NULL	(0)

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned short *)(addr)))
#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

void serial_init_nommu(void)
{
	__u32 p2clk;
	__u32 df;
	__u32 lcr;
	volatile unsigned int *reg;
	__u32 port = 0;
	__u32 i = 0;

	p2clk = (24000000);

	//config uart clk
	reg = (volatile unsigned int *)(CCU_UART_PA);
	*reg &= ~(1 << (16 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (1 << (16 + port));
	//de-assert uart reset
	reg = (volatile unsigned int *)(CCU_UART_RESET_PA);
	*reg &= ~(1 << (16 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (1 << (16 + port));
	
	// config uart gpio
	// config tx gpio
	//fpga not need care gpio config;
	reg = (volatile unsigned int *)(0x01c20800 + 0x104);
	*reg &= ~(0x77 << (16 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (0x22 << (16 + port));
	
	/* set baudrate */
	df = (p2clk + (SUART_BAUDRATE<<3))/(SUART_BAUDRATE<<4);
	lcr = readl(SUART_LCR_PA);
	writel(1, SUART_HALT_PA);
	writel(lcr|0x80, SUART_LCR_PA);
	writel(df>>8, SUART_DLH_PA);
	writel(df&0xff, SUART_DLL_PA);
	writel(lcr&(~0x80), SUART_LCR_PA);
	writel(0, SUART_HALT_PA);

	/* set mode, Set Lin Control Register*/
	writel(3, SUART_LCR_PA);
	/* enable fifo */
	writel(0xe1, SUART_FCR_PA);

}

static void serial_put_char_nommu(char c)
{
	while (!(readl(SUART_USR_PA) & 2));
	writel(c, SUART_THR_PA);
}

static char serial_get_char_nommu(void)
{
	__u32 time = 0xffff;
	while(!(readl(SUART_USR_PA)&0x08) && time--);
	if (!time)
		return 0;
	return readl(SUART_RBR_PA);
}

__s32 serial_puts_nommu(const char *string)
{
	//ASSERT(string != NULL);
	
	while(*string != '\0')
	{
		if(*string == '\n')
		{
			// if current character is '\n', 
			// insert output with '\r'.
			serial_put_char_nommu('\r');
		}
		serial_put_char_nommu(*string++);
	}
	
	return OK;
}

__u32 serial_gets_nommu(char* buf, __u32 n)
{
	__u32 i;
	char c;
	
	for (i=0; i<n; i++) {
		c = serial_get_char_nommu();
		if (c == 0)
			break;
		buf[i] = c;
	}
	return i+1;
}

void serial_init(void)
{

	__u32 p2clk;
	__u32 df;
	__u32 lcr;
	volatile unsigned int *reg;
	__u32 port = 0;
	__u32 i = 0;

	p2clk = (24000000);

	//judge uart clk
	
	//config uart clk
	reg = (volatile unsigned int *)(CCU_UART_VA);
	*reg &= ~(1 << (16 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (1 << (16 + port));
	//de-assert uart reset
	reg = (volatile unsigned int *)(IO_ADDRESS(CCU_UART_RESET_PA));
	*reg &= ~(1 << (16 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (1 << (16 + port));
	// config uart gpio
	// config tx gpio
	//fpga not need care gpio config;
	reg = (volatile unsigned int *)(0xf1c20800 + 0x104);
	*reg &= ~(0x77 << (16 + port));
	for( i = 0; i < 100; i++ );
	*reg |=  (0x22 << (16 + port));

	/* set baudrate */
	df = (p2clk + (SUART_BAUDRATE<<3))/(SUART_BAUDRATE<<4);
	lcr = readl(SUART_LCR);
	writel(1, SUART_HALT);
	writel(lcr|0x80, SUART_LCR);
	writel(df>>8, SUART_DLH);
	writel(df&0xff, SUART_DLL);
	writel(lcr&(~0x80), SUART_LCR);
	writel(0, SUART_HALT);

	/* set mode, Set Lin Control Register*/
	writel(3, SUART_LCR);
	/* enable fifo */
	writel(0xe1, SUART_FCR);

}


static void serial_put_char(char c)
{
	while (!(readl(SUART_USR) & 2));
	writel(c, SUART_THR);
}

static char serial_get_char(void)
{
	__u32 time = 0xffff;
	while(!(readl(SUART_USR)&0x08) && time--);
	if (!time)
		return 0;
	return (char)(readl(SUART_RBR));
}


/*
*********************************************************************************************************
*                                       	PUT A STRING
*
* Description: 	put out a string.
*
* Arguments  : 	string	: the string which we want to put out.
*
* Returns    : 	OK if put out string succeeded, others if failed.
*********************************************************************************************************
*/
__s32 serial_puts(const char *string)
{
	//ASSERT(string != NULL);
	
	while(*string != '\0')
	{
		if(*string == '\n')
		{
			// if current character is '\n', 
			// insert output with '\r'.
			serial_put_char('\r');
		}
		serial_put_char(*string++);
	}
	
	return OK;
}


__u32 serial_gets(char* buf, __u32 n)
{
	__u32 i;
	char c;
	
	for (i=0; i<n; i++) {
		c = serial_get_char();
		if (c == 0)
			break;
		buf[i] = c;
	}
	return i+1;
}
