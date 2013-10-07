/* Samsung GT-B3730 /GT-B3740 tap port with LTE RRC/NAS info
 * 
 *
 * By Ramtin Amin <ramtin@p1sec.com>
 *    Xavier Martin <xavier.martin@p1sec.com>
 *
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
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


int gsmtap_open(void)
{
  
  int s, i;

  
  if ((gsmtap_fd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1){
    printf("Error openning gsmtap socket\n");
  }
  
  memset((char *) &si_other, 0, sizeof(si_other));
  si_other.sin_family = AF_INET;
  si_other.sin_port = htons(GSMTAP_UDP_PORT);
  if (inet_aton("192.168.0.13", &si_other.sin_addr)==0) {
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
  
  gh->version = GSMTAP_VERSION;
  gh->hdr_len = sizeof(struct gsmtap_hdr)/4;
  gh->sub_type = GSMTAP_CHANNEL_UNKNOWN;
  gh->arfcn = htons(GSMTAP_ARFCN_F_UPLINK);

  memcpy(tosend + sizeof(struct gsmtap_hdr), buf, len);

  if (sendto(gsmtap_fd, tosend, len + sizeof(struct gsmtap_hdr), 0, (const struct sockaddr *)&si_other, slen)==-1){
    printf("Error sending to gsm tap!\n");
  }


}
