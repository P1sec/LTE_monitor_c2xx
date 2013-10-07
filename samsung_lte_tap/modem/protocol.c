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
#include <string.h>
#include <stdlib.h>

int process_config_response(void *dev, const unsigned char *buf, int len)
{
  //printf("Config:\n");
  //  hexprintf(buf,len);
  gsmtap_send(buf,len);
  return 0;
}

int process_data_response(void *dev, const unsigned char *buf, int len)
{
  //printf("Data:\n");
  //  hexprintf(buf,len);
  tap_write(buf+6, len - 6);
  return 0;
}

int process_P_response(void *dev, const unsigned char *buf, int len)
{
  printf("P!:\n");
  unsigned char macaddr[6];
  int i;
  char resp[] = {0x57, 0x50, 0x04, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x01, 0x52, 0x00};
  switch(buf[2]){
  case 0x12:
    memcpy(macaddr,buf + 10, 6);
    for(i = 0; i < 6; i++){
      printf("%02x:",macaddr[i]);
    }
    printf("\n");
    tap_set_macaddr(macaddr);
    break;
  case 0x0e:
    hexprintf(buf,len);
    //lte_send_ctrl(resp,sizeof(resp));
    break;
  default:
    printf("UNKNOWN!\n");
    hexprintf(buf,len);
    break;  
  }

  return 0;
}




int process_response(void *dev, const unsigned char *buf, int len)
{
  int check_len;

  if(len < 4) {
    printf( "short read");
    return -1;
  }

  if(buf[0] != 0x57) {
    printf( "bad header");
    return -1;
  }

  check_len = 4 + buf[2] + (buf[3] << 8);
  if(buf[1] == 0x43 || buf[1] == 0x44 || buf[1] == 0x50) {
    check_len += 2;
  }

  /*
  if(check_len != len) {
    printf("bad length: %02x instead of %02x\n", check_len, len);
    hexprintf(buf,len);
    return -1;
  }
  */

  switch (buf[1]) {
  case 0x43:
    return process_config_response(dev, buf, len);
  case 0x44:
    return process_data_response(dev, buf, len);
  case 0x50:
    return process_P_response(dev, buf, len);
  default:
    printf("bad response type: %02x", buf[1]);
    return -1;
  }
}


/*
 cnt =  tapcfg_read(tapcfg, buf, 10000);

      printf("received:%d\n",cnt);
      tosend = malloc(cnt + 6);
      tosend[0] = 0x57;
      tosend[1] = 0x44;
      tosend[2] = (cnt+2) & 0xff;
      tosend[3] = (cnt +2) >> 8;
      tosend[4] = buf[12];
      tosend[5] = buf[13];
      memcpy(tosend + 6, buf,cnt);

      hexprintf(tosend,cnt + 6);

      send_ctrl(devh, tosend, cnt + 6);
      //cnt = tapcfg_write(tapcfg,tosend,cnt+6);

      free(tosend);

*/
