%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bpf_gram.h"
#include <netinet/ip.h>
extern YYSTYPE yylval;
int yyerror(const char*);
int yywrap();
%}

delim [ \t\n]
ws {delim}+
brace [-,()]
digit [0-9]
hex [0-9a-fA-F]

integer {digit}+

ipele {digit}{1,3}
ip ({ipele}\.){0,3}{ipele}(\/{digit}+)?

macele {hex}{2}
mac ({macele}:){5}{macele}

%%

{ws} {}

{brace} {return *yytext;}

{integer} {
    sscanf(yytext, "%u", &yylval.val);
    return INT;
}

{ip} {
    int i = 0, ptr = 0;
    int len = strlen(yytext);
    unsigned char tmp;
    memset(&yylval.ip[0], 0, 5 * sizeof(char));
    while(i < len)
    {
        sscanf(yytext + i, "%hhd", &yylval.ip[ptr]);
        ptr++;
        while(yytext[i] && yytext[i] != '.' && yytext[i] != '/')
            i++;
        if(yytext[i] == '/') ptr = 4;
        else yylval.ip[4] += 8;
        i++;
    }
    return IPval;
}

{mac} {
    sscanf(yytext, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
    &yylval.mac[0], &yylval.mac[1], &yylval.mac[2],
    &yylval.mac[3], &yylval.mac[4], &yylval.mac[5]);
    return MACval;
}

and {return AND;}
or {return OR;}
not {return NOT;}

"&&" {return AND;}
"||" {return OR;}
"!" {return NOT;}

host {return HOST;}
portrange {return PR;}
port {return PORT;}
proto {return PROTO;}
net {return NET;}

src {return SRC;}
dst {return DST;}

tcp {return TCP;}
udp {return UDP;}
ip {return IP;}

. {yyerror("unsupported");}

%%

int yywrap()
{
	return 1;
}