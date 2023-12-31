#include <stddef.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "equation_utils.h"
#include "equation_manipulation.h"
#include "equation_io.h"
#include "logger.h"

static struct Node *subeq_differentiate(const struct Node *equation, 
										size_t diff_var_ind,
										enum EquationError *err);
static enum EquationError subeq_simplify(struct Node *equation);
static enum EquationError subeq_evaluate(struct Node *subeq, double *vals,
										 double *res);

static bool is_equal(double a, double b);

#define diff(eq)			subeq_differentiate((eq), var, err)

enum EquationError eq_ctor(struct Equation *eq)
{
	eq->var_names = (char**) calloc(EQ_INIT_VARS_CAPACITY, sizeof(char*));
	if (!eq->var_names)
		return EQ_NO_MEM_ERR;
	eq->num_vars = 0;
	eq->cap_vars = EQ_INIT_VARS_CAPACITY;
	eq->tree = NULL;
	return EQ_NO_ERR;
}

void eq_dtor(struct Equation *eq)
{
	node_op_delete(eq->tree);
	eq->tree = NULL;
	for (size_t i = 0; i < eq->num_vars; i++)
		free(eq->var_names[i]);
	free(eq->var_names);
	eq->var_names = NULL;
	eq->cap_vars = (size_t) -1;
	eq->num_vars = (size_t) -1;
}

enum EquationError eq_differentiate(struct Equation eq, size_t diff_var_ind,
									struct Equation *diff)
{
	assert(diff);

	char **tmp = (char**) realloc(diff->var_names, eq.cap_vars * sizeof(char*));
	if (!tmp)
		return EQ_NO_MEM_ERR;
	diff->var_names = tmp;
	for (size_t i = 0; i < eq.num_vars; i++)
		diff->var_names[i] = strdup(eq.var_names[i]);
	diff->num_vars = eq.num_vars;
	diff->cap_vars = eq.cap_vars;

	enum EquationError err = EQ_NO_ERR;
	diff->tree = subeq_differentiate(eq.tree, diff_var_ind, &err);

	return err;
}

static struct Node *subeq_differentiate(const struct Node *equation, 
										size_t diff_var_ind,
										enum EquationError *err)
{
	switch (type(equation)) {
		case MATH_NUM:
			return new_num(0);
		case MATH_VAR:
			if (var(equation) == diff_var_ind)
				return new_num(1);
			else
				return new_num(0);
		case MATH_OP:
			return (*MATH_OP_DEFS[op(equation)].diff)(equation, diff_var_ind,
													  err);
		default:
			*err = EQ_UNKNOWN_OP_ERR;
			return NULL;
	}
}

enum EquationError eq_simplify(struct Equation *eq)
{
	return subeq_simplify(eq->tree);
}

static enum EquationError subeq_simplify(struct Node *equation)
{
	if (!equation)
		return EQ_NO_ERR;

	if (type(equation) == MATH_NUM || type(equation) == MATH_VAR)
		return EQ_NO_ERR;

	enum EquationError err = EQ_NO_ERR;

	err = subeq_simplify(equation->left);
	if (err < 0)
		return err;
	err = subeq_simplify(equation->right);
	if (err < 0)
		return err;

	if (type(equation) == MATH_OP && !eq_left &&
		type(eq_right) == MATH_NUM) {
		double eval_res = (*MATH_OP_DEFS[op(equation)].eval)(NAN,
															 num(eq_right),
															 &err);
		if (err < 0)
			return err;
		to_num(equation, eval_res);
		return EQ_NO_ERR;
	}
	if (type(equation) == MATH_OP && !eq_right &&
		type(eq_left) == MATH_NUM) {
		double eval_res = (*MATH_OP_DEFS[op(equation)].eval)(num(eq_left),
															 NAN,
															 &err);
		if (err < 0)
			return err;
		to_num(equation, eval_res);
		return EQ_NO_ERR;
	}
	if (eq_left && eq_right && type(equation) == MATH_OP &&
		type(eq_left) == MATH_NUM && type(eq_right) == MATH_NUM) {
		double eval_res = (*MATH_OP_DEFS[op(equation)].eval)(num(eq_left),
															 num(eq_right),
															 &err);
		if (err < 0)
			return err;
		to_num(equation, eval_res);
		return EQ_NO_ERR;
	}
	if (type(equation) == MATH_OP) {
		return (*MATH_OP_DEFS[op(equation)].simplify)(equation);
	}

	return EQ_NO_ERR;
}

enum EquationError eq_evaluate(struct Equation equation, double *vals,
							   double *res)
{
	return subeq_evaluate(equation.tree, vals, res);
}

static enum EquationError subeq_evaluate(struct Node *subeq, double *vals,
										 double *res)
{
	assert(vals);
	assert(res);

	if (!subeq) {
		*res = NAN;
		return EQ_NO_ERR;
	}

	if (type(subeq) == MATH_NUM) {
		*res = num(subeq);
		return EQ_NO_ERR;
	}
	if (type(subeq) == MATH_VAR) {
		*res = vals[var(subeq)];
		return EQ_NO_ERR;
	}

	double left = NAN;
	double right = NAN;
	enum EquationError err = EQ_NO_ERR;

	err = subeq_evaluate(subeq->left, vals, &left);
	if (err < 0)
		return err;
	err = subeq_evaluate(subeq->right, vals, &right);
	if (err < 0)
		return err;
	*res = (*MATH_OP_DEFS[op(subeq)].eval)(left, right, &err);
	if (err < 0)
		return err;
	return EQ_NO_ERR;
}

enum EquationError eq_expand_into_teylor(struct Equation eq,
										 size_t extent,
										 struct Equation *teylor)
{
	assert(teylor);
	assert(eq.num_vars <= 1); //can expand only one-variable functions

	char **tmp = (char**) realloc(teylor->var_names,
								  eq.cap_vars * sizeof(char*));
	if (!tmp)
		return EQ_NO_MEM_ERR;
	teylor->var_names = tmp;
	for (size_t i = 0; i < eq.num_vars; i++)
		teylor->var_names[i] = strdup(eq.var_names[i]);
	teylor->num_vars = eq.num_vars;
	teylor->cap_vars = eq.cap_vars;

	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;

	double vals[] = { 0.0 };
	double coeff = NAN;
	double n_fact = 1;

	struct Equation n_diff = {};
	struct Equation eq_tmp = {};

	eq_err = eq_ctor(&n_diff);
	if (eq_err < 0)
		goto finally;
	eq_err = eq_differentiate(eq, 0, &n_diff);
	if (eq_err < 0)
		goto finally;
	eq_err = eq_simplify(&n_diff);
	if (eq_err < 0)
		goto finally;
	eq_err = eq_evaluate(eq, vals, &coeff);
	if (eq_err < 0)
		goto finally;
	teylor->tree = new_num(coeff);
	if (eq_err < 0)
		goto finally;

	for (size_t n = 1; n <= extent; n++) {
		n_fact *= (double) n;
		eq_err = eq_evaluate(n_diff, vals, &coeff);
		if (eq_err < 0)
			goto finally;
		teylor->tree =
			new_op(MATH_ADD, teylor->tree,
				   new_op(MATH_MULT, new_op(MATH_DIV, new_num(coeff),
				   							new_num(n_fact)),
				   		  new_op(MATH_POW, new_var(0), new_num((double) n))));
		if (eq_err < 0)
			goto finally;
		eq_err = eq_differentiate(n_diff, 0, &eq_tmp);
		if (eq_err < 0)
			goto finally;
		eq_dtor(&n_diff);
		n_diff = eq_tmp;
		eq_tmp.tree = NULL;
		eq_tmp.var_names = NULL;
		eq_err = eq_simplify(&n_diff);
		if (eq_err < 0)
			goto finally;
	}

	finally:
		eq_dtor(&n_diff);
		return eq_err;
}

static bool is_equal(double a, double b)
{
	return abs(a - b) < EQ_EPSILON;
}

struct Node *math_diff_add(const struct Node *equation, size_t var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ADD);
	
	return new_op(MATH_ADD, diff(eq_left), diff(eq_right));
}
									 	
double math_eval_add(double l, double r, enum EquationError */*err*/)
{
	return l + r;
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

struct Node *math_diff_sub(const struct Node *equation, size_t var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_SUB);

	return new_op(MATH_SUB, diff(eq_left), diff(eq_right));
}

double math_eval_sub(double l, double r, enum EquationError */*err*/)
{
	return l - r;
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

struct Node *math_diff_mult(const struct Node *equation, size_t var,
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

double math_eval_mult(double l, double r, enum EquationError */*err*/)
{
	return l * r;
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

struct Node *math_diff_div(const struct Node *equation, size_t var,
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
				  new_op(MATH_POW, copy(eq_right), new_num(2)));
}

double math_eval_div(double l, double r, enum EquationError *err)
{
	assert(err);

	if (is_equal(r, 0)) {
		*err = EQ_ZERO_DIV_ERR;
		return NAN;
	}

	return l / r;
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

struct Node *math_diff_pow(const struct Node *equation, size_t var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_POW);

	if (type(eq_right) == MATH_NUM) {
		return new_op(MATH_MULT, new_op(MATH_MULT, copy(eq_right),
					  new_op(MATH_POW, copy(eq_left),
					  		 new_op(MATH_SUB, copy(eq_right), new_num(1)))),
					  diff(eq_left));
	}
	//TODO: copy(equation) instead of copy(eq_left) ^ copy(eq_right)
	if (type(eq_left) == MATH_NUM) {
		return new_op(MATH_MULT, new_op(MATH_MULT,
					  new_op(MATH_POW, copy(eq_left), copy(eq_right)),
					  new_num(log(num(eq_left)))),
					  diff(eq_right));
	}


	return new_op(MATH_MULT, new_op(MATH_POW, new_num(M_E),
				  new_op(MATH_MULT,
					     new_op(MATH_LN, NULL, copy(eq_left)),
					     copy(eq_right))),
				  new_op(MATH_ADD, new_op(MATH_DIV,
				  		 new_op(MATH_MULT, diff(eq_left), copy(eq_right)), 
				  		 copy(eq_left)),
				  		 new_op(MATH_MULT, new_op(MATH_LN, NULL, copy(eq_left)), diff(eq_right))));
}

double math_eval_pow(double l, double r, enum EquationError */*err*/)
{
	return pow(l, r);
}

enum EquationError math_simplify_pow(struct Node *equation)
{
	assert(equation);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_POW);

	if (type(eq_right) == MATH_NUM && is_equal(num(eq_right), 1)) {
		eq_lift_up_left(equation);
	} else if (type(eq_left) == MATH_NUM && (is_equal(num(eq_left), 1) ||
			 is_equal(num(eq_left), 0))) {
		to_num(equation, num(eq_left));
	}

	return EQ_NO_ERR;
}

struct Node *math_diff_ln(const struct Node *equation, size_t var,
						  enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_LN);

	return new_op(MATH_MULT,
				  new_op(MATH_DIV, new_num(1), copy(eq_right)),
				  diff(eq_right));
}

double math_eval_ln(double l, double r, enum EquationError *err)
{
	assert(isnan(l));
	assert(err);

	if (r <= 0) {
		*err = EQ_LN_NEGATIVE_ARG_ERR;
		return NAN;
	}

	return 0;
}

enum EquationError math_simplify_ln (struct Node */*equation*/)
{
	return EQ_NO_ERR;
}

struct Node *math_diff_cos(const struct Node *equation, size_t var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_COS);

	return new_op(MATH_MULT, new_op(MATH_MULT, new_num(-1), 
				  new_op(MATH_SIN, NULL, copy(eq_right))),
				  diff(eq_right));
}

double math_eval_cos(double l, double r, enum EquationError */*err*/)
{
	assert(isnan(l));

	return cos(r);
}
	
enum EquationError math_simplify_cos(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}

struct Node *math_diff_sin(const struct Node *equation, size_t var,
						   enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_SIN);

	return new_op(MATH_MULT, new_op(MATH_COS, NULL, copy(eq_right)),
				  diff(eq_right));
}

double math_eval_sin(double l, double r, enum EquationError */*err*/)
{
	assert(isnan(l));

	return sin(r);
}

enum EquationError math_simplify_sin(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}

struct Node *math_diff_sqrt(const struct Node *equation, size_t var,
						    enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_SQRT);

	return new_op(MATH_DIV, diff(eq_right),
				  new_op(MATH_MULT, new_num(2), 
				  		 new_op(MATH_SQRT, NULL, copy(eq_right))));
}

double math_eval_sqrt(double l, double r, enum EquationError */*err*/)
{
	assert(isnan(l));

	return sqrt(r);
}

enum EquationError math_simplify_sqrt(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}

struct Node *math_diff_tg(const struct Node *equation, size_t var,
						    enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_TG);

	return new_op(MATH_DIV, diff(eq_right),
				  new_op(MATH_POW,
				  		 new_op(MATH_COS, NULL, copy(eq_right)), 
				  		 new_num(2)));
}

double math_eval_tg(double l, double r, enum EquationError */*err*/)
{
	assert(isnan(l));

	return tan(r);
}

enum EquationError math_simplify_tg(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}

struct Node *math_diff_ctg(const struct Node *equation, size_t var,
						    enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_CTG);

	return new_op(MATH_MULT, new_num(-1), 
				  new_op(MATH_DIV, diff(eq_right), new_op(MATH_POW,
				  		 new_op(MATH_SIN, NULL, copy(eq_right)), 
				  		 new_num(2))));
}

double math_eval_ctg(double l, double r, enum EquationError *err)
{
	assert(isnan(l));

	if (is_equal(tan(r), 0)) {
		*err = EQ_WRONG_CTG_ARG_ERR;
		return NAN;
	}
	return 1 / tan(r);
}

enum EquationError math_simplify_ctg(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}

struct Node *math_diff_arcsin(const struct Node *equation, size_t var,
							  enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ARCSIN);

	return new_op(MATH_DIV, diff(eq_right), new_op(MATH_SQRT, NULL,
				  new_op(MATH_SUB, new_num(1), new_op(MATH_POW,
				  		 copy(eq_right), new_num(2)))));
}

double math_eval_arcsin(double l, double r, enum EquationError *err)
{
	assert(isnan(l));

	if (abs(r) > 1) {
		*err = EQ_WRONG_ARCSIN_ARG_ERR;
		return NAN;
	}

	return asin(r);
}

enum EquationError math_simplify_arcsin(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}

struct Node *math_diff_arccos(const struct Node *equation, size_t var,
						    enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ARCCOS);

	return new_op(MATH_MULT, new_num(-1),
				  new_op(MATH_DIV, diff(eq_right), new_op(MATH_SQRT, NULL,
				  new_op(MATH_SUB, new_num(1), new_op(MATH_POW,
				  		 copy(eq_right), new_num(2))))));
}

double math_eval_arccos(double l, double r, enum EquationError *err)
{
	assert(isnan(l));
	
	if (abs(r) > 1) {
		*err = EQ_WRONG_ARCCOS_ARG_ERR;
		return NAN;
	}

	return acos(r);
}

enum EquationError math_simplify_arccos(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}


struct Node *math_diff_arctg(const struct Node *equation, size_t var,
						    enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ARCTG);

	return new_op(MATH_DIV, diff(eq_right), new_op(MATH_ADD, new_num(1), 
				  new_op(MATH_POW, copy(eq_right), new_num(2))));
}

double math_eval_arctg(double l, double r, enum EquationError */*err*/)
{
	assert(isnan(l));
	
	return atan(r);
}

enum EquationError math_simplify_arctg(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}


struct Node *math_diff_arcctg(const struct Node *equation, size_t var,
						    enum EquationError *err)
{
	assert(equation);
	assert(err);
	assert(type(equation) == MATH_OP);
	assert(op(equation) == MATH_ARCCTG);

	return new_op(MATH_DIV, diff(eq_right), new_op(MATH_ADD, new_num(1), 
				  new_op(MATH_POW, copy(eq_right), new_num(2))));
}

double math_eval_arcctg(double l, double r, enum EquationError */*err*/)
{
	assert(isnan(l));
	
	return atan(r);
}

enum EquationError math_simplify_arcctg(struct Node */*equation*/)
{
	return EQ_NO_ERR;
}
