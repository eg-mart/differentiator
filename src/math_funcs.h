#ifndef _MATH_FUNCS_H
#define _MATH_FUNCS_H

enum MathOp {
	MATH_ADD,
	MATH_MULT,
	MATH_DIV,
	MATH_SUB,
	MATH_POW,
};

enum MathTokenType {
	MATH_NUM,
	MATH_OP,
	MATH_VAR,
};

#endif /*_MATH_FUNCS_H*/
