#include <stdlib.h>
#include <assert.h>

#include "tree.h"

enum TreeError node_op_new(struct Node **node, elem_t data)
{
	assert(node);

	*node = (struct Node*) calloc(1, sizeof(struct Node));
	if (!(*node))
		return TREE_NO_MEM_ERR;

	node_ctor(*node, data);
	return TREE_NO_ERR;
}

void node_ctor(struct Node *node, elem_t data)
{
	assert(node);

	node->data = data;
	node->left = NULL;
	node->right = NULL;
}

void node_op_delete(struct Node *node)
{
	if (!node)
		return;
	
	node_op_delete(node->left);
	node_op_delete(node->right);
	free(node);
}

const char *tree_err_to_str(enum TreeError err)
{
	switch (err) {
		case TREE_NO_MEM_ERR:
			return "No memory\n";
		case TREE_NO_ERR:
			return "No error occured\n";
		default:
			return "An unknown error occured\n";
	}
}