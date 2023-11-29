#ifndef _EQUATION_UTILS_H
#define _EQUATION_UTILS_H

#include "tree.h"

struct Node *differentiate(struct Node *tr, char varname);
struct Node *simplify(struct Node *node);

#endif /*_EQUATION_UTILS_H*/
