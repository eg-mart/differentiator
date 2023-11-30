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
static bool is_math_token(const char *str);
static bool is_math_op(const char *str);

static enum EquationIOError read_element(struct MathToken *data,
										 struct Buffer *buf);
static enum EquationIOError subtree_load(struct Node **tree, struct Buffer *buf);

static void subtree_print(const struct Node *node, FILE *out);
static void subtree_print_latex(const struct Node *eq, FILE *out);

enum EquationIOError eq_load_from_buf(struct Node **equation, struct Buffer *buf)
{
	assert(equation);
	assert(buf);

	buffer_reset(buf);
	enum EquationIOError err = subtree_load(equation, buf);
	buffer_reset(buf);
	return err;
}

static char *skip_space(char *str)
{
	assert(str);

	while (isspace(*str))
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

static enum EquationIOError read_element(struct MathToken *data,
										 struct Buffer *buf)
{
	assert(buf);
	assert(data);

	size_t tok_len = get_word_len(buf->pos);
	for (size_t i = 0; i < MATH_OP_DEFS_SIZE; i++) {
		if (strncmp(buf->pos, MATH_OP_DEFS[i].name, tok_len) == 0 &&
			strlen(MATH_OP_DEFS[i].name) == tok_len) {
			data->type = MATH_OP;
			data->value.op = (enum MathOp) i;
			buf->pos += tok_len;
			return EQIO_NO_ERR;
		}
	}

	double num = NAN;
	int read = 0;
	if (sscanf(buf->pos, "%lf%n", &num, &read) == 1) {
		data->type = MATH_NUM;
		data->value.num = num;
		buf->pos += read;
		return EQIO_NO_ERR;
	}

	data->type = MATH_VAR;
	data->value.varname = *buf->pos;
	buf->pos++;

	return EQIO_NO_ERR;
}

static enum EquationIOError subtree_load(struct Node **equation,
										 struct Buffer *buf)
{
	assert(equation);
	assert(buf);

	enum EquationIOError eqio_err = EQIO_NO_ERR;

	buf->pos = skip_space(buf->pos);
	size_t word_len = get_word_len(buf->pos);

	if (strncmp(buf->pos, OPEN_DELIM, word_len) == 0 &&
		word_len == strlen(CLOSE_DELIM)) {
		buf->pos = skip_space(buf->pos + word_len);
		struct Node *left = NULL;
		eqio_err = subtree_load(&left, buf);
		if (eqio_err < 0)
			return eqio_err;

		buf->pos = skip_space(buf->pos);
		elem_t read = {};
		eqio_err = read_element(&read, buf);
		if (eqio_err < 0) {
			node_op_delete(left);
			return eqio_err;
		}

		enum TreeError tr_err = node_op_new(equation, read);
		if (tr_err < 0) {
			node_op_delete(left);
			return EQIO_TREE_ERR;
		}
		(*equation)->left = left;

		eqio_err = subtree_load(&(*equation)->right, buf);
		if (eqio_err < 0) {
			node_op_delete(*equation);
			return eqio_err;
		}

		buf->pos = skip_space(buf->pos);
		if (buf->pos[0] != ')') {
			node_op_delete(*equation);
			return EQIO_SYNTAX_ERR;
		}
		(buf->pos)++;
		
		return EQIO_NO_ERR;
	}

	if (is_math_token(buf->pos) && !is_math_op(buf->pos)) {
		elem_t read = {};
		eqio_err = read_element(&read, buf);
		if (eqio_err < 0)
			return eqio_err;

		enum TreeError tr_err = node_op_new(equation, read);
		if (tr_err < 0)
			return EQIO_TREE_ERR;

		(*equation)->left = NULL;
		(*equation)->right = NULL;

		return EQIO_NO_ERR;
	}

	return EQIO_SYNTAX_ERR;
}

static bool is_math_token(const char *str)
{
	size_t tok_len = get_word_len(str);
	size_t alpha_cnt = 0;
	bool is_num = true;
	for (size_t i = 0; i < tok_len; i++) {
		if (!isdigit(str[i]) && str[i] != '.' && str[i] != ',') {
			is_num = false;
			if (isalpha(str[i]))
				alpha_cnt++;
		}
	}
	return (alpha_cnt == 1) || is_num;
}

static bool is_math_op(const char *str)
{
	size_t tok_len = get_word_len(str);
	for (size_t i = 0; i < MATH_OP_DEFS_SIZE; i++) {
		if (strncmp(str, MATH_OP_DEFS[i].name, tok_len) == 0 &&
			strlen(MATH_OP_DEFS[i].name) == tok_len)
			return true;
	}
	return false;
}

void eq_print(const struct Node *equation, FILE *out)
{
	assert(out);
	assert(equation);

	subtree_print(equation, out);
	fputs("\n", out);
}

void eq_print_token(char *buf, struct MathToken tok, size_t n)
{
	switch (tok.type) {
		case MATH_OP:
			snprintf(buf, n, "%s", MATH_OP_DEFS[tok.value.op].name);
			return;
		case MATH_NUM:
			snprintf(buf, n, "%.lf", tok.value.num);
			return;
		case MATH_VAR:
			snprintf(buf, n, "%c", tok.value.varname);
			return;
		default:
			snprintf(buf, n, "err");
			return;
	}
}

void subtree_print(const struct Node *node, FILE *out)
{
	assert(out);

	if (!node)
		return;

	const size_t PRINT_BUF_SIZE = 512;
	static char print_buf[PRINT_BUF_SIZE] = "";
	
	if (node->data.type == MATH_OP)
		fputs("(", out);
	subtree_print(node->left, out);
	if (node->data.type == MATH_OP)
		fputs(" ", out);
	eq_print_token(print_buf, node->data, PRINT_BUF_SIZE);
	fputs(print_buf, out);
	if (node->data.type == MATH_OP)
		fputs(" ", out);
	subtree_print(node->right, out);
	if (node->data.type == MATH_OP)
		fputs(")", out);
}

static void subtree_print_latex(const struct Node *eq, FILE *out)
{
	assert(out);

	if (!eq)
		return;

	switch (eq->data.type) {
		case MATH_NUM:
			fprintf(out, "%.2lf", eq->data.value.num);
			return;
		case MATH_VAR:
			fprintf(out, "%c", eq->data.value.varname);
			return;
		case MATH_OP:
			switch (eq->data.value.op) {
				case MATH_DIV:
					fprintf(out, "\\frac{");
					subtree_print_latex(eq->left, out);
					fprintf(out, "}{");
					subtree_print_latex(eq->right, out);
					fprintf(out, "}");
					break;
				case MATH_ADD:
				case MATH_SUB:
				case MATH_MULT:
				default:
					fprintf(out, "(");
					subtree_print_latex(eq->left, out);
					fprintf(out, " %s ", MATH_OP_DEFS[eq->data.value.op].name);
					subtree_print_latex(eq->right, out);
					fprintf(out, ")");
					break;
			}
			return;
		default:
			fprintf(out, "UNKNOWN");
			return;
	}
}

void eq_print_latex(const struct Node *equation, FILE *out)
{
	fprintf(out, "\\begin{equation}\n");
	subtree_print_latex(equation, out);
	fprintf(out, "\\end{equation}\n");
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
			"\\maketitle\n");
}

void eq_end_latex_print(FILE *out)
{
	fprintf(out, "\n\\end{document}");
	fclose(out);
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
		default:
			return "Unknown error occured\n";
	}
}