#ifndef GV1_CODEGEN_H
#define GV1_CODEGEN_H

#include <stdio.h>

#include "ast.h"

int codegen_emit(FILE *out, const ASTNode *root);

#endif
