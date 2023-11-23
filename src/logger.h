#ifndef LOGGER_MODULE
#define LOGGER_MODULE

#include <stdio.h>

/** An enum representing a logging level (DEBUG is least important, ERROR is most) */
enum Log_level {
	/** The least important logging level for debug messages */
	DEBUG	=	0,
	/** Logging level for information about normal operation of a program */
	INFO	=	1,
	/** Logging level for non-fatal error - warnings - during program execution */
	WARN	=	2,
	/** The most important logging level for fatal errors when program exits */
	ERROR	=	3
};

/** An enum representing errors that may occur during logging */
enum Log_error {
	/** No error occured */
	NO_LOG_ERR	=	0,
	/** A handler couldn't be added to the logger because of a memory error */
	LOG_ERR_MEM		=	-1,
	/** An error happened while writing to a handler */
	ERR_WRITE	=	-2
};

/** A struct representing the logger */
struct Logger {
	/** A number of handlers (files with configuration) currently in the logger */
	size_t num_handlers;
	/** Amount of handlers that logger has memory for */
	size_t capacity;
	/** A pointer to an arrat of handlers */
	struct Log_handler *handlers;
};

/** A struct representing log handler - a file that will receive logs */
struct Log_handler {
	/** Pointer to a file that logs will be written to */
	FILE *output;
	/** The minimal level of logs that should be written to this handler */
	enum Log_level level;
	/** Whether to write escape codes with foreground colors to this handler */ 
	bool use_colors;
};

/** 
* Logger constructot - must be called before any use of the logger
*/
void logger_ctor();

/** 
* Logger destructor - must be called after logger is no longer needed. Doesn't close
* the logger's handlers' files, they must be closed separately.
*/
void logger_dtor();

/**
* Adds a handler to the logger. Can be called anytime after constructor was called
* 
* @param [in] handler a struct representing the log handler
*
* @return error in case an error happened while allocating memory, NO_LOG_ERROR otherwise
*/
enum Log_error add_log_handler(struct Log_handler handler);

/**
* Logs a message with a specified log level, adding a prefix and a color according
* to this level
*
* @param [in] level a log level of this message (from DEBUG to ERROR)
* @param [in] message a pointer to the message format string, composed as for printf
* @param [in] ... values to print according to the message string (as in printf)
*
* @return error in case writing failed in one of the handlers, NO_LOG_ERROR otherwise
*/
enum Log_error log_message(enum Log_level level, const char *message, ...);

/**
* Logs the result of a test with appropriate prefix and colors
*
* @param [in] is_succesful whether the test was succesful or not
* @param [in] num_test the number of the test to be put in the prefix
* @param [in] message a pointer to the message format string, composed as for printf
* @param [in] ... values to print according to the message string (as in printf)
*/
enum Log_error log_test(bool is_succesful, int num_test, const char *message, ...);

enum Log_error log_string(enum Log_level level, const char *message, ...);

#endif