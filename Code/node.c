#include "node.h"

void cus_error(char _type, int _line, const char *s) {
  char err[100];
  sprintf(err, "Error type %c at Line %d: %s", _type, _line, s);
  yyerror(err);
}

void sem_error(int n, int _line, const char *s) {
  char err[100];
  sprintf(err, "Error type %d at Line %d: %s", n, _line, s);
  yyerror(err);
}

void tree_output(object *head, int tab) {
  int i;
  if (head == NULL) return;
  if (head->type != TNUL)
    for (i = 0; i < tab * 2; i++) printf(" ");
  switch (head->type) {
    case TINT:
      printf("INT: %d\n", head->vint);
      break;
    case TFLOAT:
      printf("FLOAT: %f\n", head->vfloat);
      break;
    case TID:
      printf("ID: %s\n", head->vstr);
      break;
    case TTYPE:
      printf("TYPE: %s\n", head->vstr);
      break;
    case TSYM:
      printf("%s\n", head->vstr);
      break;
    case TREL:
      printf("%s\n", head->vstr);
      break;
    case TDEF:
      printf("%s (%d)\n", head->vstr, head->fl);
      tree_output(head->child, tab + 1);
      break;
    case TNUL:
      tree_output(head->child, tab + 1);
      break;
  }
  tree_output(head->next, tab);
}

object *init_tnul() {
  object *node = malloc(sizeof(object));
  node->type = TNUL;
  node->child = node->next = NULL;
  node->fl = node->vint = 0;
  return node;
}