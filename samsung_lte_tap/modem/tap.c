#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tapcfg.h"

tapcfg_t *tapcfg;

int create_tap()
{
 

  int fd=0;

  tapcfg = tapcfg_init();  
  tapcfg_start(tapcfg, "tap1", 0);
  fd = tapcfg_get_fd(tapcfg);

  printf("fd: %d\n",fd);

  return fd;

}

void tap_set_macaddr(unsigned char *ethaddr)
{
  tapcfg_iface_set_hwaddr(tapcfg, ethaddr, 6);
  tapcfg_iface_set_status(tapcfg, TAPCFG_STATUS_ALL_UP);
}

void tap_set_mtu(int tapfd, char *tap_dev, int mtu)
{
  tapcfg_iface_set_mtu(tapcfg,mtu);
}

void tap_write(char *buf, int len)
{
  int cnt;
  cnt = tapcfg_write(tapcfg, buf, len);
}

void tap_read()
{
  unsigned char *tosend;
  int read=0;

  tosend = malloc(10006);
  //printf("TAP_READ\n");
  
  read =  tapcfg_read(tapcfg, tosend + 6, 10006);
  tosend[0] = 0x57;
  tosend[1] = 0x44;
  tosend[2] = (read+2) & 0xff;
  tosend[3] = (read +2) >> 8;
  tosend[4] = tosend[12];
  tosend[5] = tosend[13];
  //memcpy(tosend + 6, tosend, read);
  lte_send_ctrl( tosend, read + 6);
  free(tosend);

}
