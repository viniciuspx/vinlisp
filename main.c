// Author: Vinicius Gabriel Santos
// Creation date: 2025-01-03
// Last modification date: 2025-01-04

// Description: Studies conducted on the book "Build Your Own Lisp" by Daniel Holden.
// This is a simple implementation of a Lisp-like language interpreter using the mpc library.
// Link to book: http://www.buildyourownlisp.com/

#include "mpc.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#include <editline/readline.h>
#include <editline/history.h>
#endif


// ENUMS

enum { LVAL_NUM, LVAL_ERR };
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

// lval structuring lval stands for Lisp Value

typedef struct {
  int type;
  long num;
  int err;
} lval;

// lval functions

lval lval_num(long x){
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int x){
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

void lval_print(lval v){
  switch (v.type) {
    case LVAL_NUM: printf("%li", v.num); break;
    case LVAL_ERR:
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division by zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid operator!");
      }
      if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid number!");
      }
      break;
  }
}

void lval_println(lval v){
  lval_print(v);
  putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {

  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) { return 
  y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); }
  if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
  if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num)); }
  if (strcmp(op, "min") == 0) { return lval_num(x.num < y.num ? x.num : y.num); }
  if (strcmp(op, "max") == 0) { return lval_num(x.num > y.num ? x.num : y.num); }
  
  return lval_err(LERR_BAD_OP);
}

lval eval (mpc_ast_t* t) {
  if (strstr(t->tag, "number")){
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
  puts("VinLisp Version 0.0.0.0.1");
  puts("C-c to Exit\n");

  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* VinLisp = mpc_new("vinlisp");
  mpc_result_t r;

  /* Define the grammar for VinLisp */
  /* 
    Grammar for VinLisp (Polish Notation Language):
    - number   : Matches an integer, which can be negative.
    - operator : Matches one of the four basic arithmetic operators: +, -, *, /.
    - expr     : Matches either a number or a parenthesized expression that starts with an operator followed by one or more expressions.
    - vinlisp  : Matches the start of the input, followed by an operator and one or more expressions, and the end of the input.
  */

  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                                \
      number   : /-?[0-9]+/ ;                                        \
      operator : '+' | '-' | '*' | '/' | '^' | '%' | /min/ | /max/ ; \
      expr     : <number> | '(' <operator> <expr>+ ')' ;             \
      vinlisp  : /^/ <operator> <expr>+ /$/ ;                        \
    ",
    Number, Operator, Expr, VinLisp);
  

  while(1){
    char* input= readline("vinlisp> ");
    add_history(input);  

    if (mpc_parse("<stdin>", input, VinLisp, &r)) {
      
      /* Prints the tree */

      // mpc_ast_print(r.output);

      /* Print tree structure of the AST */

      // mpc_ast_t* a = r.output;
      // printf("Tag: %s\n", a->tag);
      // printf("Contents: %s\n", a->contents);
      // printf("Number of children: %i\n", a->children_num);

      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else
    {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    /* Prints raw input after the evaluation */

    // printf("%s\n", input);
    
    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, VinLisp);
  
  return 0;
}
