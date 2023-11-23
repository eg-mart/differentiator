#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdio.h>

struct Buffer {
	char *data;
	char *pos;
	size_t size;
};

enum BufferError {
	BUF_FILE_ACCESS_ERR = -3,
	BUF_FILE_READ_ERR	= -2,
	BUF_NO_MEM_ERR		= -1,
	BUF_NO_ERR			= 0,
};

const size_t BUF_INIT_SIZE = 2048;

enum BufferError buffer_ctor(struct Buffer *buf);
enum BufferError buffer_load_from_file(struct Buffer *buf, const char *filename);
void buffer_reset(struct Buffer *buf);
void buffer_dtor(struct Buffer *buf);

#endif /*_BUFFER_H*/
