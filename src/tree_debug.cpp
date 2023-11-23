#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "tree_debug.h"
#include "logger.h"

const int ELEM_BUF_SIZE = 1024;

static void _subtree_dump_log(const struct Node *node, print_func print_el,
							  size_t level);
static void _subtree_dump_gui(const struct Node *node, print_func print_el,
							  FILE *dump, size_t node_id);

void tree_dump_log(const struct Node *tr, print_func print_el,
				   const char *filename, const char* funcname, int line,
				   const char* varname)
{
	assert(tr);
	assert(print_el);
	assert(filename);
	assert(funcname);
	assert(varname);

	log_message(DEBUG, "Dumping tree %s[%p]:\n", varname, tr);
	log_message(DEBUG, "(called from %s:%d %s)\n", filename, line, funcname);
	_subtree_dump_log(tr, print_el, 0);
	log_message(DEBUG, "Dumping of %s[%p] ended\n", varname, tr);
}

static void _subtree_dump_log(const struct Node *node, print_func print_el,
						  size_t level)
{
	assert(print_el);

	static char buf[ELEM_BUF_SIZE] = {};
	
	log_string(DEBUG, "   ");
	for (size_t i = 0; i < level; i++)
		log_string(DEBUG, "   ");
	if (!node) {
		log_string(DEBUG, "    nil\n");
		return;
	}
	log_string(DEBUG, "{\n");

	log_string(DEBUG, "   ");
	for (size_t i = 0; i < level; i++)
		log_string(DEBUG, "   ");

	buf[0] = '\0';
	print_el(buf, node->data, ELEM_BUF_SIZE - 1);
	log_string(DEBUG, "   %s\n", buf);

	_subtree_dump_log(node->left, print_el, level + 1);
	_subtree_dump_log(node->right, print_el, level + 1);
	
	log_string(DEBUG, "   ");
	for (size_t i = 0; i < level; i++)
		log_string(DEBUG, "   ");
	log_string(DEBUG, "}\n");
}

FILE *tree_start_html_dump(const char *filename)
{
	assert(filename);

	FILE *file = fopen(filename, "w");
	if (!file)
		return NULL;

	fputs("<!DOCTYPE html>\n"
		  "<html lang=\"ru\">\n"
		  "<head>\n"
		  "    <meta charset=\"UTF-8\">\n"
		  "    <title>Tree dump</title>\n"
		  "</head>\n"
		  "<body>\n",
		  file);
	return file;
}

void tree_end_html_dump(FILE *file)
{
	assert(file);

	fputs("</body>\n"
		  "</html>\n",
		  file);
	fclose(file);
}

void tree_dump_gui(const struct Node *tr, print_func print_el, FILE *dump_html,
				   const char *filename, const char *funcname, int line,
				   const char *varname)
{
	assert(tr);
	assert(print_el);
	assert(filename);
	assert(funcname);
	assert(varname);
	assert(dump_html);

	log_message(DEBUG, "HTML-dumping tree %s[%p]:\n", varname, tr);
	log_message(DEBUG, "(called from %s:%d %s)\n", filename, line, funcname);

	static size_t dump_count = 0;

	const size_t FILENAME_SIZE = 512;

	time_t rawtime = 0;
	struct tm *timeinfo = {};
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char dump_prefix[FILENAME_SIZE] = {};
	strftime(dump_prefix, FILENAME_SIZE, "dump/%b-%d-%T", timeinfo);
	snprintf(dump_prefix + strlen(dump_prefix),
			 FILENAME_SIZE - strlen(dump_prefix), "-%lu", dump_count);
	dump_count++;

	char dot_name[FILENAME_SIZE] = {};
	strncpy(dot_name, dump_prefix, FILENAME_SIZE);
	strncat(dot_name, ".dot", FILENAME_SIZE - strlen(dot_name));

	mkdir("dump", 0777);
	FILE *dot_file = fopen(dot_name, "w");
	if (!dot_file) {
		log_message(ERROR, "Opening dot file %s for dumping failed\n", dot_name);
		return;
	}

	const char *BEGIN = \
	"digraph {\n"
	"graph [dpi = 200, splines=ortho];\n"
	"node [shape = \"Mrecord\"];\n";

	fputs(BEGIN, dot_file);
	_subtree_dump_gui(tr, print_el, dot_file, 0);
	fputs("}\n", dot_file);
	fclose(dot_file);

	char image_name[FILENAME_SIZE] = {};
	strncpy(image_name, dump_prefix, FILENAME_SIZE);
	strncat(image_name, ".png", FILENAME_SIZE - strlen(image_name));

	const size_t CMD_SIZE = 512;
	char gen_dot_cmd[CMD_SIZE] = "dot -Tpng -o \"";
	strncat(gen_dot_cmd, image_name, CMD_SIZE - strlen(gen_dot_cmd));
	strncat(gen_dot_cmd, "\" \"", CMD_SIZE - strlen(gen_dot_cmd));
	strncat(gen_dot_cmd, dot_name, CMD_SIZE - strlen(gen_dot_cmd));
	strncat(gen_dot_cmd, "\"", CMD_SIZE - strlen(gen_dot_cmd));
	system(gen_dot_cmd);

	fprintf(dump_html, "<hr>\n<p style=\"font-size:30px\">"
			"Tree %s[%p]</br>\n", varname, tr);
	fprintf(dump_html, "(called from %s:%d %s</p>\n", filename, line, funcname);
	fprintf(dump_html, "<img src=\"%s\">\n", image_name);
	
	log_message(DEBUG, "HTML-dumping of %s[%p] ended\n", varname, tr);
}

static void _subtree_dump_gui(const struct Node *node, print_func print_el,
							  FILE *dump, size_t node_id)
{
	if (!node)
		return;

	static char buf[ELEM_BUF_SIZE] = {};
	buf[0] = '\0';
	print_el(buf, node->data, ELEM_BUF_SIZE - 1);
	fprintf(dump, "node%lu [label=\"{%s | {", node_id, buf);
	if (node->left)
		fputs("да", dump);
	else
		fputs("-", dump);
	fputs(" | ", dump);
	if (node->right)
		fputs("нет", dump);
	else
		fputs("-", dump);
	fputs("}}\"]\n", dump);

	_subtree_dump_gui(node->left, print_el, dump, 2 * node_id + 1);
	_subtree_dump_gui(node->right, print_el, dump, 2 * node_id + 2);

	if (node->left)
		fprintf(dump, "node%lu -> node%lu [color=green]\n", node_id, 
				2 * node_id + 1);
	if (node->right)
		fprintf(dump, "node%lu -> node%lu [color=red]\n", node_id,
				2 * node_id + 2);
}
