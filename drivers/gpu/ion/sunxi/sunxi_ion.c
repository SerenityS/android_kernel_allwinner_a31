/*
 * drivers/gpu/sunxi/sunxi_ion.c
 * (C) Copyright 2010-2015
 * Reuuimlla Technology Co., Ltd. <www.reuuimllatech.com>
 * liugang <liugang@reuuimllatech.com>
 *
 * sunxi ion driver interface
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include <linux/err.h>
#include <linux/ion.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "../ion_priv.h"
#include <linux/sunxi_ion.h>
#include <linux/module.h>
#include <linux/uaccess.h>  

struct ion_heap *sunxi_tiler_heap_create(struct ion_platform_heap *data);
struct ion_heap *ion_heap_create(struct ion_platform_heap *heap_data);
void sunxi_tiler_heap_destroy(struct ion_heap *heap);
void ion_heap_destroy(struct ion_heap *heap);
int sunxi_tiler_alloc(struct ion_heap *heap,
		     struct ion_client *client,
		     struct sunxi_ion_tiler_alloc_data *data);
int sunxi_tiler_heap_map_user(struct ion_heap *heap, struct ion_buffer *buffer,
			     struct vm_area_struct *vma);
int sunxi_tiler_pages(struct ion_client *client, struct ion_handle *handle,
		     int *n, u32 **tiler_addrs);
void sunxi_tiler_heap_free(struct ion_buffer *buffer);

/* platform heap definations */
static struct ion_platform_data sunxi_heap_config =
{
	.nr = 4,
	.heaps = {
		{
			.type = ION_HEAP_TYPE_SYSTEM_CONTIG,
			.name = "system_contig",
			.id   = HEAP_ID_SYSTEM_CONTIG,
		},
		{
			.type = ION_HEAP_TYPE_SYSTEM,
			.name = "system",
			.id   = HEAP_ID_SYSTEM,
		},
		{
			.type = ION_HEAP_TYPE_CARVEOUT,
			.name = "carveout",
			.id   = HEAP_ID_CARVEOUT,
			.base = ION_CARVEOUT_MEM_BASE,
			.size = ION_CARVEOUT_MEM_SIZE,
		},
        {
			.type = SUNXI_ION_HEAP_TYPE_TILER,
			.name = "tilerheap",
			//.id   = HEAP_ID_SUNXI_TILER,
			.id   = SUNXI_ION_HEAP_TYPE_TILER,
			.base = ION_CARVEOUT_MEM_BASE,
			.size = ION_CARVEOUT_MEM_SIZE,
		}
	}
};

struct ion_device *sunxi_ion_device;
EXPORT_SYMBOL_GPL(sunxi_ion_device);

int num_heaps;
struct ion_heap **heaps;
struct ion_heap *tiler_heap;
static struct ion_heap *nonsecure_tiler_heap;

int sunxi_ion_tiler_alloc(struct ion_client *client,
			 struct sunxi_ion_tiler_alloc_data *data)
{
    //printk(KERN_DEBUG "%s, line %d, tiler_heap 0x%08x\n", __func__, __LINE__, (u32)tiler_heap);
	return sunxi_tiler_alloc(tiler_heap, client, data);
}
EXPORT_SYMBOL_GPL(sunxi_ion_tiler_alloc);

int sunxi_ion_nonsecure_tiler_alloc(struct ion_client *client,
			 struct sunxi_ion_tiler_alloc_data *data)
{
	if (!nonsecure_tiler_heap)
		return -ENOMEM;
	return sunxi_tiler_alloc(nonsecure_tiler_heap, client, data);
}

long sunxi_ion_ioctl(struct ion_client *client, unsigned int cmd,
		    unsigned long arg)
{
	switch (cmd) {
	case SUNXI_ION_TILER_ALLOC:
	{
		struct sunxi_ion_tiler_alloc_data data;
		int ret;

		if (!tiler_heap) {
			pr_err("%s: Tiler heap requested but no tiler "
			       "heap exists on this platform\n", __func__);
			return -EINVAL;
		}
		if (copy_from_user(&data, (void __user *)arg, sizeof(data)))
			return -EFAULT;
		ret = sunxi_ion_tiler_alloc(client, &data);
		if (ret)
			return ret;
		if (copy_to_user((void __user *)arg, &data,
				 sizeof(data)))
			return -EFAULT;
		break;
	}
	default:
		pr_err("%s: Unknown custom ioctl\n", __func__);
		return -ENOTTY;
	}
	return 0;
}

int sunxi_ion_probe(struct platform_device *pdev)
{
	struct ion_platform_data *pdata = pdev->dev.platform_data;
	int err;
	int i;

	num_heaps = pdata->nr;

	heaps = kzalloc(sizeof(struct ion_heap *) * pdata->nr, GFP_KERNEL);

	sunxi_ion_device = ion_device_create(sunxi_ion_ioctl);
    printk("%s, line %d, sunxi_ion_device 0x%08x\n", __func__, __LINE__, (u32)sunxi_ion_device);
	if (IS_ERR_OR_NULL(sunxi_ion_device)) 
    {
		kfree(heaps);
		return PTR_ERR(sunxi_ion_device);
	}

	/* create the heaps as specified in the board file */
	for (i = 0; i < num_heaps; i++) 
    {
		struct ion_platform_heap *heap_data = &pdata->heaps[i];
        printk("%s, line %d, sunxi_ion_device 0x%08x\n", __func__, __LINE__, (u32)heap_data->type);
		if ((int)heap_data->type == (int)SUNXI_ION_HEAP_TYPE_TILER) 
        {
            printk(KERN_DEBUG "%s, line %d, heap_data->id 0x%08x\n", __func__, __LINE__, (u32)heap_data->id);
			heaps[i] = sunxi_tiler_heap_create(heap_data);
			if (heap_data->id == SUNXI_ION_HEAP_NONSECURE_TILER)
			{
				nonsecure_tiler_heap = heaps[i];
			}
			else
			{
				tiler_heap = heaps[i];
			}
            printk("%s, line %d, tiler_heap 0x%08x\n", __func__, __LINE__, (u32)tiler_heap);
		} 
        else 
		{
			heaps[i] = ion_heap_create(heap_data);
		}
		if (IS_ERR_OR_NULL(heaps[i])) {
			err = PTR_ERR(heaps[i]);
			goto err;
		}
		ion_device_add_heap(sunxi_ion_device, heaps[i]);
		pr_info("%s: adding heap %s of type %d with %lx@%x\n",
			__func__, heap_data->name, heap_data->type,
			heap_data->base, heap_data->size);

	}

	platform_set_drvdata(pdev, sunxi_ion_device);
	return 0;
err:
	for (i = 0; i < num_heaps; i++) {
		if (heaps[i]) {
			if ((int)heaps[i]->type == (int)SUNXI_ION_HEAP_TYPE_TILER)
				sunxi_tiler_heap_destroy(heaps[i]);
			else
				ion_heap_destroy(heaps[i]);
		}
	}
	kfree(heaps);
	return err;
}

int sunxi_ion_remove(struct platform_device *pdev)
{
	struct ion_device *idev = platform_get_drvdata(pdev);
	int i;

	ion_device_destroy(idev);
	for (i = 0; i < num_heaps; i++)
		if ((int)heaps[i]->type == (int)SUNXI_ION_HEAP_TYPE_TILER)
			sunxi_tiler_heap_destroy(heaps[i]);
		else
			ion_heap_destroy(heaps[i]);
	kfree(heaps);
	return 0;
}

static struct platform_device sunxi_ion_dev = {
	.name 	= "ion-sunxi",
	.id 	= 0,
	.dev	= {
		.platform_data = &sunxi_heap_config,
	}
};
static struct platform_driver sunxi_ion_drv = {
	.probe = sunxi_ion_probe,
	.remove = sunxi_ion_remove,
	.driver = { .name = "ion-sunxi" }
};

static int __init ion_init(void)
{
	printk(KERN_DEBUG "%s enter\n", __func__);
	if(platform_device_register(&sunxi_ion_dev))
		printk(KERN_ERR "%s(%d) err: platform_device_register failed\n", __func__, __LINE__);
	if(platform_driver_register(&sunxi_ion_drv))
		printk(KERN_ERR "%s(%d) err: platform_driver_register failed\n", __func__, __LINE__);
	return 0;
}

static void __exit ion_exit(void)
{
	platform_driver_unregister(&sunxi_ion_drv);
	platform_device_unregister(&sunxi_ion_dev);
}

MODULE_LICENSE("GPL");
subsys_initcall(ion_init);
module_exit(ion_exit);

