#include <string.h>
#include <stdio.h>

#include "logger.h"
#include "tree.h"
#include "tree_debug.h"
#include "tree_io.h"
#include "differentiator.h"
#include "buffer.h"
#include "cmd_args.h"

void print_math_token(char *buf, struct MathToken tok, size_t n);

enum ArgError handle_input_filename(const char *arg_str, void *processed_args);
enum ArgError handle_dump_filename(const char *arg_str, void *processed_args);

struct CmdArgs {
	const char *input_file;
	const char *dump_file;
};

const ArgDef arg_defs[] = {
	{"input", 'i', "Name of the input file with a formula",
	 false, false, handle_input_filename},
	{"dump", 'd', "Name of the html dump file",
	 false, false, handle_dump_filename},
};

int main(int argc, const char *argv[])
{
	logger_ctor();
	add_log_handler({stderr, DEBUG, true});


	enum ArgError arg_err = ARG_NO_ERR;
	enum BufferError buf_err = BUF_NO_ERR;
	enum TreeIOError trio_err = TRIO_NO_ERR;

	struct CmdArgs args = {};
	struct Buffer buf = {};
	struct Node *tr = NULL;

	FILE *dump = NULL;

	arg_err = process_args(arg_defs, sizeof(arg_defs) / sizeof(arg_defs[0]),
						   argv, argc, &args);
	if (arg_err < 0) {
		log_message(ERROR, arg_err_to_str(arg_err));
		goto error;
	} else if (arg_err == ARG_HELP_CALLED) {
		return 0;
	}

	dump = tree_start_html_dump(args.dump_file);
	if (!dump) {
		log_message(ERROR, "Error opening dump file %s\n", args.dump_file);
		goto error;
	}

	buf_err = buffer_ctor(&buf);
	if (buf_err < 0) {
		goto error;
	}
	buf_err = buffer_load_from_file(&buf, args.input_file);
	if (buf_err < 0) {
		goto error;
	}

	trio_err = tree_load_from_buf(&tr, &buf);
	if (trio_err < 0) {
		log_message(ERROR, tree_io_err_to_str(trio_err));
		goto error;
	}
	TREE_DUMP_GUI(tr, print_math_token, dump);

	error:
		buffer_dtor(&buf);
		node_op_delete(tr);
		logger_dtor();
		return 1;

	buffer_dtor(&buf);
	node_op_delete(tr);
	tree_end_html_dump(dump);
	logger_dtor();

	return 0;
}

void print_math_token(char *buf, struct MathToken tok, size_t n)
{
	switch (tok.type) {
		case MATH_OP:
			switch (tok.value.op) {
				case MATH_ADD:
					*buf = '+';
					break;
				case MATH_SUB:
					*buf = '-';
					break;
				case MATH_DIV:
					*buf = '/';
					break;
				case MATH_MULT:
					*buf = '*';
					break;
				default:
					strncpy(buf, "err: wrong op", n);
					break;
			}
			break;
		case MATH_NUM:
			snprintf(buf, n, "%lf", tok.value.num);
			break;
		default:
			strncpy(buf, "err: wrong type", n);
			break;
	}
}

enum ArgError handle_input_filename(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->input_file = arg_str;
	return ARG_NO_ERR;
}

enum ArgError handle_dump_filename(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->dump_file = arg_str;
	return ARG_NO_ERR;
}
