#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "equation_io.h"
#include "logger.h"
#include "equation_utils.h"

static char *skip_space(char *str);
static size_t get_word_len(const char *str);
static void clear_stdin();

static enum EquationIOError eq_read_op(struct MathToken *tok,
									   struct Buffer *buf);
static enum EquationIOError eq_read_num(struct MathToken *tok,
										struct Buffer *buf);
static enum EquationIOError eq_read_var(struct MathToken *tok,
										struct Equation *eq, struct Buffer *buf);

static enum EquationIOError subeq_load(struct Node **subeq, struct Equation *eq,
									   struct Buffer *buf);

static void subeq_print(const struct Node *subeq, struct Equation eq, FILE *out);
static void subeq_print_latex(const struct Node *subeq, struct Equation eq,
							  FILE *out, bool put_brackets);

enum EquationIOError eq_load_from_buf(struct Equation *eq, struct Buffer *buf)
{
	assert(eq);
	assert(buf);

	buffer_reset(buf);
	enum EquationIOError err = subeq_load(&eq->tree, eq, buf);
	buffer_reset(buf);
	return err;
}

static char *skip_space(char *str)
{
	assert(str);

	while (*str && isspace(*str))
		str++;
	return str;
}

static size_t get_word_len(const char *str)
{
	size_t len = 0;
	if (!isalnum(str[0]) && str[0] != '_')
		return 1;
	
	while (str[len] != '\0' && (isalnum(str[len]) ||
		   str[len] == '_' || str[len] == '.' || str[len] == ','))
		len++;
	return len;
}

static enum EquationIOError subeq_load(struct Node **subeq, struct Equation *eq,
									   struct Buffer *buf)
{
	assert(eq);
	assert(subeq);
	assert(buf);

	static size_t OPEN_DELIM_LEN = strlen(OPEN_DELIM);
	static size_t CLOSE_DELIM_LEN = strlen(CLOSE_DELIM);

	enum EquationIOError eqio_err = EQIO_NO_ERR;

	buf->pos = skip_space(buf->pos);
	size_t word_len = get_word_len(buf->pos);

	if (strncmp(buf->pos, OPEN_DELIM, word_len) == 0 &&
		word_len == OPEN_DELIM_LEN) {
		buf->pos = skip_space(buf->pos + word_len);
		struct Node *left = NULL;
		eqio_err = subeq_load(&left, eq, buf);
		if (eqio_err < 0)
			return eqio_err;
		if (left && left->data.type == MATH_VAR) {
			buf->pos[0] = '\0';
			buf->pos++;
		}

		buf->pos = skip_space(buf->pos);
		elem_t read = {};
		eqio_err = eq_read_op(&read, buf);
		if (eqio_err < 0) {
			node_op_delete(left);
			return eqio_err;
		}

		enum TreeError tr_err = node_op_new(subeq, read);
		if (tr_err < 0) {
			node_op_delete(left);
			return EQIO_TREE_ERR;
		}
		(*subeq)->left = left;

		eqio_err = subeq_load(&(*subeq)->right, eq, buf);
		if (eqio_err < 0) {
			node_op_delete(*subeq);
			*subeq = NULL;
			return eqio_err;
		}

		buf->pos = skip_space(buf->pos);
		//TODO: checking for closing braces
		word_len = get_word_len(buf->pos);
		if (!strncmp(buf->pos, CLOSE_DELIM, word_len) == 0 ||
			word_len != CLOSE_DELIM_LEN) {
			node_op_delete(*subeq);
			*subeq = NULL;
			return EQIO_SYNTAX_ERR;
		}
		if ((*subeq)->right->data.type == MATH_VAR)
			buf->pos[0] = '\0';
		buf->pos += word_len;
		
		return EQIO_NO_ERR;
	}

	elem_t read = {};
	eqio_err = eq_read_num(&read, buf);

	if (eqio_err < 0)
		eqio_err = eq_read_var(&read, eq, buf);

	if (eqio_err == EQIO_SYNTAX_ERR) {
		*subeq = NULL;
		log_message(DEBUG, "it worked\n");
		return EQIO_NO_ERR;
	}

	if (eqio_err < 0)
		return eqio_err;
	
	enum TreeError tr_err = node_op_new(subeq, read);
	if (tr_err < 0)
		return EQIO_TREE_ERR;

	(*subeq)->left = NULL;
	(*subeq)->right = NULL;

	return eqio_err;
}

static enum EquationIOError eq_read_op(struct MathToken *tok,
									   struct Buffer *buf)
{
	size_t word_len = get_word_len(buf->pos);
	for (size_t i = 0; i < MATH_OP_DEFS_SIZE; i++) {
		//TODO: less strlen(), more execution speed
		if (strncmp(buf->pos, MATH_OP_DEFS[i].name, word_len) == 0 &&
			strlen(MATH_OP_DEFS[i].name) == word_len) {
			tok->type = MATH_OP;
			tok->value.op = (enum MathOp) i;
			buf->pos += word_len;
			return EQIO_NO_ERR;
		}
	}

	return EQIO_SYNTAX_ERR;
}

static enum EquationIOError eq_read_num(struct MathToken *tok,
										struct Buffer *buf)
{
	size_t word_len = get_word_len(buf->pos);
	int read_len = 0;
	double val = NAN;
	int read_status = sscanf(buf->pos, "%lf%n", &val, &read_len);
	if (read_status != 1 || word_len != (size_t) read_len)
		return EQIO_SYNTAX_ERR;
	tok->type = MATH_NUM;
	tok->value.num = val;
	buf->pos += word_len;
	return EQIO_NO_ERR;
}

static enum EquationIOError eq_read_var(struct MathToken *tok,
										struct Equation *eq, struct Buffer *buf)
{
	size_t word_len = get_word_len(buf->pos);
	for (size_t i = 0; i < MATH_OP_DEFS_SIZE; i++) {
		//TODO: less strlen(), more execution speed
		if (strncmp(buf->pos, MATH_OP_DEFS[i].name, word_len) == 0 &&
			strlen(MATH_OP_DEFS[i].name) == word_len) {
			return EQIO_SYNTAX_ERR;
		}
	}

	for (size_t i = 0; i < eq->num_vars; i++) {
		if (strncmp(eq->var_names[i], buf->pos, word_len) == 0 &&
			strlen(eq->var_names[i]) == word_len) {
			tok->type = MATH_VAR;
			tok->value.var_ind = i;
			buf->pos += word_len;
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

	eq->var_names[eq->num_vars] = buf->pos;
	//buf->pos[word_len] = '\0';
	tok->type = MATH_VAR;
	tok->value.var_ind = eq->num_vars;
	eq->num_vars++;
	
	buf->pos += word_len;

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