#ifndef _MATH_FUNCS_H
#define _MATH_FUNCS_H

#include "tree.h"

enum EquationError {
	EQ_UNKNOWN_OP_ERR	= -4,
	EQ_ZERO_DIV_ERR		= -3,
	EQ_NO_MEM_ERR		= -2,
	EQ_TREE_ERR			= -1,
	EQ_NO_ERR			=  0,
};

typedef struct Node 	  *(*op_diff)	   (struct Node *equation, char var,
										    enum EquationError *err);
typedef double 			   (*op_eval)	   (struct Node *equation,
										    enum EquationError *err);
typedef enum EquationError (*op_simplify)  (struct Node *equation);

struct Node		 *math_diff_add	   (struct Node *equation, char var,
									 		enum EquationError *err);
double 		 	  math_eval_add	   (struct Node *equation,
											enum EquationError *err);
enum EquationError math_simplify_add(struct Node *equation);

struct Node 	  	 *math_diff_sub	   (struct Node *equation, char var,
									 		enum EquationError *err);
double		 	  math_eval_sub	   (struct Node *equation,
											enum EquationError *errr);
enum EquationError math_simplify_sub(struct Node *equation);

struct Node		 *math_diff_mult    (struct Node *equation, char var,
							 		 		 enum EquationError *err);
double			  math_eval_mult    (struct Node *equation,
											 enum EquationError *err);
enum EquationError math_simplify_mult(struct Node *equation);

struct Node		 *math_diff_div	   (struct Node *equation, char var,
									 		enum EquationError *err);
double 			  math_eval_div	   (struct Node *equation,
											enum EquationError *err);
enum EquationError math_simplify_div(struct Node *equation);

struct MathOpDefinition {
	const char  *name;
	int priority;
	op_diff 	diff;
	op_eval 	eval;
	op_simplify simplify;
};
	
const struct MathOpDefinition MATH_OP_DEFS[] = {
	{ "+", 2, math_diff_add,  math_eval_add,  math_simplify_add  },
	{ "*", 1, math_diff_mult, math_eval_mult, math_simplify_mult },
	{ "-", 2, math_diff_sub,  math_eval_sub,  math_simplify_sub  },
	{ "/", 1, math_diff_div,  math_eval_div,  math_simplify_div  },
};
const size_t MATH_OP_DEFS_SIZE = sizeof(MATH_OP_DEFS) / 
								 sizeof(MATH_OP_DEFS[0]);

#endif /*_MATH_FUNCS_H*/