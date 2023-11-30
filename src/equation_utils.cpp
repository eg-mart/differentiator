#include <stddef.h>
#include <assert.h>
#include <math.h>

#include "equation_utils.h"
#include "logger.h"

static struct Node *eq_copy(struct Node *equation, enum EquationError *err);
static struct Node *eq_new_operator(enum MathOp op, struct Node *left,
									struct Node *right, enum EquationError *err);
static struct Node *eq_new_number(double num, enum EquationError *err);
static struct Node *eq_new_variable(char varname, enum EquationError *err);
static void eq_change_to_num(struct Node *equation, double num);
static void eq_change_to_op(struct Node *equation, enum MathOp op,
							struct Node *left, struct Node *right);
static bool is_equal(double a, double b);

#define diff(eq)			eq_differentiate((eq), var, err)
#define copy(eq) 			eq_copy((eq), err)
#define new_op(op, l, r)	eq_new_operator((op), (l), (r), err)
#define new_num(num)		eq_new_number((num), err)
#define new_var(var)		eq_new_variable((var), err)
#define to_num(eq, num) 	eq_change_to_num((eq), (num))
#define to_op(eq, op, l, r) eq_change_to_op((eq), (op), (l), (r))
#define move(dest, src)	 	eq_move((dest), (src))
#define eq_left	 		 	equation->left
#define eq_right 		 	equation->right
#define type(eq)		 	(eq)->data.type
#define num(eq)			 	(eq)->data.value.num
#define var(eq)			 	(eq)->data.value.varname
#define op(eq)			 	(eq)->data.value.op

struct Node *eq_differentiate(struct Node *equation, char diff_var_name,
							  enum EquationError *err)
{
	switch (type(equation)) {
		case MATH_NUM:
			return new_num(0);
		case MATH_VAR:
			if (var(equation) == diff_var_name)
				return new_num(1);
			else
				return new_var(var(equation));
		case MATH_OP:
			return (*MATH_OP_DEFS[op(equation)].diff)(equation, diff_var_name,
													  err);
		default:
			*err = EQ_UNKNOWN_OP_ERR;
			return NULL;
	}
}

enum EquationError eq_simplify(struct Node *equation)
{
	assert(equation);

	if (type(equation) == MATH_NUM || type(equation) == MATH_VAR)
		return EQ_NO_ERR;

	eq_simplify(equation->left);
	eq_simplify(equation->right);

	enum EquationError err = EQ_NO_ERR;

	if (type(equation) == MATH_OP && type(eq_left) == MATH_NUM &&
		type(eq_right) == MATH_NUM) {
		double eval_res = (*MATH_OP_DEFS[op(equation)].eval)(equation, &err);
		if (err < 0)
			return err;
		to_num(equation, eval_res);
		return EQ_NO_ERR;
	} else if (type(equation) == MATH_OP) {
		return (*MATH_OP_DEFS[op(equation)].simplify)(equation);
	}

	return EQ_NO_ERR;
}

static struct Node *eq_copy(struct Node *equation, enum EquationError *err)
{
	assert(err);

	if (!equation)
		return NULL;
	
	struct MathToken data = equation->data;
	struct Node *new_node = NULL;
	enum TreeError tr_err = node_op_new(&new_node, data);

	if (tr_err == TREE_NO_MEM_ERR) {
		*err = EQ_NO_MEM_ERR;
		return NULL;
	} else if (tr_err < 0) {
		*err = EQ_TREE_ERR;
		return NULL;
	}

	new_node->left = copy(equation->left);
	new_node->right = copy(equation->right);

	return new_node;
}

static struct Node *eq_new_operator(enum MathOp op, struct Node *left,
									struct Node *right, enum EquationError *err)
{
	assert(err);

	if (!left || !right)
		return NULL;

	struct MathToken data = {};
	data.type = MATH_OP;
	data.value.op = op;
	struct Node *new_node = NULL;
	enum TreeError tr_err = node_op_new(&new_node, data);

	if (tr_err == TREE_NO_MEM_ERR) {
		*err = EQ_NO_MEM_ERR;
		node_op_delete(left);
		node_op_delete(right);
		return NULL;
	} else if (tr_err < 0) {
		*err = EQ_TREE_ERR;
		node_op_delete(left);
		node_op_delete(right);
		return NULL;
	}

	new_node->left = left;
	new_node->right = right;

	return new_node;
}

static struct Node *eq_new_number(double num, enum EquationError *err)
{
	assert(err);

	struct MathToken data = {};
	data.type = MATH_NUM;
	data.value.num = num;
	struct Node *new_node = NULL;
	enum TreeError tr_err = node_op_new(&new_node, data);

	if (tr_err == TREE_NO_MEM_ERR) {
		*err = EQ_NO_MEM_ERR;
		return NULL;
	} else if (tr_err < 0) {
		*err = EQ_TREE_ERR;
		return NULL;
	}

	new_node->left = NULL;
	new_node->right = NULL;

	return new_node;
}

static struct Node *eq_new_variable(char varname, enum EquationError *err)
{
	assert(err);

	struct MathToken data = {};
	data.type = MATH_VAR;
	data.value.varname = varname;
	struct Node *new_node = NULL;
	enum TreeError tr_err = node_op_new(&new_node, data);

	if (tr_err == TREE_NO_MEM_ERR) {
		*err = EQ_NO_MEM_ERR;
		return NULL;
	} else if (tr_err < 0) {
		*err = EQ_TREE_ERR;
		return NULL;
	}

	new_node->left = NULL;
	new_node->right = NULL;

	return new_node;
}

static void eq_change_to_num(struct Node *equation, double num)
{
	assert(equation);

	type(equation) = MATH_NUM;
	num(equation) = num;
	node_op_delete(eq_left);
	node_op_delete(eq_right);
	eq_left = NULL;
	eq_right = NULL;
}

static void eq_change_to_op(struct Node *equation, enum MathOp op,
							struct Node *left, struct Node *right)
{
	assert(equation);
	assert(left);
	assert(right);
	
	type(equation) = MATH_OP;
	op(equation) = op;
	node_op_delete(equation->left);
	node_op_delete(equation->right);
	equation->left = left;
	equation->right = right;
}

static void eq_lift_up_left(struct Node *equation)
{
	assert(equation);
	assert(equation->left);

	struct Node *left = equation->left;
	node_op_delete(equation->right);
	equation->data = left->data;
	equation->right = left->right;
	equation->left = left->left;
	free(left);
}

static void eq_lift_up_right(struct Node *equation)
{
	assert(equation);
	assert(equation->right);

	struct Node *right = equation->right;
	node_op_delete(equation->left);
	equation->data = right->data;
	equation->right = right->right;
	equation->left = right->left;
	free(right);
}

static bool is_equal(double a, double b)
{
	return abs(a - b) < EQ_EPSILON;
}

struct Node *math_diff_add(struct Node *equation, char var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ADD);
	
	return new_op(MATH_ADD, diff(eq_left), diff(eq_right));
}
									 	
double math_eval_add(struct Node *equation, enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ADD);
	assert(type(eq_left) == MATH_NUM);
	assert(type(eq_right) == MATH_NUM);

	return num(eq_left) + num(eq_right);
}

enum EquationError math_simplify_add(struct Node *equation)
{
	assert(equation);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ADD);

	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;
	if (type(eq_left) == MATH_NUM) {
		if (is_equal(num(eq_left), 0))
			eq_lift_up_right(equation);
	} else if (type(eq_right) == MATH_NUM) {
		if (is_equal(num(eq_right), 0))
			eq_lift_up_left(equation);
	} else if (type(eq_right) == MATH_VAR && type(eq_left) == MATH_VAR &&
			   var(eq_right) == var(eq_left)) {
		to_op(equation, MATH_MULT, new_num(2), copy(eq_left));
	}
	return eq_err;
}

struct Node *math_diff_sub(struct Node *equation, char var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_SUB);

	return new_op(MATH_SUB, diff(eq_left), diff(eq_right));
}

double math_eval_sub(struct Node *equation, enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_SUB);
	assert(type(eq_left) == MATH_NUM);
	assert(type(eq_right) == MATH_NUM);

	return num(eq_left) - num(eq_right);
}

enum EquationError math_simplify_sub(struct Node *equation)
{
	assert(equation);

	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;
	if (type(eq_left) == MATH_NUM) {
		if (is_equal(num(eq_left), 0))
			to_op(equation, MATH_MULT, new_num(-1), copy(eq_right));
	} else if (type(eq_right) == MATH_NUM) {
		if (is_equal(num(eq_right), 0))
			eq_lift_up_left(equation);
	} else if (type(eq_right) == MATH_VAR && type(eq_left) == MATH_VAR &&
			   var(eq_right) == var(eq_left)) {
		to_num(equation, 0);
	}
	return eq_err;
}

struct Node *math_diff_mult(struct Node *equation, char var,
							enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_MULT);

	return new_op(MATH_ADD,
				  new_op(MATH_MULT, diff(eq_left), copy(eq_right)),
				  new_op(MATH_MULT, copy(eq_left), diff(eq_right)));
}

double math_eval_mult(struct Node *equation, enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_MULT);
	assert(type(eq_left) == MATH_NUM);
	assert(type(eq_right) == MATH_NUM);

	return num(eq_left) * num(eq_right);
}

enum EquationError math_simplify_mult(struct Node *equation)
{
	assert(equation);

	if (type(eq_left) == MATH_NUM) {
		if (is_equal(num(eq_left), 0))
			to_num(equation, 0);
		else if (is_equal(num(eq_left), 1))
			eq_lift_up_right(equation);
	} else if (type(eq_right) == MATH_NUM) {
		if (is_equal(num(eq_right), 0))
			to_num(equation, 0);
		else if (is_equal(num(eq_right), 1))
			eq_lift_up_left(equation);
	}

	return EQ_NO_ERR;
}

struct Node *math_diff_div(struct Node *equation, char var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_DIV);

	return new_op(MATH_DIV,
				  new_op(MATH_SUB,
				  		 new_op(MATH_MULT, diff(eq_left), copy(eq_right)),
				  		 new_op(MATH_MULT, copy(eq_left), diff(eq_right))),
				  new_op(MATH_MULT, copy(eq_right), copy(eq_right)));
}

double math_eval_div(struct Node *equation, enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_DIV);
	assert(type(eq_left) == MATH_NUM);
	assert(type(eq_right) == MATH_NUM);

	if (is_equal(num(eq_right), 0)) {
		*err = EQ_ZERO_DIV_ERR;
		return NAN;
	}

	return num(eq_left) / num(eq_right);
}

enum EquationError math_simplify_div(struct Node *equation)
{
	assert(equation);

	if (type(eq_left) == MATH_NUM) {
		if (is_equal(num(eq_left), 0))
			to_num(equation, 0);
	} else if (type(eq_right) == MATH_NUM) {
		if (is_equal(num(eq_right), 0))
			return EQ_ZERO_DIV_ERR;
		else if (is_equal(num(eq_right), 1))
			eq_lift_up_left(equation);
	} else if (type(eq_right) == MATH_VAR && type(eq_left) == MATH_VAR &&
			   var(eq_left) == var(eq_right)) {
		to_num(equation, 1);
	}
	return EQ_NO_ERR;
}