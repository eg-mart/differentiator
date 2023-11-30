#ifndef _TREE_H
#define _TREE_H

enum MathOp {
	MATH_ADD,
	MATH_MULT,
	MATH_SUB,
	MATH_DIV,
};

enum MathTokenType {
	MATH_NUM,
	MATH_OP,
	MATH_VAR,
};

union MathTokenValue {
	double num;
	enum MathOp op;
	char varname;
};

struct MathToken {
	enum MathTokenType type;
	union MathTokenValue value;
};

typedef MathToken elem_t;

struct Node {
	elem_t data;
	struct Node *left;
	struct Node *right;
};

enum TreeError {
	TREE_NO_MEM_ERR = -1,
	TREE_NO_ERR 	= 0,
};

enum TreeError node_op_new(struct Node **node, elem_t data);
void node_ctor(struct Node *node, elem_t data);
void node_op_delete(struct Node *node);
const char *tree_err_to_str(enum TreeError err);

#endif /*_TREE_H*/