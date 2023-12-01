#ifndef _TREE_DEBUG_H
#define _TREE_DEBUG_H

#include <stdio.h>
#include "tree.h"
#include "equation_utils.h"

#define TREE_DUMP_LOG(tr, print_el) \
	tree_dump_log((tr), (print_el), __FILE__, __PRETTY_FUNCTION__, __LINE__, #tr)
#define TREE_DUMP_GUI(tr, print_el, file) \
	tree_dump_gui((tr), (print_el), (file), __FILE__, __PRETTY_FUNCTION__, \
				 __LINE__, #tr)

typedef void (*print_func)(char*, elem_t, struct Equation, size_t);

void tree_dump_log(struct Equation eq, print_func print_el,
				   const char *filename, const char *funcname, int line,
				   const char *varname);
void tree_dump_gui(struct Equation eq, print_func print_el,
				   FILE *dump_html, const char *filename, const char *funcname,
				   int line, const char *varname);
FILE *tree_start_html_dump(const char *filename);
void tree_end_html_dump(FILE *file);

#endif /*_TREE_DEBUG_H*/