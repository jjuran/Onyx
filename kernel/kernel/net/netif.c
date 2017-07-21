/*
* Copyright (c) 2017 Pedro Falcato
* This file is part of Onyx, and is released under the terms of the MIT License
* check LICENSE at the root directory for more information
*/
#include <assert.h>
#include <errno.h>

#include <kernel/netif.h>
#include <kernel/spinlock.h>
#include <kernel/dev.h>
#include <kernel/udp.h>

#include <sys/ioctl.h>

static spinlock_t netif_list_lock = {0};
struct netif *netif_list = NULL;
unsigned int netif_ioctl(int request, void *argp, vfsnode_t* this)
{
	struct netif *netif = this->helper;
	assert(netif);
	switch(request)
	{
		case SIOSETINET4:
		{
			if(vmm_check_pointer(argp, sizeof(struct if_config_inet)) < 0)
				return -EFAULT;
			struct if_config_inet *c = argp;
			struct sockaddr_in *local = (struct sockaddr_in*) &netif->local_ip;
			memcpy(&local->sin_addr, &c->address, sizeof(struct in_addr));
			struct sockaddr_in *router = (struct sockaddr_in*) &netif->router_ip;
			memcpy(&router->sin_addr, &c->router, sizeof(struct in_addr));
			return 0;
		}
		case SIOGETINET4:
		{
			if(vmm_check_pointer(argp, sizeof(struct if_config_inet)) < 0)
				return -EFAULT;
			struct if_config_inet *c = argp;
			struct sockaddr_in *local = (struct sockaddr_in*) &netif->local_ip;
			struct sockaddr_in *router = (struct sockaddr_in*) &netif->router_ip;
			memcpy(&c->address, &local->sin_addr, sizeof(struct in_addr));
			memcpy(&c->router, &router->sin_addr, sizeof(struct in_addr));
			return 0;
		}
		case SIOSETINET6:
		{
			if(vmm_check_pointer(argp, sizeof(struct if_config_inet6)) < 0)
				return -EFAULT;
			struct if_config_inet *c = argp;
			struct sockaddr_in *local = (struct sockaddr_in*) &netif->local_ip;
			memcpy(&local->sin_addr, &c->address, sizeof(struct in6_addr));
			struct sockaddr_in *router = (struct sockaddr_in*) &netif->router_ip;
			memcpy(&router->sin_addr, &c->router, sizeof(struct in6_addr));
			return 0;
		}
		case SIOGETINET6:
		{
			if(vmm_check_pointer(argp, sizeof(struct if_config_inet6)) < 0)
				return -EFAULT;
			struct if_config_inet6 *c = argp;
			struct sockaddr_in6 *local = (struct sockaddr_in6*) &netif->local_ip;
			struct sockaddr_in6 *router = (struct sockaddr_in6*) &netif->router_ip;
			memcpy(&c->address, &local->sin6_addr, sizeof(struct in6_addr));
			memcpy(&c->router, &router->sin6_addr, sizeof(struct in6_addr));
			return 0;
		}
		case SIOGETMAC:
		{
			if(copy_to_user(argp, &netif->mac_address, 6) < 0)
				return -EFAULT;
			return 0;
		}
	}
	return -ENOTTY;
}
void netif_register_if(struct netif *netif)
{
	if(udp_init_netif(netif) < 0)
		return;
	struct file_ops *fops = malloc(sizeof(struct file_ops));
	if(!fops)
		return;
	memset(fops, 0, sizeof(struct file_ops));
	struct minor_device *minor = dev_register(0, 0);
	if(!minor)
	{
		free(fops);
		return;
	}
	vfsnode_t *file = creat_vfs(slashdev, netif->name, 0644);
	if(!file)
	{
		free(fops);
		dev_unregister(minor->majorminor);
		return;
	}
	file->dev = minor->majorminor;
	minor->fops = fops;
	fops->ioctl = netif_ioctl;
	file->helper = netif;
	netif->device_file = file;
	acquire_spinlock(&netif_list_lock);
	if(!netif_list)
	{
		netif_list = netif;
	}
	else
	{
		struct netif *n = netif_list;
		while(n->next) n = n->next;
		n->next = netif;
	}
	release_spinlock(&netif_list_lock);
}
int netif_unregister_if(struct netif *netif)
{
	acquire_spinlock(&netif_list_lock);
	if(netif_list == netif)
	{
		netif_list = netif->next;
		release_spinlock(&netif_list_lock);
		return 0;
	}
	else
	{
		struct netif *n = netif_list;
		while(n)
		{
			if(n->next == netif)
			{
				n->next = netif->next;
				release_spinlock(&netif_list_lock);
				return 0;
			}
			n = n->next;
		}
	}
	return -1;
}
struct netif *netif_choose(void)
{
	for(struct netif *n = netif_list; n; n = n->next)
	{
		if(n->flags & NETIF_LINKUP)
			return n;
	}
	return NULL;
}
int netif_send_packet(struct netif *netif, const void *buffer, uint16_t size)
{
	assert(netif);
	if(netif->sendpacket)
		return netif->sendpacket(buffer, size);
	return errno = ENODEV, -1;
}
void netif_get_ipv4_addr(struct sockaddr_in *s, struct netif *netif)
{
	memcpy(&s, &netif->local_ip, sizeof(struct sockaddr));
}
