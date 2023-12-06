#include <assert.h>
#include <stdlib.h>

#include "equation_manipulation.h"

struct Node *eq_copy(struct Node *equation, enum EquationError *err)
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

struct Node *eq_new_operator(enum MathOp op, struct Node *left,
							 struct Node *right, enum EquationError *err)
{
	assert(err);

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

struct Node *eq_new_number(double num, enum EquationError *err)
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

struct Node *eq_new_variable(size_t var_ind, enum EquationError *err)
{
	assert(err);

	struct MathToken data = {};
	data.type = MATH_VAR;
	data.value.var_ind = var_ind;
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

void eq_change_to_num(struct Node *equation, double num)
{
	assert(equation);

	type(equation) = MATH_NUM;
	num(equation) = num;
	node_op_delete(eq_left);
	node_op_delete(eq_right);
	equation->left = NULL;
	equation->right = NULL;
}

void eq_change_to_op(struct Node *equation, enum MathOp op,
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

void eq_lift_up_left(struct Node *equation)
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

void eq_lift_up_right(struct Node *equation)
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
