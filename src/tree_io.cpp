#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "tree_io.h"
#include "logger.h"

static char *skip_space(char *str);
static size_t get_word_len(const char *str);
static bool is_math_token(const char *str);

static enum TreeIOError read_element(elem_t *data, struct Buffer *buf);
static enum TreeIOError subtree_load(struct Node **tree, struct Buffer *buf);

static void print_element(elem_t data, FILE *file);
static void subtree_print(const struct Node *node, FILE *out);

enum TreeIOError tree_load_from_buf(struct Node **tree, struct Buffer *buf)
{
	assert(tree);
	assert(buf);

	buffer_reset(buf);
	enum TreeIOError err = subtree_load(tree, buf);
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

static enum TreeIOError read_element(elem_t *data, struct Buffer *buf)
{
	assert(buf);
	assert(data);

	switch (*buf->pos) {
		case '+':
			data->value.op = MATH_ADD;
			data->type = MATH_OP;
			buf->pos++;
			return TRIO_NO_ERR;
		case '-':
			data->value.op = MATH_SUB;
			data->type = MATH_OP;
			buf->pos++;
			return TRIO_NO_ERR;
		case '/':
			data->value.op = MATH_DIV;
			data->type = MATH_OP;
			buf->pos++;
			return TRIO_NO_ERR;
		case '*':
			data->value.op = MATH_MULT;
			data->type = MATH_OP;
			buf->pos++;
			return TRIO_NO_ERR;
		case '^':
			data->value.op = MATH_POW;
			data->type = MATH_OP;
			buf->pos++;
			return TRIO_NO_ERR;
		default:
			break;
	}

	double num = NAN;
	int read = 0;
	if (sscanf(buf->pos, "%lf%n", &num, &read) != 1) {
		data->type = MATH_VAR;
		data->value.varname = *buf->pos;
		buf->pos++;
		return TRIO_NO_ERR;
	}
	data->type = MATH_NUM;
	data->value.num = num;
	buf->pos += read;

	return TRIO_NO_ERR;
}

static enum TreeIOError subtree_load(struct Node **tree, struct Buffer *buf)
{
	assert(tree);
	assert(buf);

	enum TreeIOError trio_err = TRIO_NO_ERR;

	buf->pos = skip_space(buf->pos);
	size_t word_len = get_word_len(buf->pos);

	if (strncmp(buf->pos, "(", word_len) == 0 && word_len == strlen("(")) {
		buf->pos = skip_space(buf->pos + word_len);
		struct Node *left = NULL;
		trio_err = subtree_load(&left, buf);
		if (trio_err < 0)
			return trio_err;

		buf->pos = skip_space(buf->pos);

		elem_t read = {};
		trio_err = read_element(&read, buf);
		if (trio_err < 0) {
			node_op_delete(left);
			return trio_err;
		}

		enum TreeError tr_err = node_op_new(tree, read);
		if (tr_err < 0) {
			node_op_delete(left);
			return TRIO_TREE_ERR;
		}

		(*tree)->left = left;
		trio_err = subtree_load(&(*tree)->right, buf);
		if (trio_err < 0) {
			node_op_delete(*tree);
			return trio_err;
		}

		buf->pos = skip_space(buf->pos);
		if (buf->pos[0] != ')')
			return TRIO_SYNTAX_ERR;
		(buf->pos)++;
		
		return TRIO_NO_ERR;
	}
	if (is_math_token(buf->pos)) {
		elem_t read = {};
		trio_err = read_element(&read, buf);
		if (trio_err < 0)
			return trio_err;

		enum TreeError tr_err = node_op_new(tree, read);
		if (tr_err < 0)
			return TRIO_TREE_ERR;

		(*tree)->left = NULL;
		(*tree)->right = NULL;

		return TRIO_NO_ERR;
	}

	return TRIO_SYNTAX_ERR;
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

void tree_print(const struct Node *tree, FILE *out)
{
	assert(out);
	assert(tree);

	subtree_print(tree, out);
	fputs("\n", out);
}

void print_element(elem_t data, FILE *out)
{
	switch (data.type) {
		case MATH_OP:
			switch (data.value.op) {
				case MATH_ADD:
					fputs("+", out);
					return;
				case MATH_SUB:
					fputs("-", out);
					return;
				case MATH_DIV:
					fputs("/", out);
					return;
				case MATH_MULT:
					fputs("*", out);
					return;
				case MATH_POW:
					fputs("^", out);
					return;
				default:
					fputs("?", out);
					return;
			}
		case MATH_NUM:
			fprintf(out, "%.lf", data.value.num);
			return;
		case MATH_VAR:
			putc(data.value.varname, out);
			return;
		default:
			fputs("?", out);
			return;
	}
}

void subtree_print(const struct Node *node, FILE *out)
{
	assert(out);

	if (!node)
		return;
	
	if (node->data.type == MATH_OP)
		fputs("(", out);
	subtree_print(node->left, out);
	if (node->data.type == MATH_OP)
		fputs(" ", out);
	print_element(node->data, out);
	if (node->data.type == MATH_OP)
		fputs(" ", out);
	subtree_print(node->right, out);
	if (node->data.type == MATH_OP)
		fputs(")", out);
}

const char *tree_io_err_to_str(enum TreeIOError err)
{
	switch (err) {
		case TRIO_SYNTAX_ERR:
			return "Syntax error in tree's string representation\n";
		case TRIO_TREE_ERR:
			return "Tree error happened while reading the tree\n";
		case TRIO_NO_ERR:
			return "No error occured\n";
		default:
			return "Unknown error occured\n";
	}
}
