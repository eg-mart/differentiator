#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "buffer.h"

static enum BufferError buffer_resize(struct Buffer *buf, size_t new_size);
static enum BufferError get_file_size(FILE *file, size_t *size);

enum BufferError buffer_ctor(struct Buffer *buf)
{
	assert(buf);

	enum BufferError err = buffer_resize(buf, BUF_INIT_SIZE);
	if (err < 0)
		return err;
	buffer_reset(buf);
	return err;
}

enum BufferError buffer_load_from_file(struct Buffer *buf, const char *filename)
{
	assert(buf);
	assert(filename);

	FILE *input = fopen(filename, "r");
	if (!input)
		return BUF_FILE_ACCESS_ERR;

	size_t filesize = 0;
	enum BufferError err = get_file_size(input, &filesize);
	if (err < 0)
		return err;

	err = buffer_resize(buf, filesize + 1);
	if (err < 0)
		return err;

	size_t read_chars = fread(buf->data, sizeof(char), buf->size - 1, input);
	if (read_chars < buf->size - 1)
		return BUF_FILE_READ_ERR;
	buffer_reset(buf);

	return BUF_NO_ERR;
}

void buffer_reset(struct Buffer *buf)
{
	assert(buf);

	buf->pos = buf->data;
}

static enum BufferError buffer_resize(struct Buffer *buf, size_t new_size)
{
	char *tmp = (char*) realloc(buf->data, new_size * sizeof(char));
	if (!tmp)
		return BUF_NO_MEM_ERR;
	buf->data = tmp;
	buf->size = new_size;
	memset(buf->data, 0, new_size);
	return BUF_NO_ERR;
}

void buffer_dtor(struct Buffer *buf)
{
	buf->size = 0;
	free(buf->data);
	buf->data = NULL;
	buf->pos = NULL;
}

static enum BufferError get_file_size(FILE *file, size_t *size)
{
	assert(file);
	assert(size);

	int fd = fileno(file);
	struct stat stbuf;
	if (fstat(fd, &stbuf) == -1) return BUF_FILE_ACCESS_ERR;
	*size = (size_t) stbuf.st_size;
	return BUF_NO_ERR;
}