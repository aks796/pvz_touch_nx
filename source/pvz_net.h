/* pvz_net.h -- real sockets for the mod's multiplayer (pvz_net.c). */
#ifndef PVZ_NET_H
#define PVZ_NET_H
#include "so_util.h"

/* libHomura's socket imports; resolved before the generic table. */
extern DynLibFunction pvz_net_imports[];
extern int pvz_net_imports_count;

/* For the runtime's bionic_io.c (dcr_net.h): a descriptor from these
 * sockets, and its generic calls -- port_net_owns/close/fcntl/ioctl/ready,
 * defined in pvz_net.c. */
#include "dcr_net.h"

#endif
