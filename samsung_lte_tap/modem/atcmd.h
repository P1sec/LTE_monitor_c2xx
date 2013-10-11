#ifndef __ATCMD_H_
#define __ATCMD_H_




#define AT_TYPE_COMMAND  1
#define AT_TYPE_RESPONSE 2
#define AT_TYPE_OK 3
#define AT_TYPE_ERROR 4


char *args[100];
char *thecmd;
int narg;
int cmdtype;

struct atcb_s {
  char name[100];
  int type;
  void (*cb)(char *name, int argc, char **argv, void *user_data);
  struct atcb_s *next;
};

typedef struct atctx_s {
  char last_cmd[100];
  void (*ok_cb)(struct atctx_s *ctx, void *user_data);
  void (*error_cb)(struct atctx_s *ctx, void *user_data);
  void (*default_response_cb)(char *name, int argc, char **argv, void *user_data);
  void (*default_command_cb)(char *name, int argc, char **argv, void *user_data);
  struct atcb_s *list;
  void *user_data;
}atctx_t;


atctx_t *cur_ctx;
int yylex(void);
void yyerror(char *);
void on_arg(char *arg);
void on_command(char *cmd);
void on_response(char *cmd);
void on_endline();





#endif //__ATCMD_H_
