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
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

unsigned char *atcmd[] = {
  "AT",
  "ATE1",
  "AT+CSCS=\"UCS2\"",
  "AT+CMGF=0",
  "AT+CHANGEALLPATH?",
  "AT+VERSNAME=1,0",
  "AT+VERSNAME=1,1",
  "AT+CMEE=2",
  "AT+CGREG=2",
  "AT+CFUN=5",
  "AT+CPIN?",
  "AT+CNUM",
  "AT+MODESELECT=2",
  "AT+CSQ?",
  "AT+COPSNAME",
  "AT+CSQ?",
  "AT+CGACT?",
  "AT+CGDCONT=1,\"IP\",\"orange.fr\"",
  "AT+CGATT=1",
  "AT+CGACT=1,1"
};

/*
unsigned char *atcmd[] = {
  "AT",
  "ATE0",
  "AT+CGREG=2",
  "AT+CFUN=5",
  "AT+MODESELECT=3",
  "AT+CHANGEALLPATH=1",
  "AT+CGACT?",
  "AT+CGDCONT=1,\"IP\",\"orange.fr\"",
  "AT+CGACT?",
  "AT+CGATT=1",
  "AT+CGACT=1,1",
  "AT+CGACT=1,1",
};
*/



int state=1;
int nwstate = 0;
int err=0;
void parse_at_cmd(char *c)
{
  int i;
  char *ok = NULL;
  char *nwstateind = NULL;
  char *erra = NULL ;

 
  ok = strstr(c,"OK");
  erra = strstr(c,"ERROR");
  nwstateind = strstr(c,"+NWSTATEIND:");

  printf("received |%s|\n",c);
  if(ok){
    printf("OK\n");
    state++;
  }
  if(nwstateind){
    printf("nwstateind\n");
    sscanf(nwstateind + 12,"%d\r\n",&nwstate);
    printf("new nwstate = %d\n",nwstate);
  }

  if(erra){
    printf("ERROR\n");
    err=1;
  }
  return;
}


void modem_response(char *c, int len)
{
  int i;
  
  printf("received: (%d)\n",len);
  c[len] = 0;
  parse_at_cmd(c);
}

void send_at_cmd(char *c)
{
  char buf[100];
  
  memset(buf,0,100);
  sprintf(buf,"%s\r\n",c);
  printf("sending |%s|\n",buf);
  lte_send_modem(buf);
  
}

void modem_process()
{
  
  static int curstate = 0;

  if(err){
    send_at_cmd(atcmd[state]);
    err=0;
    return;
  }

  if(curstate != state){
    if(state <= 18){
      send_at_cmd(atcmd[state]);
      curstate = state;
      sleep(1);
    }else {
      connected();
    }
  } 

}
