/*
* Copyright (c) 2016, 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/utsname.h>

#include <kernel/kernelinfo.h>
#include <kernel/vmm.h>
#include <kernel/network.h>
int sys_uname(struct utsname *ubuf)
{
	struct utsname buf = {0};
	strcpy(buf.sysname, OS_NAME);
	strcpy(buf.release, OS_RELEASE);
	strcpy(buf.version, OS_VERSION);
	strcpy(buf.machine, OS_MACHINE);

	strcpy(buf.nodename, network_gethostname());
	if(copy_to_user(ubuf, &buf, sizeof(struct utsname)) < 0)
		return -EFAULT;
	return 0;
}
