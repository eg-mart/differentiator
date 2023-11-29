#include <stddef.h>
#include <assert.h>
#include <math.h>

#include "math_funcs.h"
#include "equation_utils.h"

static struct Node *copy(struct Node *node);

static struct Node *new_op(enum MathOp val, struct Node *left,
						   struct Node *right);
static struct Node *new_num(double val);

static struct Node *l(struct Node *node);
static struct Node *r(struct Node *node);

static enum MathOp op(struct Node *node);
static double num(struct Node *node);
static enum MathTokenType type(struct Node *node);
static char var(struct Node *node);

#define d(node) differentiate((node), varname)
#define c(node) copy(node)

struct Node *differentiate(struct Node *tr, char varname)
{
	if (!tr)
		return NULL;
	
	switch (type(tr)) {
		case MATH_NUM:
			return new_num(0);
		case MATH_VAR:
			if (tr->data.value.varname == varname)
				return new_num(1);
			return new_num(0);
		case MATH_OP:
			switch (op(tr)) {
				case MATH_ADD:
					return new_op(MATH_ADD, d(l(tr)), d(r(tr)));
				case MATH_SUB:
					return new_op(MATH_SUB, d(l(tr)), d(r(tr)));
				case MATH_MULT:
					return new_op(MATH_ADD,
								  new_op(MATH_MULT, d(l(tr)), c(r(tr))),
								  new_op(MATH_MULT, c(l(tr)), d(r(tr))));
				case MATH_DIV:
					return new_op(MATH_DIV,
								  new_op(MATH_SUB,
									  	 new_op(MATH_MULT, d(l(tr)), c(r(tr))),
									  	 new_op(MATH_MULT, c(l(tr)), d(r(tr)))),
								  new_op(MATH_POW, c(r(tr)), new_num(2)));
				case MATH_POW:
					if (type(r(tr)) == MATH_NUM)
						return new_op(MATH_MULT, new_op(MATH_MULT, 
														new_num(num(r(tr))),
									  		  			new_op(MATH_POW, c(l(tr)), 
										  		     		   new_num(num(r(tr)) - 1))),
									  d(l(tr)));
					return c(tr);
									  		 
				default:
					return NULL;
			}
		default:
			return NULL;
	}
}

struct Node *simplify(struct Node *node)
{
	if (!node)
		return NULL;
	if (type(node) == MATH_NUM || type(node) == MATH_VAR)
		return c(node);

	struct Node *left = simplify(l(node));
	struct Node *right = simplify(r(node));
	struct Node *new_node = NULL;

	if (type(node) == MATH_OP && type(left) == MATH_NUM &&
		type(right) == MATH_NUM) {
		switch (op(node)) {
			case MATH_ADD:
				new_node = new_num(num(left) + num(right));
				break;
			case MATH_SUB:
				new_node = new_num(num(left) + num(right));
				break;
			case MATH_MULT:
				new_node = new_num(num(left) * num(right));
				break;
			case MATH_DIV:
				if (num(right) != 0)
					new_node = new_num(num(left) / num(right));
				break;
			case MATH_POW:
				new_node = new_num(powf(num(left), num(right)));
				break;
			default:
				return NULL;
		}
		if (new_node) {
			node_op_delete(left);
			node_op_delete(right);
			return new_node;
		}
	}

	if (type(node) == MATH_OP) {
		switch (op(node)) {
			case MATH_MULT:
				if ((type(left) == MATH_NUM && num(left) == 0) ||
					(type(right) == MATH_NUM && num(right) == 0))
					new_node = new_num(0);
				else if (type(left) == MATH_NUM && num(left) == 1)
					new_node = c(right);
				else if (type(right) == MATH_NUM && num(right) == 1)
					new_node = c(left);
				else if (type(right) == MATH_VAR && type(left) == MATH_VAR &&
						 var(right) == var(left))
					new_node = new_op(MATH_POW, c(left), new_num(2));
				//TODO: add (x*2) * (x*2), (x^2) * (x^2) simplification
				break;
			case MATH_ADD:
				if (type(right) == MATH_NUM && num(right) == 0)
					new_node = c(left);
				else if (type(left) == MATH_NUM && num(left) == 0)
					new_node = c(right);
				else if (type(left) == MATH_VAR && type(right) == MATH_VAR &&
						 var(left) == var(right))
					new_node = new_op(MATH_MULT, new_num(2), c(left));
				//TODO: add (x+2) + (x+2) simplification
				break;
			case MATH_SUB:
				if (type(right) == MATH_NUM && num(right) == 0)
					new_node = c(left);
				else if (type(left) == MATH_NUM && num(left) == 0)
					new_node = new_op(MATH_MULT, new_num(-1), c(right));
				else if (type(left) == MATH_VAR && type(right) == MATH_VAR &&
						 var(left) == var(right))
					new_node = new_num(0);
				break;
			case MATH_DIV:
				if (type(left) == MATH_NUM && num(left) == 0)
					new_node = new_num(0);
				break;
			case MATH_POW:
				if (type(right) == MATH_NUM && num(right) == 1)
					new_node = c(left);
				if (type(left) == MATH_NUM && num(left) == 1)
					new_node = c(right);
				break;
				//TODO: add (x^3)^2 simplification
			default:
				return NULL;
		}
		if (new_node) {
			node_op_delete(left);
			node_op_delete(right);
			return new_node;
		}
	}

	node_op_new(&new_node, node->data);
	new_node->left = left;
	new_node->right = right;
	return new_node;
}

static struct Node *copy(struct Node *node)
{
	if (!node)
		return NULL;
	
	struct Node *new_node = NULL;
	node_op_new(&new_node, node->data);
	new_node->left = c(node->left);
	new_node->right = c(node->right);
	return new_node;
}

static struct Node *new_op(enum MathOp val, struct Node *left,
						   struct Node *right)
{
	struct MathToken tok = {};
	tok.type = MATH_OP;
	tok.value.op = val;
	struct Node *new_node;
	node_op_new(&new_node, tok);
	new_node->left = left;
	new_node->right = right;
	return new_node;
}

static struct Node *new_num(double val)
{
	struct MathToken tok = {};
	tok.type = MATH_NUM;
	tok.value.num = val;
	struct Node *new_node;
	node_op_new(&new_node, tok);
	new_node->left = NULL;
	new_node->right = NULL;
	return new_node;
}

static struct Node *l(struct Node *node)
{
	assert(node);
	return node->left;
}

static struct Node *r(struct Node *node)
{
	assert(node);
	return node->right;
}

static enum MathOp op(struct Node *node)
{
	assert(node);
	return node->data.value.op;
}

static double num(struct Node *node)
{
	assert(node);
	return node->data.value.num;
}

static char var(struct Node *node)
{
	assert(node);
	return node->data.value.varname;
}

static enum MathTokenType type(struct Node *node)
{
	assert(node);
	return node->data.type;
}