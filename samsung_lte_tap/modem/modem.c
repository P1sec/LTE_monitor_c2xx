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
#include "at_cmd.h"


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

int state=1;
int nwstate = 0;
int err=0;
atctx_t *mctx;



void modem_response(char *c, int len)
{
  int i;

  printf("received: (%d)\n",len);
  printf("%s\n",c);
  c[len] = 0;

  
  at_parse(mctx, c, len);
  
}

void send_at_cmd(char *c)
{
  char buf[100];
  
  memset(buf,0,100);
  sprintf(buf,"%s\r\n",c);
  //printf("sending |%s|\n",buf);
  lte_send_modem(buf);
  
}

void modem_process(unsigned char *apn)
{
  
  static int curstate = 0;
  char myapn[]="AT+CGDCONT=1,\"IP\",\"%s\"";
  char tmp[200];


  if(err){
    send_at_cmd(atcmd[state]);
    err=0;
    return;
  }

  if(curstate != state){
    if(state <= 18){
      if(state == 17){
	sprintf(tmp,myapn,apn);
	send_at_cmd(tmp);
      } else {
	send_at_cmd(atcmd[state]);
      }
      curstate = state;
      sleep(1);
    }else {
      connected();
    }
  } 
}






void default_cb(char *name, int argc, char **argv, void *user_data)
{
  int i;
  printf("default response: %s(",name);
  for(i = 0; i < argc; i++){
    if(argv[i])
      printf("%s, ",argv[i]);
    else printf("nil, ");
  }
  printf(")\n");
}

void default_cmd_cb(char *name, int argc, char **argv, void *user_data)
{
  int i;
  printf("default command: %s(",name);
  for(i = 0; i < argc; i++){
    if(argv[i])
      printf("%s, ",argv[i]);
    else printf("nil, ");
  }
  printf(")\n");
}

void myok_cb(atctx_t *ctx, void *user)
{
  //printf("GOT OK for %s\n", ctx->last_cmd);
  state++;
}

void mycallback(char *name, int argc, char **argv, void *user_data)
{
  int i;
  for(i = 0; i < argc; i++){
    printf("%s\n",argv[i]);
  }
}


void h_ate1(char *name, int argc, char **argv, void *user_data)
{
  int i;
  printf("Called: %s\n",__FUNCTION__);
}

void h_cscs(char *name, int argc, char **argv, void *user_data)
{
  int i;
  printf("Called: %s\n",__FUNCTION__);
}



void h_cmgf(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_changeallpath(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_versname(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cmee(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cgreg(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cfun(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cpin(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cnum(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_modeselect(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_csq(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_copsname(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cgact(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cgdcont(char *name, int argc, char **argv, void *user_data)
{
  printf("Called: %s\n",__FUNCTION__);
}
void h_cgatt(char *name, int argc, char **argv, void *user_data)
{
  printf("CALLED %s\n",__FUNCTION__);
}

void h_nwstateind(char *name, int argc, char **argv, void *user_data)
{
  int i;

  printf("CALLED %s ->",__FUNCTION__);
  for(i =0 ;i < argc; i++){
    printf("%s\n",argv[i]);
  }
  printf("\n");
}



void h_activeratind(char *name, int argc, char **argv, void *user_data)
{
  int i;

  printf("CALLED %s ->",__FUNCTION__);
  for(i =0 ;i < argc; i++){
    printf("%s\n",argv[i]);
  }
  printf("\n");
}



void h_modechangeind(char *name, int argc, char **argv, void *user_data)
{
  int i;

  printf("CALLED %s ->",__FUNCTION__);
  for(i =0 ;i < argc; i++){
    printf("%s\n",argv[i]);
  }
  printf("\n");
}





void modem_at_init()
{
  //  atctx_t *ctx;
  
  mctx = at_init_ctx(myok_cb, NULL,default_cb, default_cmd_cb, NULL);

  at_add_handler(mctx, "ATE1", AT_TYPE_RESPONSE, h_ate1);
  at_add_handler(mctx, "CSCS", AT_TYPE_RESPONSE, h_cscs);
  at_add_handler(mctx, "CMGF", AT_TYPE_RESPONSE, h_cmgf);
  //at_add_handler(mctx, "CHANGEALLPATH", AT_TYPE_RESPONSE, h_changeallpath);
  at_add_handler(mctx, "VERSNAME", AT_TYPE_RESPONSE, h_versname);
  at_add_handler(mctx, "CMEE", AT_TYPE_RESPONSE, h_cmee);
  at_add_handler(mctx, "CGREG", AT_TYPE_RESPONSE, h_cgreg);
  at_add_handler(mctx, "CFUN", AT_TYPE_RESPONSE, h_cfun);
  //at_add_handler(mctx, "CPIN", AT_TYPE_RESPONSE, h_cpin);
  at_add_handler(mctx, "CNUM", AT_TYPE_RESPONSE, h_cnum);
  at_add_handler(mctx, "MODESELECT", AT_TYPE_RESPONSE, h_modeselect);
  at_add_handler(mctx, "CSQ", AT_TYPE_RESPONSE, h_csq);
  at_add_handler(mctx, "COPSNAME", AT_TYPE_RESPONSE, h_copsname);
  at_add_handler(mctx, "CGACT", AT_TYPE_RESPONSE, h_cgact);
  at_add_handler(mctx, "CGDCONT", AT_TYPE_RESPONSE, h_cgdcont);
  at_add_handler(mctx, "CGATT", AT_TYPE_RESPONSE, h_cgatt);
  at_add_handler(mctx, "NWSTATEIND", AT_TYPE_RESPONSE, h_nwstateind);
  at_add_handler(mctx, "MODECHANGEIND", AT_TYPE_RESPONSE, h_modechangeind);
  at_add_handler(mctx, "ACTIVERATIND", AT_TYPE_RESPONSE, h_activeratind);


}
