#ifndef _DIFFERENTIATOR_H
#define _DIFFERENTIATOR_H

enum MathOp {
	MATH_ADD,
	MATH_MULT,
	MATH_DIV,
	MATH_SUB
};

enum MathTokenType {
	MATH_NUM,
	MATH_OP,
};

union MathTokenValue {
	double num;
	enum MathOp op;
};

struct MathToken {
	enum MathTokenType type;
	union MathTokenValue value;
};

#endif /*_DIFFERENTIATOR_H*/
