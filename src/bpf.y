%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bpf.h"

int yyerror(const char*);
int yylex();

extern void* yy_scan_string(const char *);
extern int yylex_destroy (void);

struct bpf_node* newnode();
void setparent(struct bpf_node*, struct bpf_node*);
void mergenext(struct bpf_node*, struct bpf_node*);
static struct bpf_node *root;

%}
%union {
    unsigned char ip[5];
    unsigned char mac[6];
    unsigned int val;
    struct bpf_node *node;
}

%define lr.type lalr

%token <ip> IPval
%token <mac> MACval
%token <val> INT
%token HOST PR PORT SRC DST PROTO NET
%token TCP UDP IP
%token AND OR NOT

%left OR
%left AND
%right NOT

%type<node> BasicExp PROTO Expr INTlist IPlist
%type<val> Dir Proto NEDir

%start S

%%

S: Expr {root = $1;}
| {root = NULL;}
;

Expr:
  BasicExp
| Expr OR Expr {
    if($1->op == OP_OR)
    {
        if($3->op == OP_OR)
        {
            mergenext($1->child, $3->child);
            setparent($1, $1->child);
            free($3);
        }
        else
        {
            $$ = $1;
            mergenext($3, $1->child);
            $1->child = $3;
            setparent($1, $3);
        }
    }
    else if($3->op == OP_OR)
    {
        $$ = $3;
        mergenext($1, $3->child);
        $3->child = $1;
        setparent($3, $1);
    }
    else
    {
        $$ = newnode();
        $$->op = OP_OR;
        $$->child = $1;
        mergenext($1, $3);
        setparent($$, $1);
    }
}
| Expr AND Expr {
    if($1->op == OP_AND)
    {
        if($3->op == OP_AND)
        {
            mergenext($1->child, $3->child);
            setparent($1, $1->child);
            free($3);
        }
        else
        {
            $$ = $1;
            mergenext($1->child, $3);
            setparent($1, $3);
        }
    }
    else if($3->op == OP_AND)
    {
        $$ = $3;
        mergenext($1, $3->child);
        $3->child = $1;
        setparent($3, $1);
    }
    else
    {
        $$ = newnode();
        $$->op = OP_AND;
        $$->child = $1;
        mergenext($1, $3);
        setparent($$, $1);
    }
}
| NOT Expr {
    if($2->op == OP_NOT)
    {
        $$ = $2->child;
        free($2);
        setparent(NULL, $$);
    }
    else
    {
        $$ = newnode();
        $$->child = $2;
        setparent($$, $2);
        $$->op = OP_NOT;
    }
}
| '(' Expr ')' {$$ = $2;}
;

BasicExp:
  Proto {
      $$ = newnode();
      $$->op = OP_PROTOCOL;
      $$->param.proto = $1;
}
| Dir HOST IPlist {
    struct bpf_node *ptr = $3;
    while(ptr)
    {
        ptr->op = OP_HOST;
        ptr->param.dir = $1;
        ptr = ptr->next;
    }
    $$ = newnode();
    $$->op = OP_OR;
    $$->child = $3;
    setparent($$, $3);
}
| NEDir IPlist {
    struct bpf_node *ptr = $2;
    while(ptr)
    {
        ptr->op = OP_HOST;
        ptr->param.dir = $1;
        ptr = ptr->next;
    }
    $$ = newnode();
    $$->op = OP_OR;
    $$->child = $2;
    setparent($$, $2);
}
| Dir NET IPlist {
    struct bpf_node *ptr = $3;
    while(ptr)
    {
        ptr->op = OP_NET;
        ptr->param.dir = $1;
        ptr = ptr->next;
    }
    $$ = newnode();
    $$->op = OP_OR;
    $$->child = $3;
    setparent($$, $3);
}
| Proto Dir PORT INTlist {
    struct bpf_node *ptr = $4;
    while(ptr)
    {
        ptr->op = OP_PORT;
        ptr->param.bound[0] = ptr->param.bound[1] = ptr->param.val;
        ptr->param.proto = $1;
        ptr->param.dir = $2;
        ptr = ptr->next;
    }
    $$ = newnode();
    $$->op = OP_OR;
    $$->child = $4;
    setparent($$, $4);
}
| Proto Dir PR INT '-' INT {
    $$ = newnode();
    $$->op = OP_PORT;
    $$->param.bound[0] = $4;
    $$->param.bound[1] = $6;
    $$->param.proto = $1;
    $$->param.dir = $2;
}
| Dir PORT INTlist {
    struct bpf_node *ptr = $3;
    while(ptr)
    {
        ptr->op = OP_PORT;
        ptr->param.bound[0] = ptr->param.bound[1] = ptr->param.val;
        ptr->param.proto = 0;
        ptr->param.dir = $1;
        ptr = ptr->next;
    }
    $$ = newnode();
    $$->op = OP_OR;
    $$->child = $3;
    setparent($$, $3);
}
| Dir PR INT '-' INT {
    $$ = newnode();
    $$->op = OP_PORT;
    $$->param.bound[0] = $3;
    $$->param.bound[1] = $5;
    $$->param.proto = 0;
    $$->param.dir = $1;
}
;

INTlist:
  INT {
    $$ = newnode();
    $$->param.val = $1;
}
| INT ',' INTlist {
    $$ = newnode();
    $$->param.val = $1;
    $$->next = $3;
}
;

IPlist:
  IPval {
    $$ = newnode();
    memcpy($$->param.ip, $1, 5 * sizeof(uchar));
}
| IPval ',' IPlist {
    $$ = newnode();
    memcpy($$->param.ip, $1, 5 * sizeof(uchar));
    $$->next = $3;
}
;

Proto:
  TCP {$$ = IPPROTO_TCP;}
| UDP {$$ = IPPROTO_UDP;}
| IP {$$ = IPPROTO_IP;}
;

NEDir:
  SRC {$$ = 1;}
| DST {$$ = 2;}
| SRC AND DST {$$ = 3;}
| SRC OR DST {$$ = 0;}
;

Dir:
  NEDir
| {$$ = 0;}
;

%%
int yyerror(const char *msg)
{
	fprintf(stderr ,"error: %s\n", msg);
	return 1;
}

struct bpf_node* newnode()
{
    struct bpf_node *node = (struct bpf_node*)malloc(sizeof(struct bpf_node));
    memset(node, 0, sizeof(struct bpf_node));
    return node;
}

void setparent(struct bpf_node* node, struct bpf_node* child)
{
    struct bpf_node *ptr = child;
    while(ptr)
    {
        ptr->parent = node;
        ptr = ptr->next;
    }
}

void mergenext(struct bpf_node* a, struct bpf_node* b)
{
    struct bpf_node *ptr = a;
    while(ptr->next) ptr = ptr->next;
    ptr->next = b;
}


struct bpf_node* bpf_compile(const char* filter)
{
    yy_scan_string(filter);
    yyparse();
    yylex_destroy();
    return root;
}

void free_bpf(struct bpf_node* root)
{
    if(root->child) free_bpf(root->child);
    if(root->next) free_bpf(root->next);
    free(root);
}
