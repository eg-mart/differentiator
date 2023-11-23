#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "colors.h"
#include "logger.h"

struct Logger LOGGER;

enum Log_error do_log(enum Log_level level, const char *prefix, const char *color,
					  const char *message, va_list args);

void logger_ctor()
{
	LOGGER.handlers = NULL;
	LOGGER.num_handlers = 0;
}

enum Log_error add_log_handler(struct Log_handler handler)
{
	if (LOGGER.handlers == NULL) {
		LOGGER.handlers = (Log_handler*) calloc(3, sizeof(Log_handler));
		if (LOGGER.handlers == NULL) {
			return LOG_ERR_MEM;
		}
		LOGGER.capacity = 3;
	} else if (LOGGER.capacity <= LOGGER.num_handlers) {
		LOGGER.capacity += 3;
		LOGGER.handlers = (Log_handler*) realloc(LOGGER.handlers, LOGGER.capacity *
												 sizeof(Log_handler));
		if (LOGGER.handlers == NULL)
			return LOG_ERR_MEM;
	}

	LOGGER.handlers[LOGGER.num_handlers] = handler;

	fprintf(LOGGER.handlers[LOGGER.num_handlers].output,
			"\tSTART OF LOG\n-----------------------------\n\n");
	LOGGER.num_handlers++;

	return NO_LOG_ERR;
}

void logger_dtor()
{
	for (size_t i = 0; i < LOGGER.num_handlers; i++)
		fprintf(LOGGER.handlers[i].output,
				"\n-----------------------------\n\tEND OF LOG\n");
	
	free(LOGGER.handlers);
}

enum Log_error do_log(enum Log_level level, const char *prefix, const char *color,
					  const char *message, va_list args)
{
	assert(message != NULL);
	assert(prefix != NULL);
	assert(color != NULL);

	const size_t BUFF_SIZE = 1024;
	char buff[BUFF_SIZE] = "";
	
	int written = vsnprintf(buff, BUFF_SIZE, message, args);
	
	bool error = 0;
	for (size_t i = 0; i < LOGGER.num_handlers; i++) {
		if (level < LOGGER.handlers[i].level)
			continue;
		
		if (LOGGER.handlers[i].use_colors)
			fprintf(LOGGER.handlers[i].output, "%s%s%s%s",
					color, prefix, RESET_COLOR, buff);
		else
			fprintf(LOGGER.handlers[i].output, "%s%s", prefix, buff);

		error = error || ferror(LOGGER.handlers[i].output);
	}

	if (written > (int) BUFF_SIZE - 1)
		if (log_string(level, "...message was truncated\n") < 0)
			return ERR_WRITE;
	
	if (error)
		return ERR_WRITE;
	return NO_LOG_ERR;
}

enum Log_error log_string(enum Log_level level, const char *message, ...)
{
	assert(message != NULL);

	va_list args;
	va_start(args, message);

	return do_log(level, "", RESET_COLOR, message, args);
}

enum Log_error log_message(enum Log_level level, const char *message, ...)
{
	assert(message != NULL);

	va_list args;
	va_start(args, message);

	enum Log_error status = NO_LOG_ERR;

	switch (level) {
		case ERROR:
			status = do_log(level, "[ERROR] ", RED, message, args);
			break;
		case WARN:
			status = do_log(level, "[WARNING] ", YELLOW, message, args);
			break;
		case INFO:
			status = do_log(level, "[INFO] ", BLUE, message, args);
			break;
		case DEBUG:
			status = do_log(level, "[DEBUG] ", GREEN, message, args);
			break;
		default:
			status = do_log(level, "[UNKNOWN] ", RED, message, args);
	}

	va_end(args);

	return status;
}

enum Log_error log_test(bool is_succesful, int num_test, const char *message, ...) {
	assert(message != NULL);
	
	va_list args;
	va_start(args, message);

	enum Log_error status = NO_LOG_ERR;
	const size_t PREF_SIZE = 256;
	char prefix[PREF_SIZE] = "";

	if (is_succesful) {
		sprintf(prefix, "test %d OK", num_test);
		status = do_log(ERROR, prefix, GREEN, message, args);
	} else {
		sprintf(prefix, "test %d FAILED: ", num_test);
		status = do_log(ERROR, prefix, RED, message, args);
	}

	return status;
}
