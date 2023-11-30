#ifndef _TREE_IO_H
#define _TREE_IO_H

#include "buffer.h"
#include "tree.h"

enum EquationIOError {
	EQIO_SYNTAX_ERR = -2,
	EQIO_TREE_ERR	= -1,
	EQIO_NO_ERR		= 0,
};

const char *const OPEN_DELIM = "(";
const char *const CLOSE_DELIM = ")";

enum EquationIOError eq_load_from_buf(struct Node **equation,
									  struct Buffer *buf);
void eq_print_token(char *buf, struct MathToken tok, size_t n);
void eq_print(const struct Node *equation, FILE *out);
const char *eq_io_err_to_str(enum EquationIOError err);

#endif /*_TREE_IO_H*/
