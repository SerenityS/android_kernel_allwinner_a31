/*************************************************************************/ /*!
@Title          OMAP linux display driver components
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/

/**************************************************************************
 The 3rd party driver is a specification of an API to integrate the IMG POWERVR
 Services driver with 3rd Party display hardware.  It is NOT a specification for
 a display controller driver, rather a specification to extend the API for a
 pre-existing driver for the display hardware.

 The 3rd party driver interface provides IMG POWERVR client drivers (e.g. PVR2D)
 with an API abstraction of the system's underlying display hardware, allowing
 the client drivers to indirectly control the display hardware and access its
 associated memory.
 
 Functions of the API include
 - query primary surface attributes (width, height, stride, pixel format, CPU
     physical and virtual address)
 - swap/flip chain creation and subsequent query of surface attributes
 - asynchronous display surface flipping, taking account of asynchronous read
 (flip) and write (render) operations to the display surface

 Note: having queried surface attributes the client drivers are able to map the
 display memory to any IMG POWERVR Services device by calling
 PVRSRVMapDeviceClassMemory with the display surface handle.

 This code is intended to be an example of how a pre-existing display driver may
 be extended to support the 3rd Party Display interface to POWERVR Services
 - IMG is not providing a display driver implementation.
 **************************************************************************/

#include <linux/version.h>

#include <asm/atomic.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hardirq.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/fb.h>
#include <linux/console.h>
#include <linux/mutex.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "dc_sunxi.h"
#include "pvrmodule.h"


MODULE_SUPPORTED_DEVICE(DEVNAME);


void *OMAPLFBAllocKernelMem(unsigned long ulSize)
{
	return kmalloc(ulSize, GFP_KERNEL);
}

void OMAPLFBFreeKernelMem(void *pvMem)
{
	kfree(pvMem);
}

void OMAPLFBCreateSwapChainLockInit(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_init(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainLockDeInit(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_destroy(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainLock(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_lock(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBCreateSwapChainUnLock(OMAPLFB_DEVINFO *psDevInfo)
{
	mutex_unlock(&psDevInfo->sCreateSwapChainMutex);
}

void OMAPLFBAtomicBoolInit(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

void OMAPLFBAtomicBoolDeInit(OMAPLFB_ATOMIC_BOOL *psAtomic)
{
}

void OMAPLFBAtomicBoolSet(OMAPLFB_ATOMIC_BOOL *psAtomic, OMAPLFB_BOOL bVal)
{
	atomic_set(psAtomic, (int)bVal);
}

OMAPLFB_BOOL OMAPLFBAtomicBoolRead(OMAPLFB_ATOMIC_BOOL *psAtomic)
{
	return (OMAPLFB_BOOL)atomic_read(psAtomic);
}

void OMAPLFBAtomicIntInit(OMAPLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

void OMAPLFBAtomicIntDeInit(OMAPLFB_ATOMIC_INT *psAtomic)
{
}

void OMAPLFBAtomicIntSet(OMAPLFB_ATOMIC_INT *psAtomic, int iVal)
{
	atomic_set(psAtomic, iVal);
}

int OMAPLFBAtomicIntRead(OMAPLFB_ATOMIC_INT *psAtomic)
{
	return atomic_read(psAtomic);
}

void OMAPLFBAtomicIntInc(OMAPLFB_ATOMIC_INT *psAtomic)
{
	atomic_inc(psAtomic);
}

OMAPLFB_ERROR OMAPLFBGetLibFuncAddr (char *szFunctionName, PFN_DC_GET_PVRJTABLE *ppfnFuncTable)
{
	if(strcmp("PVRGetDisplayClassJTable", szFunctionName) != 0)
	{
		return (OMAPLFB_ERROR_INVALID_PARAMS);
	}

	/* Nothing to do - should be exported from pvrsrv.ko */
	*ppfnFuncTable = PVRGetDisplayClassJTable;

	return (OMAPLFB_OK);
}

/* Inset a swap buffer into the swap chain work queue */
void OMAPLFBQueueBufferForSwap(OMAPLFB_SWAPCHAIN *psSwapChain, OMAPLFB_BUFFER *psBuffer)
{
	int res = queue_work(psSwapChain->psWorkQueue, &psBuffer->sWork);

	if (res == 0)
	{
		printk(KERN_WARNING DRIVER_PREFIX ": %s: Device %u: Buffer already on work queue\n", __FUNCTION__, psSwapChain->uiFBDevID);
	}
}

/* Process an item on a swap chain work queue */
static void WorkQueueHandler(struct work_struct *psWork)
{
	OMAPLFB_BUFFER *psBuffer = container_of(psWork, OMAPLFB_BUFFER, sWork);

	OMAPLFBSwapHandler(psBuffer);
}

/* Create a swap chain work queue */
OMAPLFB_ERROR OMAPLFBCreateSwapQueue(OMAPLFB_SWAPCHAIN *psSwapChain)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
	/*
	 * Calling alloc_ordered_workqueue with the WQ_FREEZABLE and
	 * WQ_MEM_RECLAIM flags set, (currently) has the same effect as
	 * calling create_freezable_workqueue. None of the other WQ
	 * flags are valid. Setting WQ_MEM_RECLAIM should allow the
	 * workqueue to continue to service the swap chain in low memory
	 * conditions, preventing the driver from holding on to
	 * resources longer than it needs to.
	 */
	psSwapChain->psWorkQueue = alloc_ordered_workqueue(DEVNAME, WQ_FREEZABLE | WQ_MEM_RECLAIM);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	psSwapChain->psWorkQueue = create_freezable_workqueue(DEVNAME);
#else
	/*
	 * Create a single-threaded, freezable, rt-prio workqueue.
	 * Such workqueues are frozen with user threads when a system
	 * suspends, before driver suspend entry points are called.
	 * This ensures this driver will not call into the Linux
	 * framebuffer driver after the latter is suspended.
	 */
	psSwapChain->psWorkQueue = __create_workqueue(DEVNAME, 1, 1, 1);
#endif
#endif
	if (psSwapChain->psWorkQueue == NULL)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: Couldn't create workqueue\n", __FUNCTION__, psSwapChain->uiFBDevID);

		return (OMAPLFB_ERROR_INIT_FAILURE);
	}

	return (OMAPLFB_OK);
}

/* Prepare buffer for insertion into a swap chain work queue */
void OMAPLFBInitBufferForSwap(OMAPLFB_BUFFER *psBuffer)
{
	INIT_WORK(&psBuffer->sWork, WorkQueueHandler);
}

/* Destroy a swap chain work queue */
void OMAPLFBDestroySwapQueue(OMAPLFB_SWAPCHAIN *psSwapChain)
{
	destroy_workqueue(psSwapChain->psWorkQueue);
}

/* Flip display to given buffer */
void OMAPLFBFlip(OMAPLFB_DEVINFO *psDevInfo, OMAPLFB_BUFFER *psBuffer)
{
	struct fb_var_screeninfo sFBVar;
	int res;
	unsigned long ulYResVirtual;

	OMAPLFB_CONSOLE_LOCK();

	sFBVar = psDevInfo->psLINFBInfo->var;

	sFBVar.xoffset = 0;
	sFBVar.yoffset = psBuffer->ulYOffset;

	ulYResVirtual = psBuffer->ulYOffset + sFBVar.yres;

	/*
	 * PVR_OMAPLFB_DONT_USE_FB_PAN_DISPLAY should be defined to work
	 * around flipping problems seen with the Taal LCDs on Blaze.
	 * The work around is safe to use with other types of screen on Blaze
	 * (e.g. HDMI) and on other platforms (e.g. Panda board).
	 */
	/*
	 * Attempt to change the virtual screen resolution if it is too
	 * small.  Note that fb_set_var also pans the display.
	 */
	if (0)//sFBVar.xres_virtual != sFBVar.xres || sFBVar.yres_virtual < ulYResVirtual)
	{
		sFBVar.xres_virtual = sFBVar.xres;
		sFBVar.yres_virtual = ulYResVirtual;

		sFBVar.activate = FB_ACTIVATE_NOW | FB_ACTIVATE_FORCE;

		res = fb_set_var(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_set_var failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}
	else
	{		
		res = fb_pan_display(psDevInfo->psLINFBInfo, &sFBVar);
		if (res != 0)
		{
			printk(KERN_ERR DRIVER_PREFIX ": %s: Device %u: fb_pan_display failed (Y Offset: %lu, Error: %d)\n", __FUNCTION__, psDevInfo->uiFBDevID, psBuffer->ulYOffset, res);
		}
	}

	OMAPLFB_CONSOLE_UNLOCK();
}

OMAPLFB_UPDATE_MODE OMAPLFBGetUpdateMode(OMAPLFB_DEVINFO *psDevInfo)
{
	return OMAPLFB_UPDATE_MODE_AUTO;
}


void OMAPLFBPrintInfo(OMAPLFB_DEVINFO *psDevInfo)
{
    return;
}

/* Wait for VSync */
OMAPLFB_BOOL OMAPLFBWaitForVSync(OMAPLFB_DEVINFO *psDevInfo)
{
	return OMAPLFB_TRUE;
}

/*
 * Wait for screen to update.  If the screen is in manual or auto update
 * mode, we can call this function to wait for the screen to update.
 */
OMAPLFB_BOOL OMAPLFBManualSync(OMAPLFB_DEVINFO *psDevInfo)
{
	return OMAPLFB_TRUE;
}

/*
 * If the screen is manual or auto update mode, wait for the screen to
 * update.
 */
OMAPLFB_BOOL OMAPLFBCheckModeAndSync(OMAPLFB_DEVINFO *psDevInfo)
{
	OMAPLFB_UPDATE_MODE eMode = OMAPLFB_UPDATE_MODE_AUTO;

	switch(eMode)
	{
		case OMAPLFB_UPDATE_MODE_AUTO:
		case OMAPLFB_UPDATE_MODE_MANUAL:
			return OMAPLFBManualSync(psDevInfo);
		default:
			break;
	}

	return OMAPLFB_TRUE;
}

/* Unblank the screen */
OMAPLFB_ERROR OMAPLFBUnblankDisplay(OMAPLFB_DEVINFO *psDevInfo)
{
	int res;

	OMAPLFB_CONSOLE_LOCK();
	res = fb_blank(psDevInfo->psLINFBInfo, 0);
	OMAPLFB_CONSOLE_UNLOCK();
	if (res != 0 && res != -EINVAL)
	{
		printk(KERN_ERR DRIVER_PREFIX
			": %s: Device %u: fb_blank failed (%d)\n", __FUNCTION__, psDevInfo->uiFBDevID, res);
		return (OMAPLFB_ERROR_GENERIC);
	}

	return (OMAPLFB_OK);
}

/* Set up Linux Framebuffer event notification */
OMAPLFB_ERROR OMAPLFBEnableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo)
{
	return (OMAPLFB_OK);
}

/* Disable Linux Framebuffer event notification */
OMAPLFB_ERROR OMAPLFBDisableLFBEventNotification(OMAPLFB_DEVINFO *psDevInfo)
{
	return (OMAPLFB_OK);
}


/* Insert the driver into the kernel */
static int __init OMAPLFB_Init(void)
{    
	if(OMAPLFBInit() != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: OMAPLFBInit failed\n", __FUNCTION__);
		return -ENODEV;
	}

	return 0;

}

/* Remove the driver from the kernel */
static void __exit OMAPLFB_Cleanup(void)
{    
	if(OMAPLFBDeInit() != OMAPLFB_OK)
	{
		printk(KERN_ERR DRIVER_PREFIX ": %s: OMAPLFBDeInit failed\n", __FUNCTION__);
	}
}

/*
 These macro calls define the initialisation and removal functions of the
 driver.  Although they are prefixed `module_', they apply when compiling
 statically as well; in both cases they define the function the kernel will
 run to start/stop the driver.
*/
late_initcall(OMAPLFB_Init);
module_exit(OMAPLFB_Cleanup);
