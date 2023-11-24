#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "tree_io.h"
#include "differentiator.h"
#include "logger.h"

static char *skip_space(char *str);
static size_t get_word_len(const char *str);

static enum TreeIOError read_element(elem_t *data, struct Buffer *buf);
static enum TreeIOError _tree_load(struct Node **tree, struct Buffer *buf);
static void _subtree_save(const struct Node *node, FILE *out, size_t level);

enum TreeIOError tree_load_from_buf(struct Node **tree, struct Buffer *buf)
{
	assert(tree);
	assert(buf);

	buffer_reset(buf);
	enum TreeIOError err = _tree_load(tree, buf);
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
	if (!isalpha(str[0]) && str[0] != '_')
		return 1;
	
	while (str[len] != '\0' && (isalnum(str[len]) || str[len] == '_'))
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
		default:
			break;
	}

	double num = NAN;
	int read = 0;
	if (sscanf(buf->pos, "%lf%n", &num, &read) != 1) {
		return TRIO_SYNTAX_ERR;
	}
	data->type = MATH_NUM;
	data->value.num = num;
	buf->pos += read;

	return TRIO_NO_ERR;
}

static enum TreeIOError _tree_load(struct Node **tree, struct Buffer *buf)
{
	assert(tree);
	assert(buf);

	enum TreeIOError trio_err = TRIO_NO_ERR;

	buf->pos = skip_space(buf->pos);
	size_t word_len = get_word_len(buf->pos);

	if (strncmp(buf->pos, "_", word_len) == 0 && word_len == strlen("_")) {
		buf->pos = skip_space(buf->pos + word_len);
		*tree = NULL;
		return TRIO_NO_ERR;
	}
	if (strncmp(buf->pos, "(", word_len) == 0 && word_len == strlen("("))  {
		buf->pos += word_len;
		struct Node *left = NULL;
		trio_err = _tree_load(&left, buf);
		if (trio_err < 0)
			return trio_err;

		buf->pos = skip_space(buf->pos);

		elem_t read = {};
		trio_err = read_element(&read, buf);
		if (trio_err < 0)
			return trio_err;

		enum TreeError tr_err = node_op_new(tree, read);
		if (tr_err < 0)
			return TRIO_TREE_ERR;

		(*tree)->left = left;
		trio_err = _tree_load(&(*tree)->right, buf);
		if (trio_err < 0)
			return trio_err;

		buf->pos = skip_space(buf->pos);
		if (buf->pos[0] != ')')
			return TRIO_SYNTAX_ERR;
		(buf->pos)++;
		return TRIO_NO_ERR;
	}
	return TRIO_SYNTAX_ERR;
}

void tree_save(const struct Node *tree, FILE *out)
{
	assert(out);

	_subtree_save(tree, out, 0);
}

void _subtree_save(const struct Node *node, FILE *out, size_t level)
{
	assert(out);

	for (size_t i = 0; i < level; i++)
		fputs("    ", out);
	
	if (!node) {
		fputs("nil\n", out);
		return;
	}

	fprintf(out, "(<%s>\n", node->data);
	_subtree_save(node->left, out, level + 1);
	_subtree_save(node->right, out, level + 1);

	for (size_t i = 0; i < level; i++)
		fputs("    ", out);
	fputs(")\n", out);
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
