#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "equation_manipulation.h"
#include "equation_io.h"
#include "logger.h"
#include "equation_utils.h"
#include "gnuplot_i.h"

static void clear_stdin();

static void get_space(struct Buffer *buf);
static enum EquationIOError get_add(struct Node **subeq, struct Equation *eq,
									struct Buffer *buf);
static enum EquationIOError get_mult(struct Node **subeq, struct Equation *eq,
									 struct Buffer *buf);
static enum EquationIOError get_pow(struct Node **subeq, struct Equation *eq,
									struct Buffer *buf);
static enum EquationIOError get_prim(struct Node **subeq, struct Equation *eq,
										struct Buffer *buf);
static enum EquationIOError get_func(struct Node **subeq, struct Equation *eq,
									 struct Buffer *buf);
static enum EquationIOError get_num(struct Node **subeq, struct Equation *eq,
									struct Buffer *buf);
static enum EquationIOError get_var(struct Node **subeq, struct Equation *eq,
									  struct Buffer *buf);

static void subeq_print(const struct Node *subeq, struct Equation eq, FILE *out);
static void subeq_print_latex(const struct Node *subeq, struct Equation eq,
							  FILE *out, bool put_brackets);

static void eq_to_gnuplot_str(struct Node *subeq, struct Equation eq,
							  struct Buffer *buf);

enum EquationIOError eq_load_from_buf(struct Equation *eq, struct Buffer *buf)
{
	assert(eq);
	assert(buf);

	buffer_reset(buf);
	
	enum EquationIOError eqio_err = get_add(&eq->tree, eq, buf);
	if (eqio_err < 0)
		return eqio_err;
	if (*buf->pos)
		return EQIO_SYNTAX_ERR;
	return EQIO_NO_ERR;
}

static void get_space(struct Buffer *buf)
{
	assert(buf);

	while (*buf->pos && isspace(*buf->pos))
		buf->pos++;
}

static enum EquationIOError get_add(struct Node **subeq, struct Equation *eq,
									struct Buffer *buf)
{
	assert(subeq);
	assert(eq);
	assert(buf);

	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;
	enum EquationIOError eqio_err = get_mult(subeq, eq, buf);
	if (eqio_err < 0)
		return eqio_err;
	get_space(buf);
	while (*buf->pos == '+' || *buf->pos == '-') {
		char op = *buf->pos;
		buf->pos++;
		get_space(buf);
		struct Node *subeq1 = NULL;
		eqio_err = get_mult(&subeq1, eq, buf);
		if (eqio_err < 0) {
			node_op_delete(subeq1);
			return eqio_err;
		}
		switch (op) {
			case '+':
				*subeq = new_op(MATH_ADD, *subeq, subeq1);
				break;
			case '-':
				*subeq = new_op(MATH_SUB, *subeq, subeq1);
				break;
			default:
				return EQIO_UNKNOWN_ERR;
		}
		if (eq_err < 0)
			return EQIO_EQUATION_ERR;
		get_space(buf);
	}
	return EQIO_NO_ERR;
}

static enum EquationIOError get_mult(struct Node **subeq, struct Equation *eq,
									 struct Buffer *buf)
{
	assert(subeq);
	assert(eq);
	assert(buf);

	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;
	enum EquationIOError eqio_err = get_pow(subeq, eq, buf);
	if (eqio_err < 0)
		return eqio_err;
	get_space(buf);
	while (*buf->pos == '*' || *buf->pos == '/') {
		char op = *buf->pos;
		buf->pos++;
		get_space(buf);
		struct Node *subeq1 = NULL;
		eqio_err = get_pow(&subeq1, eq, buf);
		if (eqio_err < 0) {
			node_op_delete(subeq1);
			return eqio_err;
		}
		switch (op) {
			case '*':
				*subeq = new_op(MATH_MULT, *subeq, subeq1);
				break;
			case '/':
				*subeq = new_op(MATH_DIV, *subeq, subeq1);
				break;
			default:
				return EQIO_UNKNOWN_ERR;
		}
		if (eq_err < 0)
			return EQIO_EQUATION_ERR;
		get_space(buf);
	}
	return EQIO_NO_ERR;
}

static enum EquationIOError get_pow(struct Node **subeq, struct Equation *eq,
									struct Buffer *buf)
{
	assert(subeq);
	assert(eq);
	assert(buf);

	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;
	enum EquationIOError eqio_err = get_prim(subeq, eq, buf);
	if (eqio_err < 0)
		return eqio_err;
	get_space(buf);
	while (*buf->pos == '^') {
		buf->pos++;
		get_space(buf);
		struct Node *subeq1 = NULL;
		eqio_err = get_prim(&subeq1, eq, buf);
		if (eqio_err < 0)
			return eqio_err;
		*subeq = new_op(MATH_POW, *subeq, subeq1);
		if (eq_err < 0)
			return EQIO_EQUATION_ERR;
		get_space(buf);
	}
	return EQIO_NO_ERR;
}

static enum EquationIOError get_prim(struct Node **subeq, struct Equation *eq,
									 struct Buffer *buf)
{
	assert(subeq);
	assert(eq);
	assert(buf);

	enum EquationIOError eqio_err = EQIO_NO_ERR;
	if (*buf->pos == '(') {
		buf->pos++;
		get_space(buf);
		eqio_err = get_add(subeq, eq, buf);
		if (eqio_err < 0)
			return eqio_err;
		get_space(buf);
		if (*buf->pos == ')') {
			buf->pos++;
			return EQIO_NO_ERR;
		}
		return EQIO_SYNTAX_ERR;
	} else if (isdigit(*buf->pos) || *buf->pos == '-') {
		return get_num(subeq, eq, buf);
	}
	eqio_err = get_func(subeq, eq, buf);
	if (eqio_err == EQIO_UNKNOWN_FUNC_ERR)
		return get_var(subeq, eq, buf);
	return eqio_err;
}

static enum EquationIOError get_func(struct Node **subeq, struct Equation *eq,
									 struct Buffer *buf)
{
	assert(subeq);
	assert(eq);
	assert(buf);

	enum EquationIOError eqio_err = EQIO_NO_ERR;
	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;
	for (size_t i = (size_t) MATH_POW; i < MATH_OP_DEFS_SIZE; i++) {
		size_t name_len = strlen(MATH_OP_DEFS[i].name);
		if (strncmp(buf->pos, MATH_OP_DEFS[i].name, name_len) == 0) {
			if (*(buf->pos + name_len) != '(')
				return EQIO_SYNTAX_ERR;
			buf->pos += name_len + 1;
			get_space(buf);
			struct Node *subeq_arg = NULL;
			eqio_err = get_add(&subeq_arg, eq, buf);
			if (eqio_err < 0) {
				node_op_delete(subeq_arg);
				return EQIO_EQUATION_ERR;
			}
			*subeq = new_op((enum MathOp) i, NULL, subeq_arg);
			if (eq_err < 0)
				return EQIO_EQUATION_ERR;
			get_space(buf);
			if (*buf->pos != ')')
				return EQIO_SYNTAX_ERR;
			buf->pos++;
			return EQIO_NO_ERR;
		}
	}
	return EQIO_UNKNOWN_FUNC_ERR;
}

static enum EquationIOError get_num(struct Node **subeq, struct Equation *eq,
									struct Buffer *buf)
{
	assert(subeq);
	assert(eq);
	assert(buf);

	double val = 0.0;
	char *old_pos = buf->pos;
	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;

	bool is_neg = false;
	if (*buf->pos == '-') {
		is_neg = true;
		buf->pos++;
	}

	char *old_pos1 = buf->pos;
	while ('0' <= *buf->pos && *buf->pos <= '9') {
		val = val * 10 + *buf->pos - '0';
		buf->pos++;
	}
	if (old_pos1 >= buf->pos) {
		buf->pos = old_pos;
		return EQIO_SYNTAX_ERR;
	}

	if (*buf->pos == '.') {
		buf->pos++;
		double cur_pow = 0.1;
		char *old_pos2 = buf->pos;
		while ('0' <= *buf->pos && *buf->pos <= '9') {
			val += cur_pow * (*buf->pos - '0');
			cur_pow /= 10;
			buf->pos++;
		}
		if (old_pos2 >= buf->pos) {
			buf->pos = old_pos;
			return EQIO_SYNTAX_ERR;
		}
	}

	if (is_neg)
		val *= -1;

	*subeq = new_num(val);
	return EQIO_NO_ERR;
}

static enum EquationIOError get_var(struct Node **subeq, struct Equation *eq,
									struct Buffer *buf)
{
	assert(subeq);
	assert(eq);
	assert(buf);

	if (!(isalpha(*buf->pos) || *buf->pos == '_'))
		return EQIO_SYNTAX_ERR;
	char *old_pos = buf->pos;
	buf->pos++;
	while (isalnum(*buf->pos) || *buf->pos == '_')
		buf->pos++;

	enum EquationError eq_err = EQ_NO_ERR;
	enum EquationError *err = &eq_err;
	size_t var_len = buf->pos - old_pos;
	for (size_t i = 0; i < eq->num_vars; i++) {
		if (strncmp(eq->var_names[i], old_pos, var_len) == 0 &&
			strlen(eq->var_names[i]) == var_len) {
			*subeq = new_var(i);
			if (eq_err < 0)
				return EQIO_EQUATION_ERR;
			return EQIO_NO_ERR;
		}
	}

	if (eq->cap_vars <= eq->num_vars) {
		char **tmp = (char**) realloc(eq->var_names, (eq->cap_vars +
									  EQ_DELTA_VARS_CAPACITY) * sizeof(char*));
		if (!tmp)
			return EQIO_NO_MEM_ERR;
		eq->var_names = tmp;
		eq->cap_vars += EQ_DELTA_VARS_CAPACITY;
	}

	eq->var_names[eq->num_vars] = strndup(old_pos, var_len);
	if (!eq->var_names[eq->num_vars])
		return EQIO_NO_MEM_ERR;
	*subeq = new_var(eq->num_vars);
	if (eq_err < 0)
		return EQIO_EQUATION_ERR;
	eq->num_vars++;

	return EQIO_NO_ERR;
}

enum EquationIOError eq_read_var_values_cli(struct Equation eq, double **buf)
{
	assert(buf);
	assert(!*buf);

	*buf = (double*) calloc(eq.num_vars, sizeof(double));
	if (!*buf)
		return EQIO_NO_MEM_ERR;
	for (size_t i = 0; i < eq.num_vars; i++) {
		printf("Введите значение %s:\n", eq.var_names[i]);
		int read = scanf("%lf", *buf + i);
		if (read != 1) {
			printf("Произошла ошибка. Попробуйте еще раз\n");
			clear_stdin();
			i--;
		}
	}
	return EQIO_NO_ERR;
}

static void clear_stdin()
{
	int a = getchar();
	while (a != '\n')
		a = getchar();
}

void eq_print(struct Equation eq, FILE *out)
{
	assert(out);

	subeq_print(eq.tree, eq, out);
	fputs("\n", out);
}

void eq_print_token(char *buf, struct MathToken tok,
					struct Equation eq, size_t n)
{
	switch (tok.type) {
		case MATH_OP:
			if (tok.value.op > MATH_OP_DEFS_SIZE)
				snprintf(buf, n, "unknown_op#%d", tok.value.op);
			else
				snprintf(buf, n, "%s", MATH_OP_DEFS[tok.value.op].name);
			return;
		case MATH_NUM:
			snprintf(buf, n, "%lf", tok.value.num);
			return;
		case MATH_VAR:
			if (tok.value.var_ind >= eq.num_vars)
				snprintf(buf, n, "unknown_var#%lu", tok.value.var_ind);
			else
				snprintf(buf, n, "%s", eq.var_names[tok.value.var_ind]);
			return;
		default:
			snprintf(buf, n, "unknown_toktype#%d", (int) tok.type);
			return;
	}
}

void subeq_print(const struct Node *subeq, struct Equation eq, FILE *out)
{
	assert(out);

	if (!subeq)
		return;

	const size_t PRINT_BUF_SIZE = 512;
	static char print_buf[PRINT_BUF_SIZE] = "";
	
	if (subeq->data.type == MATH_OP)
		fputs("(", out);
	subeq_print(subeq->left, eq, out);
	if (subeq->data.type == MATH_OP && subeq->left)
		fputs(" ", out);
	eq_print_token(print_buf, subeq->data, eq, PRINT_BUF_SIZE);
	fputs(print_buf, out);
	if (subeq->data.type == MATH_OP && subeq->right)
		fputs(" ", out);
	subeq_print(subeq->right, eq, out);
	if (subeq->data.type == MATH_OP)
		fputs(")", out);
}

static void subeq_print_latex(const struct Node *subeq, struct Equation eq,
							  FILE *out, bool put_brackets)
{
	assert(out);

	if (!subeq)
		return;

	bool put_left_bracks = false;
	bool put_right_bracks = false;
	switch (subeq->data.type) {
		case MATH_NUM:
			fprintf(out, "%.2lf", subeq->data.value.num);
			return;
		case MATH_VAR:
			if (subeq->data.value.var_ind > eq.num_vars)
				fprintf(out, "{unknown_var#%lu}", subeq->data.value.var_ind);
			else
				fprintf(out, "{%s}", eq.var_names[subeq->data.value.var_ind]);
			return;
		case MATH_OP:
			if (subeq->data.value.op >= MATH_OP_DEFS_SIZE) {
				subeq_print_latex(subeq->left, eq, out, true);
				fprintf(out, "unknown_op#%d", (int) subeq->data.value.op);
				subeq_print_latex(subeq->right, eq, out, true);
				return;
			}

			if (subeq->left && subeq->left->data.type == MATH_OP) {
				put_left_bracks = 
					MATH_OP_DEFS[subeq->left->data.value.op].priority >
					MATH_OP_DEFS[subeq->data.value.op].priority;
			}
			
			if (subeq->right && subeq->right->data.type == MATH_OP) {
				put_right_bracks =
					MATH_OP_DEFS[subeq->right->data.value.op].priority >
					MATH_OP_DEFS[subeq->data.value.op].priority;
			}

			if (put_brackets)
				fprintf(out, "(");
			switch (subeq->data.value.op) {
				case MATH_DIV:
					fprintf(out, "\\frac{");
					subeq_print_latex(subeq->left, eq, out, false);
					fprintf(out, "}{");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, "}");
					break;
				case MATH_MULT:
					subeq_print_latex(subeq->left, eq, out, put_left_bracks);
					fprintf(out, " \\cdot ");
					subeq_print_latex(subeq->right, eq, out, put_right_bracks);
					break;
				case MATH_POW:
					subeq_print_latex(subeq->left, eq, out, put_left_bracks);
					fprintf(out, " ^{");
					subeq_print_latex(subeq->right, eq, out, put_right_bracks);
					fprintf(out, "}");
					break;
				case MATH_LN:
					fprintf(out, "\\ln ");
					subeq_print_latex(subeq->right, eq, out, put_right_bracks);
					break;
				case MATH_COS:
					fprintf(out, "\\cos(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_SIN:
					fprintf(out, "\\sin(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_SQRT:
					fprintf(out, "\\sqrt{");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, "}");
					break;
				case MATH_TG:
					fprintf(out, "\\tg(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_CTG:
					fprintf(out, "\\cot(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_ARCSIN:
					fprintf(out, "\\arcsin(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_ARCCOS:
					fprintf(out, "\\arccos(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_ARCTG:
					fprintf(out, "\\arctg(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_ARCCTG:
					fprintf(out, "\\arcctg(");
					subeq_print_latex(subeq->right, eq, out, false);
					fprintf(out, ")");
					break;
				case MATH_ADD:
				case MATH_SUB:
				default:
					subeq_print_latex(subeq->left, eq, out, put_left_bracks);
					fprintf(out, "%s",
							MATH_OP_DEFS[subeq->data.value.op].name);
					subeq_print_latex(subeq->right, eq, out, put_right_bracks);
					break;
			}
			if (put_brackets)
				fprintf(out, ")");
			return;
		default:
			fprintf(out, "unknown_toktype#%d", (int) subeq->data.type);
			return;
	}
}

void eq_print_latex(struct Equation eq, FILE *out)
{
	fprintf(out, "\\begin{equation}\n");
	subeq_print_latex(eq.tree, eq, out, false);
	fprintf(out, "\n\\end{equation}\n\n");
}

void eq_start_latex_print(FILE *out)
{
	fprintf(out, "\\documentclass[a4paper,12pt]{article}\n"
			"\\usepackage[T2A]{fontenc}\n"
			"\\usepackage[utf8]{inputenc}\n"
			"\\usepackage[english,russian]{babel}\n"
			"\\author{Мартыненко Егор, Б01-302}\n"
			"\\title{Учебник по матану. Введение}\n"
			"\\begin{document}\n"
			"\\maketitle\n\n");
}

void eq_end_latex_print(FILE *out)
{
	fprintf(out, "\\end{document}");
}

void eq_gen_latex_pdf(const char *filename)
{
	const size_t CMD_BUF_SIZE = 512;
	char cmd_buf[CMD_BUF_SIZE] = "pdflatex >/dev/null \"";
	size_t prefix_len = strlen(cmd_buf);
	strncat(cmd_buf, filename, CMD_BUF_SIZE - prefix_len);
	strncat(cmd_buf, "\"", CMD_BUF_SIZE - prefix_len - strlen(filename));
	system(cmd_buf);
}

enum EquationIOError eq_graph(struct Equation eq, const char *img_name)
{
	const size_t CMD_BUF_CAP = 1024;
	char cmd_buf[1024] = "set output \"";
	size_t cmd_buf_size = CMD_BUF_CAP - strlen(cmd_buf);
	strncat(cmd_buf, img_name, cmd_buf_size);
	cmd_buf_size -= strlen(img_name);
	strncat(cmd_buf, "\"", cmd_buf_size);

	gnuplot_ctrl *handle = gnuplot_init();
	gnuplot_setterm(handle, "pngcairo", 1024, 768);
	gnuplot_cmd(handle, cmd_buf);
	gnuplot_setstyle(handle, "lines");

	struct Buffer eq_buf = {};
	buffer_ctor(&eq_buf);
	eq_to_gnuplot_str(eq.tree, eq, &eq_buf);
	log_message(INFO, "gnuplot str: %s\n", eq_buf.data);

	gnuplot_cmd(handle, "set samples 10000");
	gnuplot_cmd(handle, "set xrange [-4:4]");
	gnuplot_cmd(handle, "set yrange [-50:50]");
	gnuplot_plot_equation(handle, eq_buf.data, "fucking yeah!!!");
	//gnuplot_cmd(handle, "set parametric");
	//gnuplot_cmd(handle, "set urange [0:6*pi]");
	//gnuplot_cmd(handle, "set vrange [0:1]");
	//gnuplot_cmd(handle, "fx(v) = cos(v)");
	//gnuplot_cmd(handle, "fy(v) = sin(v)");
	//gnuplot_cmd(handle, "fz(v) = v");
	//gnuplot_cmd(handle, "splot fx(v),fy(v),fz(v)");
	gnuplot_close(handle);
	buffer_dtor(&eq_buf);

	return EQIO_NO_ERR;
}

static void eq_to_gnuplot_str(struct Node *subeq, struct Equation eq,
							  struct Buffer *buf)
{
	if (!subeq)
		return;

	snprintf(buf->pos, buf->size - buffer_size(buf), "(");
	buf->pos++;
	eq_to_gnuplot_str(subeq->left, eq, buf);
	if (subeq->data.type == MATH_OP) {
		switch (subeq->data.value.op) {
			case MATH_POW:
				snprintf(buf->pos, buf->size - buffer_size(buf), "**");
				break;
			case MATH_TG:
				snprintf(buf->pos, buf->size - buffer_size(buf), "tan");
				break;
			case MATH_CTG:
				snprintf(buf->pos, buf->size - buffer_size(buf), "(1/tan");
				break;
			case MATH_ARCTG:
				snprintf(buf->pos, buf->size - buffer_size(buf), "atan");
				break;
			default:
				eq_print_token(buf->pos, subeq->data, eq,
							   buf->size - buffer_size(buf));
				break;
		}
	} else {
		eq_print_token(buf->pos, subeq->data, eq,
					   buf->size - buffer_size(buf));
	}
	buf->pos += strlen(buf->pos);
	eq_to_gnuplot_str(subeq->right, eq, buf);
	if (subeq->data.value.op == MATH_CTG) {
		snprintf(buf->pos, buf->size - buffer_size(buf), ")");
		buf->pos++;
	}
	snprintf(buf->pos, buf->size - buffer_size(buf), ")");
	buf->pos++;
}

const char *eq_io_err_to_str(enum EquationIOError err)
{
	switch (err) {
		case EQIO_SYNTAX_ERR:
			return "Syntax error in tree's string representation\n";
		case EQIO_TREE_ERR:
			return "Tree error happened while reading the tree\n";
		case EQIO_NO_ERR:
			return "No error occured\n";
		case EQIO_NO_MEM_ERR:
			return "Not enough memory to store all variables\n";
		default:
			return "Unknown error occured\n";
	}
}