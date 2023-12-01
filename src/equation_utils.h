#ifndef _EQUATION_UTILS_H
#define _EQUATION_UTILS_H

#include "tree.h"
#include "math_funcs.h"

struct Equation {
	struct Node *tree;
	char **var_names;
	size_t num_vars;
	size_t cap_vars;
};

const double EQ_EPSILON = 1e-6;
const size_t EQ_INIT_VARS_CAPACITY = 1;
const size_t EQ_DELTA_VARS_CAPACITY = 1;

enum EquationError eq_ctor(struct Equation *eq);
void eq_dtor(struct Equation *eq);

enum EquationError eq_differentiate(struct Equation eq, size_t diff_var_ind,
									struct Equation *diff);
enum EquationError eq_simplify(struct Equation *eq);
enum EquationError eq_evaluate(struct Equation equation, double *vals,
							   double *res);

#endif /*_EQUATION_UTILS_H*/
