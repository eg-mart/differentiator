#ifndef _MATH_FUNCS_H
#define _MATH_FUNCS_H

#include "tree.h"

enum EquationError {
	EQ_WRONG_ARCCOS_ARG_ERR	= -8,
	EQ_WRONG_ARCSIN_ARG_ERR	= -7,
	EQ_LN_NEGATIVE_ARG_ERR	= -6,
	EQ_NO_VALUES_ERR		= -5,
	EQ_UNKNOWN_OP_ERR		= -4,
	EQ_ZERO_DIV_ERR			= -3,
	EQ_NO_MEM_ERR			= -2,
	EQ_TREE_ERR				= -1,
	EQ_NO_ERR				=  0,
};

typedef struct Node 	  *(*op_diff)	   (const struct Node *equation,
											size_t var, enum EquationError *err);
typedef double 			   (*op_eval)	   (double l, double r,
											enum EquationError *err);
typedef enum EquationError (*op_simplify)  (struct Node *equation);

struct Node		 *math_diff_add	   (const struct Node *equation, size_t var,
							 		enum EquationError *err);
double 		 	  math_eval_add	   (double l, double r,
									enum EquationError *err);
enum EquationError math_simplify_add(struct Node *equation);

struct Node 	 *math_diff_sub	   (const struct Node *equation, size_t var,
							 		enum EquationError *err);
double		 	  math_eval_sub	   (double l, double r,
									enum EquationError *errr);
enum EquationError math_simplify_sub(struct Node *equation);

struct Node		 *math_diff_mult    (const struct Node *equation, size_t var,
					 		 		 enum EquationError *err);
double			  math_eval_mult    (double l, double r,
									 enum EquationError *err);
enum EquationError math_simplify_mult(struct Node *equation);

struct Node		 *math_diff_div	   (const struct Node *equation, size_t var,
							 		enum EquationError *err);
double 			  math_eval_div	   (double l, double r,
									enum EquationError *err);
enum EquationError math_simplify_div(struct Node *equation);

struct Node		 *math_diff_pow	   (const struct Node *equation, size_t var,
									enum EquationError *err);
double			  math_eval_pow	   (double l, double r,
									enum EquationError *err);
enum EquationError math_simplify_pow(struct Node *equation);

struct Node		  *math_diff_ln	    (const struct Node *equation, size_t var,
									 enum EquationError *err);
double			   math_eval_ln	    (double l, double r,
									 enum EquationError *err);
enum EquationError math_simplify_ln (struct Node *equation);

struct Node		  *math_diff_cos    (const struct Node *equation, size_t var,
									 enum EquationError *err);
double			   math_eval_cos    (double l, double r,
									 enum EquationError *err);
enum EquationError math_simplify_cos(struct Node *equation);

struct Node		  *math_diff_sin    (const struct Node *equation, size_t var,
									 enum EquationError *err);
double			   math_eval_sin    (double l, double r,
									 enum EquationError *err);
enum EquationError math_simplify_sin(struct Node *equation);

struct Node		  *math_diff_sqrt    (const struct Node *equation, size_t var,
									 enum EquationError *err);
double			   math_eval_sqrt    (double l, double r,
									 enum EquationError *err);
enum EquationError math_simplify_sqrt(struct Node *equation);

struct Node		  *math_diff_tg    (const struct Node *equation, size_t var,
									 enum EquationError *err);
double			   math_eval_tg    (double l, double r,
									 enum EquationError *err);
enum EquationError math_simplify_tg(struct Node *equation);

struct Node		  *math_diff_ctg    (const struct Node *equation, size_t var,
									 enum EquationError *err);
double			   math_eval_ctg    (double l, double r,
									 enum EquationError *err);
enum EquationError math_simplify_ctg(struct Node *equation);

struct Node		  *math_diff_arcsin    (const struct Node *equation, size_t var,
									 	enum EquationError *err);
double			   math_eval_arcsin    (double l, double r,
									 	enum EquationError *err);
enum EquationError math_simplify_arcsin(struct Node *equation);

struct Node		  *math_diff_arccos    (const struct Node *equation, size_t var,
										enum EquationError *err);
double			   math_eval_arccos    (double l, double r,
									 	enum EquationError *err);
enum EquationError math_simplify_arccos(struct Node *equation);

struct Node		  *math_diff_arctg     (const struct Node *equation, size_t var,
									 	enum EquationError *err);
double			   math_eval_arctg    (double l, double r,
									 	enum EquationError *err);
enum EquationError math_simplify_arctg(struct Node *equation);

struct Node		  *math_diff_arcctg     (const struct Node *equation, size_t var,
									 	enum EquationError *err);
double			   math_eval_arcctg    (double l, double r,
									 	enum EquationError *err);
enum EquationError math_simplify_arcctg(struct Node *equation);

struct MathOpDefinition {
	const char  *name;
	int priority;
	op_diff 	diff;
	op_eval 	eval;
	op_simplify simplify;
};
	
const struct MathOpDefinition MATH_OP_DEFS[] = { 
	{ "+",      3, math_diff_add,    math_eval_add,      math_simplify_add    },
	{ "*",      2, math_diff_mult,   math_eval_mult,     math_simplify_mult   },
	{ "-",      3, math_diff_sub,    math_eval_sub,      math_simplify_sub    },
	{ "/",      2, math_diff_div,    math_eval_div,      math_simplify_div    },
	{ "^",	    1, math_diff_pow,    math_eval_pow,      math_simplify_pow    },
	{ "ln",     1, math_diff_ln,     math_eval_ln,       math_simplify_ln     },
	{ "sqrt",   2, math_diff_sqrt,   math_eval_sqrt,     math_simplify_sqrt   },
	{ "cos",    1, math_diff_cos,    math_eval_cos,      math_simplify_cos    },
	{ "sin",    1, math_diff_sin,    math_eval_sin,      math_simplify_sin    },
	{ "tg",     1, math_diff_tg,     math_eval_tg,       math_simplify_tg     },
	{ "ctg",    1, math_diff_ctg,    math_eval_ctg,      math_simplify_ctg    },
	{ "arcsin", 1, math_diff_arcsin, math_eval_arcsin,   math_simplify_arcsin },
	{ "arccos", 1, math_diff_arccos, math_eval_arccos,   math_simplify_arccos },
	{ "arctg", 	1, math_diff_arctg,  math_eval_arctg,    math_simplify_arctg  },
	{ "arcctg", 1, math_diff_arcctg, math_eval_arcctg,   math_simplify_arcctg },
};
const size_t MATH_OP_DEFS_SIZE = sizeof(MATH_OP_DEFS) / 
								 sizeof(MATH_OP_DEFS[0]);

#endif /*_MATH_FUNCS_H*/