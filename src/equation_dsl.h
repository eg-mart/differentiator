#include "math_funcs.h"
#include "tree.h"

struct Node *eq_new_operator(enum MathOp op, struct Node *left,
									struct Node *right, enum EquationError *err);
struct Node *eq_new_number(double num, enum EquationError *err);
struct Node *eq_new_variable(size_t var_ind, enum EquationError *err);

#define new_op(op, l, r)	eq_new_operator((op), (l), (r), err)
#define new_num(num)		eq_new_number((num), err)
#define new_var(var)		eq_new_variable((var), err)
