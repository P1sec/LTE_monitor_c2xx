#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "gsmtap.h"


int gsmtap_fd=0;
struct sockaddr_in si_other;


int gsmtap_open(char *ip)
{
  
  int s, i;

  
  if ((gsmtap_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1){
    printf("Error openning gsmtap socket\n");
  }
  
  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(GSMTAP_UDP_PORT);
  if (inet_aton(ip, &si_other.sin_addr)==0) {
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }  
  return 0;
}


void gsmtap_send(char *buf, int len)
{
  char *tosend;
  struct gsmtap_hdr *gh;
  int slen=sizeof(si_other);

  tosend = malloc(len + sizeof(struct gsmtap_hdr));

  gh = (struct gsmtap_hdr*)tosend;
  
  gh->type = GSMTAP_TYPE_C2XX;
  gh->version = GSMTAP_VERSION;
  gh->hdr_len = sizeof(struct gsmtap_hdr)/4;
  gh->sub_type = GSMTAP_CHANNEL_UNKNOWN;
  gh->arfcn = htons(GSMTAP_ARFCN_F_UPLINK);

  memcpy(tosend + sizeof(struct gsmtap_hdr), buf, len);

  if (sendto(gsmtap_fd, tosend, len + sizeof(struct gsmtap_hdr), 0, (const struct sockaddr *)&si_other, slen)==-1){
    printf("Error sending to gsm tap!\n");
  }


}
