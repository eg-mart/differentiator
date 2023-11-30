#include <string.h>
#include <stdio.h>

#include "logger.h"
#include "tree.h"
#include "tree_debug.h"
#include "equation_io.h"
#include "equation_utils.h"
#include "buffer.h"
#include "../lib-cmd-args/src/cmd_args.h"

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
	 true, false, handle_dump_filename},
};
const size_t ARG_DEFS_SIZE = sizeof(arg_defs) / sizeof(arg_defs[0]);

int main(int argc, const char *argv[])
{
	logger_ctor();
	add_log_handler({stderr, DEBUG, true});

	int ret_val = 0;

	enum ArgError arg_err = ARG_NO_ERR;
	enum BufferError buf_err = BUF_NO_ERR;
	enum EquationIOError eqio_err = EQIO_NO_ERR;
	enum EquationError eq_err = EQ_NO_ERR;

	struct CmdArgs args = {};
	struct Buffer buf = {};
	struct Node *tr = NULL;
	struct Node *diff = NULL;

	FILE *dump = NULL;

	arg_err = process_args(arg_defs, ARG_DEFS_SIZE, argv, argc, &args);
	if (arg_err < 0) {
		log_message(ERROR, arg_err_to_str(arg_err));
		arg_show_usage(arg_defs, ARG_DEFS_SIZE, argv[0]);
		goto error;
	} else if (arg_err == ARG_HELP_CALLED) {
		return 0;
	}

	if (args.dump_file) {
		dump = tree_start_html_dump(args.dump_file);
		if (!dump) {
			log_message(ERROR, "Error opening dump file %s\n", args.dump_file);
			goto error;
		}
		setvbuf(dump, NULL, _IONBF, 0);
	}

	buf_err = buffer_ctor(&buf);
	if (buf_err < 0) {
		goto error;
	}
	buf_err = buffer_load_from_file(&buf, args.input_file);
	if (buf_err < 0) {
		goto error;
	}

	eqio_err = eq_load_from_buf(&tr, &buf);
	if (eqio_err < 0) {
		log_message(ERROR, eq_io_err_to_str(eqio_err));
		goto error;
	}

	TREE_DUMP_GUI(tr, eq_print_token, dump);
	eq_print(tr, stdout);
	diff = eq_differentiate(tr, 'x', &eq_err);
	TREE_DUMP_GUI(diff, eq_print_token, dump);
	eq_err = eq_simplify(diff);
	eq_err = eq_simplify(tr);
	TREE_DUMP_GUI(diff, eq_print_token, dump);
	//eq_err = eq_simplify(&diff);
	//TREE_DUMP_GUI(diff, eq_print_token, dump);
	eq_print(tr, stdout);
	eq_print(diff, stdout);

	goto finally;

	error:
		ret_val = 1;
		goto finally;

	finally:
		buffer_dtor(&buf);
		node_op_delete(tr);
		node_op_delete(diff);
		tree_end_html_dump(dump);
		logger_dtor();
		return ret_val;

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
