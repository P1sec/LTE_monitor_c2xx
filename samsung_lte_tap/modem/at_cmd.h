#ifndef __AT_CMD_H_
#define __AT_CMD_H_




#define AT_TYPE_COMMAND  1
#define AT_TYPE_RESPONSE 2


typedef struct atctx_s {
  char last_cmd[100];
  void (*ok_cb)(struct atctx_s *ctx, void *user_data);
  void (*error_cb)(struct atctx_s *ctx, void *user_data);
  void (*default_response_cb)(char *name, int argc, char **argv, void *user_data);
  void (*default_command_cb)(char *name, int argc, char **argv, void *user_data);
  struct atcb_s *list;
  void *user_data;
}atctx_t;

atctx_t *at_init_ctx(void *ok_cb, void *error_cb, void *default_response_cb, void *default_command_cb, void *userdata);
void at_parse(atctx_t *ctx, char *buf, int len);


#endif //__ATCMD_H_
