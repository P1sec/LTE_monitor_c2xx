%{
    #include <stdio.h>
    #include "atcmd.h"

%}


%union {
  char *iValue;                 /* integer value */
};



%token <iValue> TXT
%token <iValue> TXTARG
%token <iValue> ATCMD

 //%type <iValue> atcmd
//%type <iValue> writeparam

%token  AT PLUS COMA CAND CHEVRON SEMICOLON QUESTIONMARK EQUAL CR LF SPACE COLON OK ERROR  CMD

%%


line: cmdline | cmdline line;

cmdline: command | retval;
retval: retline OK retline {cmdtype=AT_TYPE_OK; on_endline();} 
| retline ERROR retline  {cmdtype=AT_TYPE_ERROR; on_endline();} 
| retarg retline {on_endline();};
retarg: retline PLUS TXT {on_response($3);}
|retline PLUS TXT COLON arglist { on_response($3);};

command: at atcmd arg retline {on_endline();};
at:  AT {on_command("AT");};
| ATCMD { on_command($1);};

arg: QUESTIONMARK | EQUAL QUESTIONMARK | EQUAL arglist | ;
arglist: argelem | argelem COMA arglist  ;
argelem: TXT { on_arg($1);} 
| TXTARG { on_arg($1);};
atcmd: separator TXT  { on_command($2);}
| ; 
separator:  CAND | PLUS | CHEVRON | ;
retline:   CR LF;

%%



void yyerror(char *s) {
    fprintf(stderr, "erreur: %s\n", s);
}


