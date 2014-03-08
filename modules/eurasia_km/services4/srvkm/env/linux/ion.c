/*************************************************************************/ /*!
@Title          Ion driver inter-operability code.
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

#include "ion.h"

/* Three possible configurations:
 *
 *  - !SUPPORT_ION && CONFIG_ION_OMAP
 *    This is ion inter-op, not real ion support.
 *
 *  - SUPPORT_ION && CONFIG_ION_OMAP
 *    Real ion support, but sharing with an SOC ion device. We need
 *    to co-share the heaps too.
 *
 *  - SUPPORT_ION && !CONFIG_ION_OMAP
 *    "Reference" ion implementation. Creates its own ion device
 *    and heaps for the driver to use.
 */

#if !defined(SUPPORT_ION) && defined(CONFIG_ION_OMAP)

/* Legacy ion inter-op mode */

#include "services.h"
#include "servicesint.h"
#include "mutex.h"
#include "lock.h"
#include "mm.h"
#include "handle.h"
#include "perproc.h"
#include "env_perproc.h"
#include "private_data.h"
#include "pvr_debug.h"

#include <linux/module.h>
#include <linux/file.h>
#include <linux/fs.h>

extern struct ion_client *gpsIONClient;

void PVRSRVExportFDToIONHandles(int fd, struct ion_client **client,
								struct ion_handle *handles[2])
{
	PVRSRV_FILE_PRIVATE_DATA *psPrivateData;
	PVRSRV_KERNEL_MEM_INFO *psKernelMemInfo;
	LinuxMemArea *psLinuxMemArea;
	PVRSRV_ERROR eError;
	struct file *psFile;

	/* Take the bridge mutex so the handle won't be freed underneath us */
	LinuxLockMutexNested(&gPVRSRVLock, PVRSRV_LOCK_CLASS_BRIDGE);

	psFile = fget(fd);
	if(!psFile)
		goto err_unlock;

	psPrivateData = psFile->private_data;
	if(!psPrivateData)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: struct file* has no private_data; "
								"invalid export handle", __func__));
		goto err_fput;
	}

	eError = PVRSRVLookupHandle(KERNEL_HANDLE_BASE,
								(IMG_PVOID *)&psKernelMemInfo,
								psPrivateData->hKernelMemInfo,
								PVRSRV_HANDLE_TYPE_MEM_INFO);
	if(eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Failed to look up MEM_INFO handle",
								__func__));
		goto err_fput;
	}

	psLinuxMemArea = (LinuxMemArea *)psKernelMemInfo->sMemBlk.hOSMemHandle;
	BUG_ON(psLinuxMemArea == IMG_NULL);

	if(psLinuxMemArea->eAreaType != LINUX_MEM_AREA_ION)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: Valid handle, but not an ION buffer",
								__func__));
		goto err_fput;
	}

	handles[0] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[0];
	handles[1] = psLinuxMemArea->uData.sIONTilerAlloc.psIONHandle[1];
	if(client)
		*client = gpsIONClient;

err_fput:
	fput(psFile);
err_unlock:
	/* Allow PVRSRV clients to communicate with srvkm again */
	LinuxUnLockMutex(&gPVRSRVLock);
}

struct ion_handle *
PVRSRVExportFDToIONHandle(int fd, struct ion_client **client)
{
	struct ion_handle *psHandles[2] = { IMG_NULL, IMG_NULL };
	PVRSRVExportFDToIONHandles(fd, client, psHandles);
	return psHandles[0];
}

EXPORT_SYMBOL(PVRSRVExportFDToIONHandles);
EXPORT_SYMBOL(PVRSRVExportFDToIONHandle);

#endif /* !defined(SUPPORT_ION) && defined(CONFIG_ION_OMAP) */

#if defined(SUPPORT_ION)

#include <linux/scatterlist.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/err.h>

#if defined(CONFIG_ION_OMAP)

/* Real ion with sharing */

extern struct ion_device *omap_ion_device;
struct ion_device *gpsIonDev;

PVRSRV_ERROR IonInit(IMG_VOID)
{
	gpsIonDev = omap_ion_device;
	return PVRSRV_OK;
}

IMG_VOID IonDeinit(IMG_VOID)
{
	gpsIonDev = IMG_NULL;
}

#else /* defined(CONFIG_ION_OMAP) */

/* "Reference" ion implementation */

#include "../drivers/gpu/ion/ion_priv.h"

static struct ion_heap **gapsIonHeaps;
struct ion_device *gpsIonDev;
extern struct ion_heap *ion_heap_create(struct ion_platform_heap *);//aw
extern void ion_heap_destroy(struct ion_heap *);//aw

static struct ion_platform_data gsGenericConfig =
{
	.nr = 3,//aw
	.heaps =
	{
		{
			.type = ION_HEAP_TYPE_SYSTEM_CONTIG,
			.name = "System contig",
			.id   = ION_HEAP_TYPE_SYSTEM_CONTIG,
		},
		{
			.type = ION_HEAP_TYPE_SYSTEM,
			.name = "System",
			.id   = ION_HEAP_TYPE_SYSTEM,
		},
		//aw
		{
			.type = ION_HEAP_TYPE_CARVEOUT,
			.name = "carveout",
			.id   = ION_HEAP_TYPE_CARVEOUT,
			.base = ION_CARVEOUT_MEM_BASE,
			.size = ION_CARVEOUT_MEM_SIZE,
		},
			
	}
};

PVRSRV_ERROR IonInit(IMG_VOID)
{
	int uiHeapCount = gsGenericConfig.nr;
	int uiError;
	int i;

	gapsIonHeaps = kzalloc(sizeof(struct ion_heap *) * uiHeapCount, GFP_KERNEL);
	/* Create the ion devicenode */
	gpsIonDev = ion_device_create(NULL);
	if (IS_ERR_OR_NULL(gpsIonDev)) {
		kfree(gapsIonHeaps);
		return PVRSRV_ERROR_OUT_OF_MEMORY;
	}

	/* Register all the heaps */
	for (i = 0; i < gsGenericConfig.nr; i++)
	{
		struct ion_platform_heap *psPlatHeapData = &gsGenericConfig.heaps[i];

		gapsIonHeaps[i] = ion_heap_create(psPlatHeapData);
		if (IS_ERR_OR_NULL(gapsIonHeaps[i]))
		{
			uiError = PTR_ERR(gapsIonHeaps[i]);
			goto failHeapCreate;
		}
		ion_device_add_heap(gpsIonDev, gapsIonHeaps[i]);
	}

	return PVRSRV_OK;
failHeapCreate:
	for (i = 0; i < uiHeapCount; i++)
	{
		if (gapsIonHeaps[i])
		{
				ion_heap_destroy(gapsIonHeaps[i]);
		}
	}
	kfree(gapsIonHeaps);
	return PVRSRV_ERROR_OUT_OF_MEMORY;
}

IMG_VOID IonDeinit(IMG_VOID)
{
	int uiHeapCount = gsGenericConfig.nr;
	int i;

	for (i = 0; i < uiHeapCount; i++)
	{
		if (gapsIonHeaps[i])
		{
				ion_heap_destroy(gapsIonHeaps[i]);
		}
	}
	kfree(gapsIonHeaps);
	ion_device_destroy(gpsIonDev);
}

#endif /* defined(CONFIG_ION_OMAP) */

#define MAX_IMPORT_ION_FDS 3

typedef struct _ION_IMPORT_DATA_
{
	/* ion client handles are imported into */
	struct ion_client *psIonClient;

	/* Number of ion handles represented by this import */
	IMG_UINT32 ui32NumIonHandles;

	/* Array of ion handles in use by services */
	struct ion_handle *apsIonHandle[MAX_IMPORT_ION_FDS];

	/* Array of physical addresses represented by these buffers */
	IMG_SYS_PHYADDR *psSysPhysAddr;

	/* If ui32NumBuffers is 1 and ion_map_kernel() is implemented by the
	 * allocator, this may be non-NULL. Otherwise it will be NULL.
	 */
	IMG_PVOID pvKernAddr0;
}
ION_IMPORT_DATA;

PVRSRV_ERROR IonImportBufferAndAcquirePhysAddr(IMG_HANDLE hIonDev,
											   IMG_UINT32 ui32NumFDs,
											   IMG_INT32  *pai32BufferFDs,
											   IMG_UINT32 *pui32PageCount,
											   IMG_SYS_PHYADDR **ppsSysPhysAddr,
											   IMG_PVOID  *ppvKernAddr0,
											   IMG_HANDLE *phPriv,
											   IMG_HANDLE *phUnique)
{
	struct scatterlist *psTemp, *psScatterList[MAX_IMPORT_ION_FDS] = {};
	PVRSRV_ERROR eError = PVRSRV_ERROR_OUT_OF_MEMORY;
	struct ion_client *psIonClient = hIonDev;
	IMG_UINT32 i, k, ui32PageCount = 0;
	ION_IMPORT_DATA *psImportData;

	if(ui32NumFDs > MAX_IMPORT_ION_FDS)
	{
		printk(KERN_ERR "%s: More ion export fds passed in than supported "
						"(%d provided, %d max)", __func__, ui32NumFDs,
						MAX_IMPORT_ION_FDS);
		return PVRSRV_ERROR_INVALID_PARAMS;
	}

	psImportData = kzalloc(sizeof(ION_IMPORT_DATA), GFP_KERNEL);
	if (psImportData == NULL)
	{
		goto exitFailKMallocImportData;
	}

	/* Set up import data for free call */
	psImportData->psIonClient = psIonClient;
	psImportData->ui32NumIonHandles = ui32NumFDs;

	for(i = 0; i < ui32NumFDs; i++)
	{
		int fd = (int)pai32BufferFDs[i];
		struct sg_table *psSgTable;

		psImportData->apsIonHandle[i] = ion_import_dma_buf(psIonClient, fd);
		if (psImportData->apsIonHandle[i] == IMG_NULL)
		{
			eError = PVRSRV_ERROR_BAD_MAPPING;
			goto exitFailImport;
		}

		psSgTable = ion_sg_table(psIonClient, psImportData->apsIonHandle[i]);
		psScatterList[i] = psSgTable->sgl;
		if (psScatterList[i] == NULL)
		{
			eError = PVRSRV_ERROR_INVALID_PARAMS;
			goto exitFailImport;
		}

		//aw
		/* Although all heaps will provide an sg_table, the tables cannot
		 * always be trusted because sg_lists are just pointers to "struct
		 * page" values, and some memory e.g. carveout may not have valid
		 * "struct page" values. In particular, on ARM, carveout is
		 * generally reserved with memblock_remove(), which leaves the
		 * "struct page" entries uninitialized when SPARSEMEM is enabled.
		 * The effect of this is that page_to_pfn(pfn_to_page(pfn)) != pfn.
		 *
		 * There's more discussion on this mailing list thread:
		 * http://lists.linaro.org/pipermail/linaro-mm-sig/2012-August/002440.html
		 *
		 * If the heap this buffer comes from implements ->phys(), it's
		 * probably a contiguous allocator. If the phys() function is
		 * implemented, we'll use it to check sg_table->sgl[0]. If we find
		 * they don't agree, we'll assume phys() is more reliable and use
		 * that.
		 *
		 * Unfortunately, although phys() should not be implemented on
		 * non-contiguous heaps, we need to work with broken legacy heaps that
		 * do implement both. Therefore we _will_ use the sg_table if the
		 * phys() and first sg_table entry match, even though it may possibly
		 * still be the case that the sg_table is corrupt. (This is unlikely
		 * because the sg_table for most contiguous allocators should be a
		 * single span from 'start' to 'start+size'.)
		 *
		 * Also, ion prints out an error message if the heap doesn't implement
		 * ->phys(), which we want to avoid, so only use ->phys() if the
		 * sg_table contains a single span and therefore could plausibly
		 * be a contiguous allocator.
		 */
		if(!sg_next(psScatterList[i]))
		{
			ion_phys_addr_t sPhyAddr;
			size_t sLength;

			if(!ion_phys(psIonClient, psImportData->apsIonHandle[i],
						 &sPhyAddr, &sLength))
			{
				BUG_ON(sLength & ~PAGE_MASK);

				if(sg_phys(psScatterList[i]) != sPhyAddr)
				{
					psScatterList[i] = IMG_NULL;
					ui32PageCount += sLength / PAGE_SIZE;
				}
			}
		}

		for(psTemp = psScatterList[i]; psTemp; psTemp = sg_next(psTemp))
		{
			IMG_UINT32 j;
			for (j = 0; j < psTemp->length; j += PAGE_SIZE)
			{
				ui32PageCount++;
			}
		}
	}

	BUG_ON(ui32PageCount == 0);

	psImportData->psSysPhysAddr = kmalloc(sizeof(IMG_SYS_PHYADDR) * ui32PageCount, GFP_KERNEL);
	if (psImportData->psSysPhysAddr == NULL)
	{
		goto exitFailImport;
	}

	for(i = 0, k = 0; i < ui32NumFDs; i++)
	{
		if(psScatterList[i])//aw
		{
			for(psTemp = psScatterList[i]; psTemp; psTemp = sg_next(psTemp))
			{
				IMG_UINT32 j;
				for (j = 0; j < psTemp->length; j += PAGE_SIZE)
				{
					psImportData->psSysPhysAddr[k].uiAddr = sg_phys(psTemp) + j;
					k++;//aw
				}
			}
		}
		else//aw
		{
			ion_phys_addr_t sPhyAddr;
			size_t sLength, j;

			ion_phys(psIonClient, psImportData->apsIonHandle[i],
					 &sPhyAddr, &sLength);

			for(j = 0; j < sLength; j += PAGE_SIZE)
			{
				psImportData->psSysPhysAddr[k].uiAddr = sPhyAddr + j;
				k++;
			}
		}
	}

	*pui32PageCount = ui32PageCount;
	*ppsSysPhysAddr = psImportData->psSysPhysAddr;

	if(ui32NumFDs == 1)
	{
		IMG_PVOID pvKernAddr0;

		pvKernAddr0 = ion_map_kernel(psIonClient, psImportData->apsIonHandle[0]);
		if (IS_ERR(pvKernAddr0))
		{
			pvKernAddr0 = IMG_NULL;
		}

		psImportData->pvKernAddr0 = pvKernAddr0;
		*ppvKernAddr0 = pvKernAddr0;
	}
	else
	{
		*ppvKernAddr0 = NULL;
	}

	*phPriv = psImportData;
	*phUnique = (IMG_HANDLE)psImportData->psSysPhysAddr[0].uiAddr;

	return PVRSRV_OK;

exitFailImport:
	for(i = 0; psImportData->apsIonHandle[i] != NULL; i++)
	{
		ion_free(psIonClient, psImportData->apsIonHandle[i]);
	}
	kfree(psImportData);
exitFailKMallocImportData:
	return eError;
}

IMG_VOID IonUnimportBufferAndReleasePhysAddr(IMG_HANDLE hPriv)
{
	ION_IMPORT_DATA *psImportData = hPriv;
	IMG_UINT32 i;

	if (psImportData->pvKernAddr0)
	{
		ion_unmap_kernel(psImportData->psIonClient, psImportData->apsIonHandle[0]);
	}

	for(i = 0; i < psImportData->ui32NumIonHandles; i++)
	{
		ion_free(psImportData->psIonClient, psImportData->apsIonHandle[i]);
	}

	kfree(psImportData->psSysPhysAddr);
	kfree(psImportData);
}

#endif /* defined(SUPPORT_ION) */
