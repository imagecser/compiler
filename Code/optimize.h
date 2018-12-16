//
// Created by suun on 12/16/18.
//

#ifndef COMPILER_OPTIMIZE_H
#define COMPILER_OPTIMIZE_H

#include "semantic.h"


typedef struct VisitRecord_ *VisitRecord;
typedef struct VisitRecord_ {
  char name[32];
  char equivalent[32];
  int read;
  int write;
}VisitRecord_;

void optimize();

#endif //COMPILER_OPTIMIZE_H
