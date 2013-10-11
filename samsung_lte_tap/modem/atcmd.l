%{
 #include "y.tab.h"
 #include <stdlib.h>
 int flexmark = 0;
%}
%option header-file="atflex.h"


%x ESC
%%

AT                  return AT;
AT[A-Za-z0-9]*      {yylval.iValue = strdup(yytext);  return ATCMD;};
OK                 return OK;
ERROR              return ERROR;
[+]                return PLUS;
[&]                return CAND;
[,]                return COMA;
[;]                return SEMICOLON;
[:]                return COLON;
[\^]               return CHEVRON;
[?]                return QUESTIONMARK;
[=]                return EQUAL;
[\r]               return CR; 
[\n]               return LF; 
"\""               { flexmark = 0; BEGIN(ESC);};
[ ]
[A-Za-z0-9.][ A-Za-z0-9.]*       {yylval.iValue = strdup(yytext);  return TXT;};
.



<ESC>"\""           {BEGIN(INITIAL); if(!flexmark) { yylval.iValue=NULL; return TXTARG;} }; 
<ESC>[ A-Za-z0-9.,*]*   {flexmark = 1; yylval.iValue = strdup(yytext); return TXTARG;};
%%

int yywrap(void) {
    return 1;
}
