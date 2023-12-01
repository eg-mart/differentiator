#include <string.h>
#include <stdio.h>
#include <math.h>

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
enum ArgError handle_latex_filename(const char *arg_str, void *processed_args);
enum ArgError handle_eval_mode(const char *arg_str, void *processed_args);
enum ArgError handle_teylor_extent(const char *arg_str, void *processed_args);

struct CmdArgs {
	const char *input_file;
	const char *dump_file;
	const char *latex_file;
	bool eval_mode;
	size_t teylor_extent;
};

const ArgDef arg_defs[] = {
	{"input", 'i',  "Name of the input file with a formula",
	 false, false, handle_input_filename},
	{"dump",  'd',  "Name of the html dump file",
	 true, false, handle_dump_filename},
	{"latex", 'l',  "Name of the latex file the formulas will be written to",
	 true, false, handle_latex_filename},
	{"eval",  '\0', "Evaluate the derivative at a certain point",
	 true, true,  handle_eval_mode},
	{"teylor",'\0', "Set extent to which equation will be expanded into"
	 " Teylor's series (3 by default)", true, false, handle_teylor_extent},
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

	struct CmdArgs args = {NULL, NULL, NULL, false, 3};
	struct Buffer buf = {};
	struct Equation eq = {};
	struct Equation diff = {};
	struct Equation teylor = {};

	double *vals = NULL;
	double res = NAN;

	FILE *dump = NULL;
	FILE *latex = NULL;

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
		log_message(ERROR, "A buffer error happened\n");
		goto error;
	}
	buf_err = buffer_load_from_file(&buf, args.input_file);
	if (buf_err < 0) {
		log_message(ERROR, "A buffer error happened\n");
		goto error;
	}

	eq_err = eq_ctor(&eq);
	if (eq_err < 0) {
		log_message(ERROR, "An equation error happened\n");
		goto error;
	}
	eq_ctor(&diff);
	if (eq_err < 0) {
		log_message(ERROR, "An equation error happened\n");
		goto error;
	}

	eqio_err = eq_load_from_buf(&eq, &buf);
	if (eqio_err < 0) {
		log_message(ERROR, eq_io_err_to_str(eqio_err));
		goto error;
	}

	if (args.latex_file) {
		latex = fopen(args.latex_file, "w");
		if (!latex) {
			log_message(ERROR, "Unable to open file %s", args.latex_file);
			goto error;
		}
		eq_start_latex_print(latex);
	}

	eq_print(eq, stdout);
	if (dump)
		TREE_DUMP_GUI(eq, eq_print_token, dump);
	if (latex) {
		fprintf(latex, "Исходное уравнение:\n");
		eq_print_latex(eq, latex);
	}

	eq_err = eq_differentiate(eq, 0, &diff);
	if (eq_err < 0) {
		log_message(ERROR, "An error happened while differentiating\n");
		goto error;
	}
	if (dump)
		TREE_DUMP_GUI(diff, eq_print_token, dump);
	if (latex) {
		fprintf(latex, "Производная (без упрощений):");
		eq_print_latex(diff, latex);
	}

	eq_err = eq_simplify(&diff);
	if (eq_err < 0) {
		log_message(ERROR, "An error happened while simplifying\n");
		goto error;
	}
	eq_print(diff, stdout);
	if (dump)
		TREE_DUMP_GUI(diff, eq_print_token, dump);
	if (latex) {
		fprintf(latex, "Производная (упрощенная):\n");
		eq_print_latex(diff, latex);
	}

	if (args.eval_mode) {
		eq_read_var_values_cli(diff, &vals);
		eq_err = eq_evaluate(diff, vals, &res);
		if (eq_err < 0) {
			log_message(ERROR, "An error happened while evaluating\n");
			goto error;
		}
		printf("Значение производной:\n%lf\n", res);
	}

	eq_err = eq_ctor(&teylor);
	if (eq_err < 0) {
		log_message(ERROR, "An error happened while teyloring\n");
		goto error;
	}

	eq_err = eq_expand_into_teylor(eq, args.teylor_extent, &teylor);
	if (eq_err < 0) {
		log_message(ERROR, "An error happened while teyloring\n");
		goto error;
	}
	eq_err = eq_simplify(&teylor);
	if (eq_err < 0) {
		log_message(ERROR, "An error happened while teyloring\n");
		goto error;
	}
	if (dump)
		TREE_DUMP_GUI(teylor, eq_print_token, dump);
	if (latex) {
		fprintf(latex, "Формула Тейлора:\n");
		eq_print_latex(teylor, latex);
	}

	if (latex) {
		eq_end_latex_print(latex);
		fclose(latex);
		eq_gen_latex_pdf(args.latex_file);
		latex = NULL;
	}

	goto finally;

	error:
		ret_val = 1;
		goto finally;

	finally:
		if (dump)
			tree_end_html_dump(dump);
		free(vals);
		eq_dtor(&eq);
		eq_dtor(&diff);
		eq_dtor(&teylor);
		buffer_dtor(&buf);
		if (latex)
			fclose(latex);
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

enum ArgError handle_latex_filename(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->latex_file = arg_str;
	return ARG_NO_ERR;
}

enum ArgError handle_eval_mode(const char */*arg_str*/, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	args->eval_mode = true;
	return ARG_NO_ERR;
}

enum ArgError handle_teylor_extent(const char *arg_str, void *processed_args)
{
	struct CmdArgs *args = (struct CmdArgs*) processed_args;
	int read = sscanf(arg_str, "%lu", &args->teylor_extent);
	if (read != 1)
		return ARG_WRONG_ARGS_ERR;
	return ARG_NO_ERR;
}
