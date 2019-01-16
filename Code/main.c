//#include "semantic.h"
#include "optimize.h"
#include "objectcode.h"
#include "syntax.tab.h"

extern void yyrestart(FILE *);

extern int yylineno;

int main(int argc, char **argv) {
  if (argc <= 1)
    return 1;
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror(argv[1]);
    return 1;
  }
  err_count = 0;
  yylineno = 1;
  head = NULL;

  yyrestart(f);
  yyparse();
  if (err_count == 0) {
    // tree_output(head, 0);
    initHashTable();
    initInterCodeList();
    goExtDefList(head->child);
    optimize();
    if (argc == 2) {
//      writeInterFile("stdout");
      writeObjectFile("stdout");
    } else if (argc == 3) {
//      writeInterFile(argv[2]);
      writeObjectFile(argv[2]);
    }
    // traverseTree(head);
  }
  return 0;
}
