/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#ifndef _IOCTX_H
#define _IOCTX_H

#include <kernel/vfs.h>
#include <kernel/mutex.h>
#include <sys/types.h>
#include <limits.h>
typedef struct
{
	_Atomic int refcount;
	off_t seek;
	mutex_t seek_lock;
	vfsnode_t *vfs_node;
	int flags;
} file_desc_t;
typedef struct
{
	/* Current working directory */
	vfsnode_t *cwd;
	mutex_t fdlock;
	file_desc_t **file_desc;
	int file_desc_entries;
} ioctx_t;

#endif
