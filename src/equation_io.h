#ifndef _TREE_IO_H
#define _TREE_IO_H

#include "buffer.h"
#include "tree.h"

enum EquationIOError {
	EQIO_NO_MEM_ERR = -3,
	EQIO_SYNTAX_ERR = -2,
	EQIO_TREE_ERR	= -1,
	EQIO_NO_ERR		= 0,
};

const char *const OPEN_DELIM = "(";
const char *const CLOSE_DELIM = ")";

enum EquationIOError eq_load_from_buf(struct Equation *eq, struct Buffer *buf);
enum EquationIOError eq_read_var_values_cli(struct Equation eq, double **buf);

void eq_print_token(char *buf, struct MathToken tok,
					struct Equation eq, size_t n);
void eq_print(struct Equation eq, FILE *out);
void eq_start_latex_print(FILE *out);
void eq_print_latex(struct Equation eq, FILE *out);
void eq_end_latex_print(FILE *out);
void eq_gen_latex_pdf(const char *filename);

const char *eq_io_err_to_str(enum EquationIOError err);

#endif /*_TREE_IO_H*/
