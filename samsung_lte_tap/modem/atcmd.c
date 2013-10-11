#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atflex.h"
#include "atcmd.h"

#define AT_TYPE_COMMAND  1
#define AT_TYPE_RESPONSE 2
#define AT_TYPE_OK 3
#define AT_TYPE_ERROR 4



atctx_t *at_init_ctx(void *ok_cb, void *error_cb, void *default_response_cb, void *default_command_cb, void *userdata)
{
  atctx_t *ctx;

  ctx = malloc(sizeof(struct atctx_s));
  memset(ctx,0,sizeof(struct atctx_s));

  ctx->ok_cb = ok_cb;
  ctx->error_cb  =  error_cb;
  ctx->default_response_cb  =  default_response_cb;
  ctx->default_command_cb  =  default_command_cb;

  narg=0;
  
  return ctx;
}

void at_add_handler(atctx_t *ctx, char *name, int type, void *cb)
{
  
  struct atcb_s *tmp;

  tmp = malloc(sizeof(struct atcb_s));
  memset(tmp,0,sizeof(struct atcb_s));
  tmp->cb = cb;
  strcpy(tmp->name, name);
  tmp->type = type;
  tmp->next = ctx->list;
  ctx->list = tmp;
}

struct atcb_s *at_find_handler(atctx_t *ctx, char *name, int type)
{
  struct atcb_s *tmp = ctx->list;

  while(tmp){
    if(!strcmp(tmp->name, name)){
      if(tmp->type == type){
	return tmp;
      }
    }
    tmp = tmp->next;
  }

  return NULL;
}

void at_callback(atctx_t *ctx, char *name, int type,  int argc, char **argv)
{
  struct atcb_s *t=NULL;
  
  //printf("Finding cb: |%s|\n",name);
  t = at_find_handler(ctx, name, type);
  if(t){
    if(t->cb){
      t->cb(name, argc, argv, ctx->user_data);
    }
  } else {
    if(type == AT_TYPE_COMMAND){
      if(ctx->default_command_cb){
	ctx->default_command_cb(name, argc, argv, ctx->user_data);
      }
    }
    if(type == AT_TYPE_RESPONSE){
      if(ctx->default_response_cb){
	ctx->default_response_cb(name, argc, argv, ctx->user_data);
      }
    }
  }
}


void on_arg(char *arg)
{
  args[narg] = arg;
  narg++;
}

void on_command(char *cmd)
{
  int i;
  thecmd = cmd;
  strcpy(cur_ctx->last_cmd, cmd);
  cmdtype=AT_TYPE_COMMAND;
}

void on_response(char *cmd)
{
  int i;
  thecmd = cmd;
  cmdtype=AT_TYPE_RESPONSE;
}

void on_endline()
{
  char buf[200];
  
  switch(cmdtype){
  case AT_TYPE_COMMAND:
    at_callback(cur_ctx, thecmd, AT_TYPE_COMMAND, narg, args);
    break;
  case AT_TYPE_RESPONSE:
    at_callback(cur_ctx, thecmd, AT_TYPE_RESPONSE, narg, args);
    break;
  case AT_TYPE_OK:
    if(cur_ctx->ok_cb){
      cur_ctx->ok_cb( cur_ctx, cur_ctx->user_data);
    }
    break;
  case AT_TYPE_ERROR:
    if(cur_ctx->error_cb){
      cur_ctx->error_cb( cur_ctx, cur_ctx->user_data);
    }
    break;
  default:
    printf("what type ????\n");
    break;
  }
  narg=0;
}



void at_parse(atctx_t *ctx, char *buf, int len)
{
  int i;
  char *temp;

  YY_BUFFER_STATE my_string_buffer;


  narg = 0;
  temp = malloc(len + 2);
  cur_ctx = ctx;
  memset(temp,0,len +2);
  strncpy( temp, buf, len );
  temp[len +1] = 0;

  /*  for(i = 0; i < len + 2; i++){
    printf("0x%02x, ",temp[i]);
  }
  printf("\n");*/
  my_string_buffer = yy_scan_string(temp); 
  yy_switch_to_buffer( my_string_buffer ); 
  yyparse(); 
  yy_delete_buffer(my_string_buffer );
  free(temp);

  return;
}


/*
void mycallback(int argc, char **argv, void *user_data)
{
  int i;
  printf("CAlled callback %d\n",argc);
  for(i = 0; i < argc; i++){
    printf("%s\n",argv[i]);
  }
}



void default_cb(int argc, char **argv, void *user_data)
{
  printf("CAlled default CB\n");
}



int main()
{
  char buf[] = {0x41, 0x54, 0x2b, 0x43, 0x47, 0x52, 0x45, 0x47, 0x3d, 0x32, 0x0d, 0x0a, 0x0d, 0x0a, 0x2b, 0x43, 0x47, 0x52, 0x45, 0x47, 0x3a, 0x20, 0x30, 0x2c, 0x30, 0x30, 0x30, 0x30, 0x2c, 0x30, 0x30, 0x30, 0x30, 0x2c, 0x37, 0x2c, 0x30, 0x30, 0x30, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x2b, 0x4e, 0x57, 0x53, 0x54, 0x41, 0x54, 0x45, 0x49, 0x4e, 0x44, 0x3a, 0x20, 0x30, 0x0d, 0x0a, 0x0d, 0x0a, 0x4f, 0x4b, 0x0d, 0x0a, 0 , 0};
  atctx_t *ctx;
  
  printf("STARTING\n%s",buf);

  ctx = at_init_ctx(NULL, NULL,default_cb, default_cb, NULL);
  at_add_handler(ctx, "CGREG", AT_TYPE_RESPONSE, mycallback);
  
  at_parse(ctx, buf, sizeof(buf));
  at_parse(ctx, buf, sizeof(buf));
  at_parse(ctx, buf, sizeof(buf));
  

}


*/
