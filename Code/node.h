
#ifndef COMPILER_NODE_H_
#define COMPILER_NODE_H_

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int err_count;
int yylex();
void yyerror(const char *s);
typedef enum {
  TINT,
  TFLOAT,
  TID,
  TTYPE,  // int or float
  TSYM,   // symbol
  TREL,   // relop
  TDEF,   // definition
  TNUL
} ttype;
typedef struct object_s {
  ttype type;
  int fl;
  int ll;
  struct object_s *child;
  struct object_s *next;
  union {
    int vint;
    float vfloat;
    char vchar;
    char *vstr;
  };
} object;

object *head;
void cus_error(char _type, int _line, const char *s);

void sem_error(int n, int _line, const char *s);

void tree_output(object *head, int tab);

object *init_tnul();
/*
object *merge_node(int num, ...) {
    va_list valist;
    int i;
    object *node = (object *)malloc(sizeof(object));
    va_start(valist, num);
    if (num > 0)
    {
        object *cur = va_arg(valist, object *);
        node->child = cur;
        for (i = 1; i < num; ++i)
        {
            cur->next = va_arg(valist, object *);
            cur = cur->next;
        }
        node->fl = node->child->fl;
    }
    node->next = NULL;
    return node;
}
*/

#define _ac(n)                                                          \
  int i;                                                                \
  object *node = (object *)malloc(sizeof(object));                      \
  node->next = NULL;                                                    \
  if (n > 0) {                                                          \
    node->child = yyvsp[-n + 1].o;                                      \
    for (i = -n + 1; i < 0; i++) {                                      \
      yyvsp[i].o->next = yyvsp[i + 1].o;                                \
    }                                                                   \
    node->fl = node->child->fl;                                         \
    node->ll = yyvsp[0].o->ll;                                          \
    node->type = TDEF;                                                  \
    node->vstr = malloc(sizeof(char) * strlen(yytname[yyr1[yyn]]));     \
    memcpy(node->vstr, yytname[yyr1[yyn]], strlen(yytname[yyr1[yyn]])); \
  } else {                                                              \
    node->child = NULL;                                                 \
    node->type = TNUL;                                                  \
  }                                                                     \
  yyval.o = node;

#define _gerror(n) \
  _ac(n);          \
  cus_error('B', yylsp[-n + 1].first_line, yytname[yyr1[yyn]]);

#define _miss(n, _name)                          \
  _ac(n);                                        \
  char buf[128];                                 \
  sprintf(buf, "Missing character: %s", #_name); \
  cus_error('B', yylsp[0].first_line, buf);

// #define _mn(1) (yyval.o) = merge_node(1, (yyvsp[0].o))
// #define _mn(2) (yyval.o) = merge_node(2, (yyvsp[-1].o), (yyvsp[0].o))
// #define _mn(3) (yyval.o) = merge_node(3, (yyvsp[-2].o), (yyvsp[-1].o),
// (yyvsp[0].o)) #define _mn(4) (yyval.o) = merge_node(4, (yyvsp[-3].o),
// (yyvsp[-2].o), (yyvsp[-1].o), (yyvsp[0].o)) #define _mn(5) (yyval.o) =
// merge_node(5, (yyvsp[-4].o), (yyvsp[-3].o), (yyvsp[-2].o), (yyvsp[-1].o),
// (yyvsp[0].o)) #define _mn(7) (yyval.o) = merge_node(7, (yyvsp[-6].o),
// (yyvsp[-5].o), (yyvsp[-4].o), (yyvsp[-3].o), (yyvsp[-2].o), (yyvsp[-1].o),
// (yyvsp[0].o))

#endif
