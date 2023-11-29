#include <string.h>
#include <assert.h>

#include "cmd_args.h"
#include "logger.h"

enum ArgError process_args(const struct ArgDef arg_defs[], size_t arg_defs_size, 
						   const char *argv[], int argc, void *processed_args)
{
	assert(arg_defs);
	assert(argv);

	enum ArgError err = ARG_NO_ERR;
	int i = 1;
	size_t count_required = 0;
	while (i < argc) {
		if (argv[i][0] == '-' && argv[i][1] == '-') {
			for (size_t def_ind = 0; def_ind < arg_defs_size; def_ind++) {
				if (!arg_defs[def_ind].long_name)
					continue;
				
				if (strcmp(argv[i] + 2, arg_defs[def_ind].long_name) == 0) {
					if (!arg_defs[def_ind].is_flag && i >= argc - 1)
						return ARG_MISSING_ERR;

					err = arg_defs[def_ind].handler(argv[i + 1], processed_args);
					if (err < 0)
						return err;

					if (!arg_defs[def_ind].is_optional)
						count_required++;

					if (arg_defs[def_ind].is_flag)
						i += 1;
					else
						i += 2;

					break;
				}
			}
			if (strcmp(argv[i] + 2, "help") == 0) {
				arg_show_usage(arg_defs, arg_defs_size, argv[0]);
				return ARG_HELP_CALLED;
			}
		} else if (argv[i][0] == '-') {
			const char *short_name = argv[i] + 1;
			while (*short_name != '\0') {
				bool processed = false;
				for (size_t def_ind = 0; def_ind < arg_defs_size; def_ind++) {
					if (!arg_defs[def_ind].short_name)
						continue;
					
					if (*short_name == arg_defs[def_ind].short_name) {
						if (!arg_defs[def_ind].is_flag &&
							(*(short_name + 1) != '\0'))
							return ARG_WRONG_POS_ERR;
						if (!arg_defs[def_ind].is_flag &&
							i >= argc - 1)
							return ARG_MISSING_ERR;

						err = arg_defs[def_ind].handler(argv[i + 1], 
														processed_args);
						if (err < 0)
							return err;

						if (!arg_defs[def_ind].is_optional)
							count_required++;
						
						short_name++;
						if (!arg_defs[def_ind].is_flag)
							i += 1;

						processed = true;
						break;
					}
				}
				if (*short_name == 'h') {
					arg_show_usage(arg_defs, arg_defs_size, argv[0]);
					return ARG_HELP_CALLED;
				}
				if (!processed)
					return ARG_WRONG_ARGS_ERR;
			}
			i++;
		} else {
			return ARG_WRONG_ARGS_ERR;
		}
	}

	size_t count_all_required = 0;
	for (size_t def_ind = 0; def_ind < arg_defs_size; def_ind++)
		if (!arg_defs[def_ind].is_optional)
			count_all_required++;

	if (count_all_required != count_required)
		return ARG_MISSING_REQUIRED_ERR;

	return ARG_NO_ERR;
}

const char *arg_err_to_str(enum ArgError err)
{
	switch(err) {
		case ARG_WRONG_ARGS_ERR:
			return "Wrong arguments\n";
		case ARG_WRONG_POS_ERR:
			return "Argument expected after a short flag\n";
		case ARG_MISSING_ERR:
			return "Missing argument after a flag\n";
		case ARG_MISSING_REQUIRED_ERR:
			return "Missing a required flag\n";
		case ARG_NO_ERR:
			return "No error happened\n";
		case ARG_HELP_CALLED:
			return "Help command was called\n";
		default:
			return "Unknow error happened\n";
	}
}

void arg_show_usage(const struct ArgDef arg_defs[], size_t arg_defs_size,
					const char* program_name)
{
	printf("Usage: %s", program_name);
	for (size_t i = 0; i < arg_defs_size; i++) {
		printf(" ");
		if (arg_defs[i].is_optional)
			printf("[");
		else
			printf("(");

		if (arg_defs[i].long_name)
			printf("--%s", arg_defs[i].long_name);
		if (arg_defs[i].long_name && arg_defs[i].short_name)
			printf(" ");
		if (arg_defs[i].short_name)
			printf("-%c", arg_defs[i].short_name);

		if (!arg_defs[i].is_flag)
			printf(" ...");
		if (arg_defs[i].is_optional)
			printf("]");
		else
			printf(")");
	}
	printf("\n\n");
	for (size_t i = 0; i < arg_defs_size; i++) {
		if (arg_defs[i].long_name)
			printf("--%s", arg_defs[i].long_name);
		if (arg_defs[i].short_name)
			printf(" -%c", arg_defs[i].short_name);
		if (arg_defs[i].description)
			printf(" \t%s\n", arg_defs[i].description);
	}
}