/* pvz_net.h -- real sockets for the mod's multiplayer (pvz_net.c). */
#ifndef PVZ_NET_H
#define PVZ_NET_H
#include "so_util.h"

/* libHomura's socket imports; resolved before the generic table. */
extern DynLibFunction pvz_net_imports[];
extern int pvz_net_imports_count;

/* For bionic_io.c: a descriptor from these sockets, and its generic calls. */
int pvz_net_owns(int fd);
int pvz_net_close(int fd);
int pvz_net_fcntl(int fd, int cmd, long arg);
int pvz_net_ioctl(int fd, unsigned long req, void *arg);
short pvz_net_ready(int fd, short events);

#endif
