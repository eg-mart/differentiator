#ifndef _TREE_IO_H
#define _TREE_IO_H

#include "buffer.h"
#include "tree.h"

enum TreeIOError {
	TRIO_SYNTAX_ERR = -2,
	TRIO_TREE_ERR	= -1,
	TRIO_NO_ERR		= 0,
};

enum TreeIOError tree_load_from_buf(struct Node **tree, struct Buffer *buf);
void tree_save(const struct Node *tree, FILE *out);
const char *tree_io_err_to_str(enum TreeIOError err);

#endif /*_TREE_IO_H*/
