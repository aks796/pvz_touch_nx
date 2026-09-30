/* pvz_net.c -- real sockets for the mod's multiplayer (LAN and online VS).
 *
 * The engine plays offline (bionic_net.c: its sockets never connect; its
 * servers -- Transmension's activation, "trans" relay, UDP remote input --
 * are gone). The newest mod (1.1.5) adds VS over the network, all of it in
 * libHomura (WaitForSecondPlayerDialog, NetPlay):
 *   Wi-Fi VS   the host listens on TCP (any port) and announces the room by UDP
 *              broadcast to port 8888 on every interface's broadcast address
 *              (ioctl SIOCGIFCONF / SIOCGIFFLAGS / SIOCGIFADDR / SIOCGIFBRDADDR),
 *              or 255.255.255.255; guests listen on 8888, or type IP:port.
 *   Online VS  TCP to a relay server (the mod's two, or a custom one).
 * All of it non-blocking, from widget updates on the game thread: socket,
 * bind, listen, accept, connect (EINPROGRESS, then select for writing and
 * SO_ERROR), sendto/recvfrom (send/recv inline), setsockopt, getsockname,
 * shutdown, close, fcntl O_NONBLOCK, select.
 *
 * So libHomura alone is given these (pvz_loader.c resolves its imports with
 * pvz_net_imports first), on the console's own BSD sockets (libnx bsd:u, the
 * Wi-Fi the console is on), translated from Linux: sockaddr (Linux's 16-bit
 * family vs BSD's length byte + 8-bit family), SOL_SOCKET and its option
 * numbers, MSG_ and SOCK_ flags, O_NONBLOCK, errno. The interfaces come from
 * nifm: one, "wlan0", with the console's address and its subnet's broadcast.
 * Descriptors are libnx's (newlib's, small numbers: select's 1024-bit sets
 * hold them); bionic_io.c's close/fcntl/ioctl/select/poll hand a descriptor
 * this file owns (pvz_net_owns) to it. MIT.
 */
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <switch.h>
#include <unistd.h>

#include "bionic.h"
#include "pvz_net.h"
#include "util.h"

/* ---------------------------------------------------------------- state */
#define MAX_FD 1024
static uint8_t g_owned[MAX_FD];
static Mutex g_lock;
static int g_state; /* 0 not tried, 1 up, -1 failed */
static int g_nifm;
static unsigned g_traces;

static void trace(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void trace(const char *fmt, ...) {
  if (__atomic_fetch_add(&g_traces, 1, __ATOMIC_RELAXED) >= 64)
    return;
  char line[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(line, sizeof line, fmt, ap);
  va_end(ap);
  debugPrintf("[net] %s\n", line);
}

int pvz_net_owns(int fd) { return fd >= 0 && fd < MAX_FD && g_owned[fd]; }

static void own(int fd, int yes) {
  if (fd >= 0 && fd < MAX_FD)
    g_owned[fd] = (uint8_t)yes;
}

static int ip_config(uint32_t *addr, uint32_t *mask);
static void selftest(void);

/* The BSD sockets, on first use: a player who never opens the VS menus never
 * starts them. */
static int net_up(void) {
  mutexLock(&g_lock);
  if (!g_state) {
    SocketInitConfig cfg = *socketGetDefaultInitConfig();
    cfg.tcp_tx_buf_size = 0x8000;
    cfg.tcp_rx_buf_size = 0x10000;
    cfg.tcp_tx_buf_max_size = 0x40000;
    cfg.tcp_rx_buf_max_size = 0x40000;
    cfg.sb_efficiency = 4;
    cfg.num_bsd_sessions = 3;
    cfg.bsd_service_type = BsdServiceType_User;
    Result rc = socketInitialize(&cfg);
    g_state = R_SUCCEEDED(rc) ? 1 : -1;
    g_nifm = R_SUCCEEDED(nifmInitialize(NifmServiceType_User));
    uint32_t a = 0, m = 0;
    if (g_state < 0)
      debugPrintf("[net] the console's sockets could not start (0x%x): no LAN or online VS\n",
                  (unsigned)rc);
    else if (ip_config(&a, &m))
      debugPrintf("[net] sockets up; this console is %u.%u.%u.%u/%u.%u.%u.%u\n", a & 255,
                  a >> 8 & 255, a >> 16 & 255, a >> 24, m & 255, m >> 8 & 255, m >> 16 & 255,
                  m >> 24);
    else
      debugPrintf("[net] sockets up, but the console has no network connection (nifm %s)\n",
                  g_nifm ? "reports none" : "unavailable");
  }
  int ok = g_state > 0;
  static int tested;
  int test = ok && !tested;
  tested = 1;
  mutexUnlock(&g_lock);
  if (test)
    selftest();
  if (!ok)
    b_set_errno(L_ENETDOWN);
  return ok;
}

/* nifm gives the address and mask as they sit in memory for in_addr. */
static int ip_config(uint32_t *addr, uint32_t *mask) {
  if (!g_nifm)
    return 0;
  u32 a = 0, m = 0, gw = 0, d1 = 0, d2 = 0;
  if (R_FAILED(nifmGetCurrentIpConfigInfo(&a, &m, &gw, &d1, &d2)) || !a)
    return 0;
  *addr = a;
  *mask = m;
  return 1;
}

/* ---------------------------------------------------------------- errno */
/* FreeBSD numbers (the console's socket service; SO_ERROR hands them back as
 * they are) -> Linux. */
static int bsd_to_linux(int e) {
  static const uint8_t map[][2] = {
      {35, L_EAGAIN},        {36, L_EINPROGRESS},   {37, L_EALREADY},     {38, L_ENOTSOCK},
      {39, 89 /* EDESTADDRREQ */}, {40, 90 /* EMSGSIZE */}, {41, 91 /* EPROTOTYPE */},
      {42, 92 /* ENOPROTOOPT */}, {43, 93 /* EPROTONOSUPPORT */}, {45, L_EOPNOTSUPP},
      {47, L_EAFNOSUPPORT},  {48, 98 /* EADDRINUSE */}, {49, 99 /* EADDRNOTAVAIL */},
      {50, L_ENETDOWN},      {51, L_ENETUNREACH},   {52, 102 /* ENETRESET */},
      {53, 103 /* ECONNABORTED */}, {54, L_ECONNRESET}, {55, 105 /* ENOBUFS */},
      {56, 106 /* EISCONN */}, {57, L_ENOTCONN},    {60, L_ETIMEDOUT},    {61, L_ECONNREFUSED},
      {64, 112 /* EHOSTDOWN */}, {65, L_EHOSTUNREACH},
  };
  for (unsigned i = 0; i < sizeof map / sizeof map[0]; i++)
    if (map[i][0] == e)
      return map[i][1];
  return e;
}

static int fail(void) {
  b_fix_errno();
  return -1;
}

/* ---------------------------------------------------------------- sockaddr */
#define L_AF_INET 2
#define L_AF_INET6 10
#define ADDR_MAX 128

/* Linux: u16 family, then the address. BSD: u8 length, u8 family. */
static int addr_in(const void *lin, unsigned len, uint8_t *out) {
  if (!lin || len < 2 || len > ADDR_MAX)
    return 0;
  memcpy(out, lin, len);
  uint16_t fam;
  memcpy(&fam, lin, 2);
  out[0] = (uint8_t)len;
  out[1] = (uint8_t)(fam == L_AF_INET6 ? AF_INET6 : fam);
  return 1;
}

static void addr_out(const uint8_t *bsd, unsigned have, void *lin, unsigned *lin_len) {
  if (!lin || !lin_len)
    return;
  unsigned n = have < *lin_len ? have : *lin_len;
  if (n >= 2) {
    memcpy(lin, bsd, n);
    uint16_t fam = bsd[1] == AF_INET6 ? L_AF_INET6 : bsd[1];
    memcpy(lin, &fam, 2);
  }
  *lin_len = have;
}

/* ---------------------------------------------------------------- flags */
#define L_SOCK_NONBLOCK 0x800
#define L_SOCK_CLOEXEC 0x80000
#define L_O_NONBLOCK_ 0x800

static int msg_flags(int f) {
  int o = 0;
  if (f & 0x1) o |= MSG_OOB;
  if (f & 0x2) o |= MSG_PEEK;
  if (f & 0x4) o |= MSG_DONTROUTE;
  if (f & 0x40) o |= MSG_DONTWAIT;
  if (f & 0x100) o |= MSG_WAITALL;
  /* MSG_NOSIGNAL (0x4000): there are no signals here */
  return o;
}

static int set_nonblock(int fd, int on) {
  int fl = fcntl(fd, F_GETFL, 0);
  if (fl < 0)
    return -1;
  return fcntl(fd, F_SETFL, on ? (fl | O_NONBLOCK) : (fl & ~O_NONBLOCK));
}

/* ---------------------------------------------------------------- calls */
static int n_socket(int domain, int type, int proto) {
  if (!net_up())
    return -1;
  int fam = domain == L_AF_INET6 ? AF_INET6 : domain;
  int fd = socket(fam, type & ~(L_SOCK_NONBLOCK | L_SOCK_CLOEXEC), proto);
  if (fd < 0) {
    trace("socket(%d, %d) failed: errno %d", domain, type, errno);
    return fail();
  }
  if (fd >= MAX_FD) {
    close(fd);
    b_set_errno(L_EMFILE);
    return -1;
  }
  if (type & L_SOCK_NONBLOCK)
    set_nonblock(fd, 1);
  own(fd, 1);
  return fd;
}

static int n_bind(int fd, const void *a, unsigned len) {
  uint8_t h[ADDR_MAX];
  if (!addr_in(a, len, h)) {
    b_set_errno(L_EINVAL);
    return -1;
  }
  int r = bind(fd, (struct sockaddr *)h, len);
  if (r < 0) {
    trace("bind(%d, port %u) failed: errno %d", fd, (unsigned)ntohs(*(uint16_t *)(h + 2)), errno);
    return fail();
  }
  return 0;
}

static int n_listen(int fd, int backlog) { return listen(fd, backlog) < 0 ? fail() : 0; }

static int n_accept4(int fd, void *a, unsigned *len, int flags) {
  uint8_t h[ADDR_MAX];
  socklen_t hl = sizeof h;
  int c = accept(fd, (struct sockaddr *)h, &hl);
  if (c < 0)
    return fail();
  if (c >= MAX_FD) {
    close(c);
    b_set_errno(L_EMFILE);
    return -1;
  }
  own(c, 1);
  if (flags & L_SOCK_NONBLOCK)
    set_nonblock(c, 1);
  addr_out(h, hl, a, len);
  trace("accept(%d) -> %d", fd, c);
  return c;
}

static int n_accept(int fd, void *a, unsigned *len) { return n_accept4(fd, a, len, 0); }

static int n_connect(int fd, const void *a, unsigned len) {
  uint8_t h[ADDR_MAX];
  if (!addr_in(a, len, h)) {
    b_set_errno(L_EINVAL);
    return -1;
  }
  if (connect(fd, (struct sockaddr *)h, len) < 0) {
    int e = errno;
    if (e != EINPROGRESS)
      trace("connect(%d, %u.%u.%u.%u:%u) failed: errno %d", fd, h[4], h[5], h[6], h[7],
            (unsigned)ntohs(*(uint16_t *)(h + 2)), e);
    return fail();
  }
  return 0;
}

static ssize_t n_sendto(int fd, const void *b, size_t n, int f, const void *a, unsigned len) {
  uint8_t h[ADDR_MAX];
  ssize_t r;
  if (a && len) {
    if (!addr_in(a, len, h)) {
      b_set_errno(L_EINVAL);
      return -1;
    }
    r = sendto(fd, b, n, msg_flags(f), (struct sockaddr *)h, len);
  } else {
    r = send(fd, b, n, msg_flags(f));
  }
  return r < 0 ? fail() : r;
}

static ssize_t n_send(int fd, const void *b, size_t n, int f) { return n_sendto(fd, b, n, f, NULL, 0); }

static ssize_t n_recvfrom(int fd, void *b, size_t n, int f, void *a, unsigned *len) {
  uint8_t h[ADDR_MAX];
  socklen_t hl = sizeof h;
  ssize_t r = a && len ? recvfrom(fd, b, n, msg_flags(f), (struct sockaddr *)h, &hl)
                       : recv(fd, b, n, msg_flags(f));
  if (r < 0)
    return fail();
  if (a && len)
    addr_out(h, hl, a, len);
  return r;
}

static ssize_t n_recv(int fd, void *b, size_t n, int f) { return n_recvfrom(fd, b, n, f, NULL, NULL); }

/* Linux SOL_SOCKET options -> BSD; -1: none here */
static int so_opt(int o) {
  switch (o) {
  case 2: return SO_REUSEADDR;
  case 3: return SO_TYPE;
  case 4: return SO_ERROR;
  case 6: return SO_BROADCAST;
  case 7: return SO_SNDBUF;
  case 8: return SO_RCVBUF;
  case 9: return SO_KEEPALIVE;
  case 10: return SO_OOBINLINE;
  case 13: return SO_LINGER;
  case 15: return SO_REUSEPORT;
  case 18: return SO_RCVLOWAT;
  case 19: return SO_SNDLOWAT;
  case 20: return SO_RCVTIMEO;
  case 21: return SO_SNDTIMEO;
  default: return -1;
  }
}

/* level/option -> BSD; 0 if it has no counterpart (then quietly "done") */
static int opt_map(int lvl, int opt, int *hl, int *ho) {
  if (lvl == 1) { /* SOL_SOCKET */
    *hl = SOL_SOCKET;
    *ho = so_opt(opt);
    return *ho >= 0;
  }
  if (lvl == IPPROTO_TCP) {
    *hl = IPPROTO_TCP;
    switch (opt) {
    case 1: *ho = TCP_NODELAY; return 1;
    case 4: *ho = TCP_KEEPIDLE; return 1;
    case 5: *ho = TCP_KEEPINTVL; return 1;
    case 6: *ho = TCP_KEEPCNT; return 1;
    default: return 0;
    }
  }
  if (lvl == IPPROTO_IP) {
    *hl = IPPROTO_IP;
    switch (opt) {
    case 1: *ho = IP_TOS; return 1;
    case 2: *ho = IP_TTL; return 1;
    default: return 0;
    }
  }
  return 0;
}

static int n_setsockopt(int fd, int lvl, int opt, const void *v, unsigned l) {
  int hl, ho;
  if (!opt_map(lvl, opt, &hl, &ho)) {
    trace("setsockopt(%d, %d, %d): not supported here, ignored", fd, lvl, opt);
    return 0;
  }
  if (setsockopt(fd, hl, ho, v, l) < 0) {
    /* keep-alive tuning and port sharing are wishes, not needs */
    if (hl == IPPROTO_TCP || ho == SO_REUSEPORT || ho == SO_KEEPALIVE)
      return 0;
    trace("setsockopt(%d, %d, %d) failed: errno %d", fd, lvl, opt, errno);
    return fail();
  }
  return 0;
}

static int n_getsockopt(int fd, int lvl, int opt, void *v, unsigned *l) {
  int hl, ho;
  if (!opt_map(lvl, opt, &hl, &ho)) {
    b_set_errno(92 /* ENOPROTOOPT */);
    return -1;
  }
  socklen_t sl = l ? *l : 0;
  if (getsockopt(fd, hl, ho, v, &sl) < 0)
    return fail();
  if (l)
    *l = sl;
  if (hl == SOL_SOCKET && ho == SO_ERROR && v && sl >= sizeof(int)) {
    int e = *(int *)v;
    *(int *)v = e ? bsd_to_linux(e) : 0;
  }
  return 0;
}

static int name_call(int fd, void *a, unsigned *len, int peer) {
  uint8_t h[ADDR_MAX];
  socklen_t hl = sizeof h;
  int r = peer ? getpeername(fd, (struct sockaddr *)h, &hl) : getsockname(fd, (struct sockaddr *)h, &hl);
  if (r < 0)
    return fail();
  addr_out(h, hl, a, len);
  return 0;
}

static int n_getsockname(int fd, void *a, unsigned *len) { return name_call(fd, a, len, 0); }
static int n_getpeername(int fd, void *a, unsigned *len) { return name_call(fd, a, len, 1); }
static int n_shutdown(int fd, int how) { return shutdown(fd, how) < 0 ? fail() : 0; }

/* ------------------------------------------- from bionic_io.c's generic calls */
int pvz_net_close(int fd) {
  own(fd, 0);
  return close(fd) < 0 ? fail() : 0;
}

#define L_F_GETFL_ 3
#define L_F_SETFL_ 4
int pvz_net_fcntl(int fd, int cmd, long arg) {
  if (cmd == L_F_GETFL_) {
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl < 0)
      return fail();
    return 2 /* O_RDWR */ | ((fl & O_NONBLOCK) ? L_O_NONBLOCK_ : 0);
  }
  if (cmd == L_F_SETFL_)
    return set_nonblock(fd, (arg & L_O_NONBLOCK_) != 0) < 0 ? fail() : 0;
  return 0; /* F_GETFD/F_SETFD (close-on-exec) and the like */
}

/* ---- ioctl: the interface list (Linux struct ifreq: 16-byte name + a 16-byte
 * union; struct ifconf: int length + a pointer) ---- */
struct l_ifreq {
  char name[16];
  union {
    uint8_t addr[16]; /* Linux sockaddr */
    short flags;
  } u;
};
struct l_ifconf {
  int len;
  void *buf;
};

#define L_FIONREAD 0x541B
#define L_FIONBIO 0x5421
#define L_SIOCGIFCONF 0x8912
#define L_SIOCGIFFLAGS 0x8913
#define L_SIOCGIFADDR 0x8915
#define L_SIOCGIFBRDADDR 0x8919
#define L_SIOCGIFNETMASK 0x891b
#define IFNAME "wlan0"

static void put_in(uint8_t *sa, uint32_t addr) {
  memset(sa, 0, 16);
  uint16_t fam = L_AF_INET;
  memcpy(sa, &fam, 2);
  memcpy(sa + 4, &addr, 4);
}

int pvz_net_ioctl(int fd, unsigned long req, void *arg) {
  uint32_t a, m;
  switch (req) {
  case L_FIONBIO:
    return set_nonblock(fd, arg && *(int *)arg) < 0 ? fail() : 0;
  case L_FIONREAD: {
    int n = 0;
    if (ioctl(fd, FIONREAD, &n) < 0)
      return fail();
    if (arg)
      *(int *)arg = n;
    return 0;
  }
  case L_SIOCGIFCONF: {
    struct l_ifconf *c = arg;
    if (!c) {
      b_set_errno(L_EFAULT);
      return -1;
    }
    int have = ip_config(&a, &m);
    if (!have || !c->buf || c->len < (int)sizeof(struct l_ifreq)) {
      c->len = 0; /* no network: no interface */
      return 0;
    }
    struct l_ifreq *r = c->buf;
    memset(r, 0, sizeof *r);
    strcpy(r->name, IFNAME);
    put_in(r->u.addr, a);
    c->len = sizeof *r;
    return 0;
  }
  case L_SIOCGIFFLAGS:
  case L_SIOCGIFADDR:
  case L_SIOCGIFBRDADDR:
  case L_SIOCGIFNETMASK: {
    struct l_ifreq *r = arg;
    if (!r || strncmp(r->name, IFNAME, sizeof r->name) || !ip_config(&a, &m)) {
      b_set_errno(19 /* ENODEV */);
      return -1;
    }
    if (req == L_SIOCGIFFLAGS)
      r->u.flags = 0x1 | 0x2 | 0x40 | 0x1000; /* UP BROADCAST RUNNING MULTICAST */
    else
      put_in(r->u.addr, req == L_SIOCGIFADDR ? a : req == L_SIOCGIFNETMASK ? m : (a | ~m));
    return 0;
  }
  default:
    b_set_errno(L_ENOTTY);
    return -1;
  }
}

/* Readiness now (poll with no wait), in Linux poll bits (the same numbers).
 * A descriptor the console gave up on (after sleep, a reset connection)
 * reports only an error: the bits asked for are added, so the caller goes
 * on to read or check SO_ERROR and learns of it, instead of waiting forever. */
short pvz_net_ready(int fd, short events) {
  struct pollfd p = {fd, events, 0};
  if (poll(&p, 1, 0) < 0)
    return 0x20; /* POLLNVAL */
  short r = p.revents;
  if ((r & (POLLERR | POLLHUP | POLLNVAL)) && !(r & events))
    r |= events;
  return r;
}

/* ---------------------------------------------------------------- self-test */
/* Once, when the mod first asks for a socket: the calls it makes, through the
 * Linux-side entry points (so the translation is what is tested), on this
 * console alone -- TCP to itself, UDP to itself, and a broadcast on the mod's
 * port. Only the log says how it went. */
static void l_addr(uint8_t sa[16], uint32_t ip, uint16_t port) {
  memset(sa, 0, 16);
  uint16_t fam = L_AF_INET;
  memcpy(sa, &fam, 2);
  uint16_t p = htons(port);
  memcpy(sa + 2, &p, 2);
  memcpy(sa + 4, &ip, 4);
}

static uint16_t l_port(int fd) {
  uint8_t sa[16];
  unsigned l = sizeof sa;
  if (n_getsockname(fd, sa, &l) < 0)
    return 0;
  uint16_t p;
  memcpy(&p, sa + 2, 2);
  return ntohs(p);
}

static int wait_ready(int fd, short ev, int ms) {
  for (int i = 0; i < ms; i++) {
    short r = pvz_net_ready(fd, ev);
    if (r & ev)
      return 1;
    svcSleepThread(1000000);
  }
  return 0;
}

static void selftest(void) {
  const uint32_t lo = htonl(0x7f000001);
  uint8_t sa[16];
  int one = 1;
  /* TCP: listen on any port, non-blocking connect, accept, a message across */
  const char *tcp = "failed";
  int ls = n_socket(L_AF_INET, 1, 0), cs = n_socket(L_AF_INET, 1 | L_SOCK_NONBLOCK, 0), as = -1;
  l_addr(sa, 0, 0);
  n_setsockopt(ls, 1, 2, &one, sizeof one); /* SO_REUSEADDR */
  if (ls >= 0 && cs >= 0 && n_bind(ls, sa, 16) == 0 && n_listen(ls, 1) == 0) {
    uint16_t port = l_port(ls);
    l_addr(sa, lo, port);
    int r = n_connect(cs, sa, 16);
    if ((r == 0 || errno == L_EINPROGRESS) && wait_ready(cs, POLLOUT, 1000)) {
      int err = -1;
      unsigned el = sizeof err;
      n_getsockopt(cs, 1, 4, &err, &el); /* SO_ERROR */
      as = err == 0 && wait_ready(ls, POLLIN, 1000) ? n_accept(ls, NULL, NULL) : -1;
      char buf[8] = {0};
      if (as >= 0 && n_sendto(cs, "pvz", 3, 0x4000 /* NOSIGNAL */, NULL, 0) == 3 &&
          wait_ready(as, POLLIN, 1000) && n_recvfrom(as, buf, sizeof buf, 0x40, NULL, NULL) == 3 &&
          !memcmp(buf, "pvz", 3))
        tcp = "OK";
      else if (err)
        tcp = "connect refused";
    }
  }
  /* UDP: a datagram to itself */
  const char *udp = "failed";
  int us = n_socket(L_AF_INET, 2, 0);
  l_addr(sa, lo, 0);
  if (us >= 0 && n_bind(us, sa, 16) == 0) {
    l_addr(sa, lo, l_port(us));
    char buf[8] = {0};
    uint8_t from[16];
    unsigned fl = sizeof from;
    if (n_sendto(us, "pvz", 3, 0, sa, 16) == 3 && wait_ready(us, POLLIN, 500) &&
        n_recvfrom(us, buf, sizeof buf, 0x40, from, &fl) == 3)
      udp = "OK";
  }
  /* broadcast: a room announcement on the mod's port, heard by a listener on
   * it (a console need not hear its own broadcast: then only sending counts) */
  const char *bc = "no network";
  uint32_t a, m;
  if (ip_config(&a, &m)) {
    int rx = n_socket(L_AF_INET, 2, 0), tx = n_socket(L_AF_INET, 2, 0);
    n_setsockopt(rx, 1, 2, &one, sizeof one);
    n_setsockopt(rx, 1, 6, &one, sizeof one); /* SO_BROADCAST */
    n_setsockopt(tx, 1, 6, &one, sizeof one);
    l_addr(sa, 0, 8888);
    int bound = rx >= 0 && n_bind(rx, sa, 16) == 0;
    l_addr(sa, a | ~m, 8888);
    ssize_t sent = tx >= 0 ? n_sendto(tx, "pvz", 3, 0, sa, 16) : -1;
    char buf[8];
    if (sent != 3)
      bc = "could not send";
    else if (!bound)
      bc = "sent; port 8888 busy";
    else if (wait_ready(rx, POLLIN, 300) && n_recvfrom(rx, buf, sizeof buf, 0x40, NULL, NULL) == 3)
      bc = "OK (heard its own)";
    else
      bc = "sent (not heard back here, which is fine)";
    if (rx >= 0) pvz_net_close(rx);
    if (tx >= 0) pvz_net_close(tx);
  }
  debugPrintf("[net] self-test: TCP to itself %s, UDP to itself %s, LAN broadcast %s\n", tcp, udp, bc);
  int fds[] = {ls, cs, as, us};
  for (unsigned i = 0; i < sizeof fds / sizeof fds[0]; i++)
    if (fds[i] >= 0)
      pvz_net_close(fds[i]);
}

/* ---------------------------------------------------------------- table */
#define F(n) {#n, (uintptr_t)n_##n}
DynLibFunction pvz_net_imports[] = {
    F(socket),     F(bind),       F(listen),      F(accept),      F(accept4),
    F(connect),    F(send),       F(recv),        F(sendto),      F(recvfrom),
    F(setsockopt), F(getsockopt), F(getsockname), F(getpeername), F(shutdown),
};
int pvz_net_imports_count = sizeof pvz_net_imports / sizeof pvz_net_imports[0];
