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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <libusb.h>

#define SAMSUNG_VENDOR_ID 0x04e8
#define SAMSUNG_STORAGE_ID 0x689a
#define SAMSUNG_GTB3730_ID 0x6889

#define STORAGE_INTERFACE 0
#define STORAGE_ENDPOINT_OUT 0x6
#define STORAGE_ENDPOINT_IN 0x85

#define CONTROL_INTERFACE 0
#define CONTROL_ENDPOINT_OUT 0x2
#define CONTROL_ENDPOINT_IN 0x81

#define MODEM_INTERFACE 1
#define MODEM_ENDPOINT_OUT 0x4
#define MODEM_ENDPOINT_IN 0x83



struct libusb_context * ctx;

static int detach_storage()
{

  int i, r, received;
  libusb_device_handle *devh;
  static const unsigned char switch_msg[31] = {0x55,0x53,0x42,0x43,0x78,0x56,0x34,0x12,0x01,0x00,0x00,0x00,0x80,0x00,0x060,0x01};


  devh = libusb_open_device_with_vid_pid(ctx, SAMSUNG_VENDOR_ID, SAMSUNG_STORAGE_ID);
  if(devh == NULL){
    printf("Could not open Device\nissue a lsusb and make sure that the device is present and in storage mode! (04e8:689a)\n");
    return -1;
  }
  printf("Open success\n");
  
  if(!devh){
    printf("Could not open Device\n");
    return -1;
  }
  
  if(libusb_kernel_driver_active(devh, STORAGE_INTERFACE)) {
    libusb_detach_kernel_driver(devh, STORAGE_INTERFACE);
  }
  if(r = libusb_claim_interface(devh, STORAGE_INTERFACE)) {
    printf("claim: %d\n", r);
    perror("\n");
  }
  if(r = libusb_bulk_transfer(devh, STORAGE_ENDPOINT_OUT, (unsigned char*)switch_msg, sizeof(switch_msg), &received, 0)) {
    printf("detach bulk_transfer: %d\n", r);
  }
  if(r = libusb_release_interface(devh, STORAGE_INTERFACE)) {
    printf("release: %d\n", r);
  }

  libusb_reset_device(devh);
  return 0;
}




int main()
{
  
  //  libusb_context *context=NULL;
  libusb_init(&ctx);
  libusb_set_debug(ctx, 3);
  
  detach_storage();
   

  return 0;

}

