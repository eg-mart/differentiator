#ifndef _EQUATION_UTILS_H
#define _EQUATION_UTILS_H

#include "tree.h"
#include "math_funcs.h"

const double EQ_EPSILON = 1e-6;

struct Node *eq_differentiate(struct Node *equation, char diff_var_name,
							  enum EquationError *err);
enum EquationError eq_simplify(struct Node *equation);

#endif /*_EQUATION_UTILS_H*/
