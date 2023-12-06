#include "math_funcs.h"
#include "tree.h"

struct Node *eq_new_operator(enum MathOp op, struct Node *left,
							 struct Node *right, enum EquationError *err);
struct Node *eq_new_number(double num, enum EquationError *err);
struct Node *eq_new_variable(size_t var_ind, enum EquationError *err);
struct Node *eq_copy(struct Node *equation, enum EquationError *err);
void eq_change_to_num(struct Node *equation, double num);
void eq_change_to_op(struct Node *equation, enum MathOp op,
					 struct Node *left, struct Node *right);
void eq_lift_up_left(struct Node *equation);
void eq_lift_up_right(struct Node *equation);

#define new_op(op, l, r)	eq_new_operator((op), (l), (r), err)
#define new_num(num)		eq_new_number((num), err)
#define new_var(var)		eq_new_variable((var), err)
#define copy(eq) 			eq_copy((eq), err)
#define to_num(eq, num) 	eq_change_to_num((eq), (num))
#define to_op(eq, op, l, r) eq_change_to_op((eq), (op), (l), (r))
#define move(dest, src)	 	eq_move((dest), (src))
#define eq_left	 		 	equation->left
#define eq_right 		 	equation->right
#define type(eq)		 	(eq)->data.type
#define num(eq)			 	(eq)->data.value.num
#define var(eq)			 	(eq)->data.value.var_ind
#define op(eq)			 	(eq)->data.value.op

