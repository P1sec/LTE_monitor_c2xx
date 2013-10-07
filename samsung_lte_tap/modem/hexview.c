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

int hexprintf(unsigned char *c, unsigned int len)
{

  int i,j;
  char tmp[16];

  while(len){
    for(i = 0; i < 16; i++){
      len--;
      printf("%02x ",*c);
      tmp[i] = *c;
      c++;
      /*if(i == 7){
	printf("  ");
	}*/

      if(!len){
	break;
      }
    }

    for(j = 0; j < 15 -i; j++){
      printf("   ");
    }

    printf("  |");
    for(j = 0; j < i; j++){
      if((tmp[j] > 32) && (tmp[j] < 128)){
	printf("%c",tmp[j]);
      } else {
	printf(".");
      }
    }
    for(j = 0; j < 16-i; j++){
      printf(" ");
    }
    printf("|\n");
  }
}


