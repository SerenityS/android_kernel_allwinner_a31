/*
 * include/linux/sunxi_ion.h
 *
 * Copyright (C) 2011 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SUNXI_ION_H
#define _LINUX_SUNXI_ION_H

#include <linux/types.h>

/* heap id, or priority, lower will be tried first */
#define HEAP_ID_SYSTEM		0
#define HEAP_ID_SYSTEM_CONTIG	1
#define HEAP_ID_CARVEOUT	2
#define HEAP_ID_SUNXI_TILER	3

/**
 * struct sunxi_ion_tiler_alloc_data - metadata passed from userspace for allocations
 * @w:		width of the allocation
 * @h:		height of the allocation
 * @fmt:	format of the data (8, 16, 32bit or page)
 * @flags:	flags passed to heap
 * @stride:	stride of the allocation, returned to caller from kernel
 * @handle:	pointer that will be populated with a cookie to use to refer
 *		to this allocation
 *
 * Provided by userspace as an argument to the ioctl
 */
struct sunxi_ion_tiler_alloc_data 
{
	size_t w;
	size_t h;
	int fmt;
	unsigned int flags;
	struct ion_handle *handle;
	size_t stride;
	size_t offset;
};

#ifdef __KERNEL__
int sunxi_ion_tiler_alloc(struct ion_client *client,
			 struct sunxi_ion_tiler_alloc_data *data);
int sunxi_ion_nonsecure_tiler_alloc(struct ion_client *client,
			 struct sunxi_ion_tiler_alloc_data *data);
/* given a handle in the tiler, return a list of tiler pages that back it */
int sunxi_tiler_pages(struct ion_client *client, struct ion_handle *handle,
		     int *n, u32 ** tiler_pages);
#endif /* __KERNEL__ */

/* additional heaps used only on sunxi */
enum {
	SUNXI_ION_HEAP_TYPE_TILER = ION_HEAP_TYPE_CUSTOM + 1,
};

#define SUNXI_ION_HEAP_TILER_MASK (1 << SUNXI_ION_HEAP_TYPE_TILER)

enum {
	SUNXI_ION_TILER_ALLOC,
};

/**
 * These should match the defines in the tiler driver
 */
enum {
	TILER_PIXEL_FMT_MIN   = 0,
	TILER_PIXEL_FMT_8BIT  = 0,
	TILER_PIXEL_FMT_16BIT = 1,
	TILER_PIXEL_FMT_32BIT = 2,
	TILER_PIXEL_FMT_PAGE  = 3,
	TILER_PIXEL_FMT_MAX   = 3
};

/**
 * List of heaps in the system
 */
enum {
	SUNXI_ION_HEAP_LARGE_SURFACES,
	SUNXI_ION_HEAP_TILER,
	SUNXI_ION_HEAP_SECURE_INPUT,
	SUNXI_ION_HEAP_NONSECURE_TILER,
};

#endif /* _LINUX_ION_H */

