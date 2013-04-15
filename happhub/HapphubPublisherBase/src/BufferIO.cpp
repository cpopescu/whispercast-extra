#include "BufferIO.h"

extern "C" {

#include <libavformat/avformat.h>

int buffer_create(Buffer** buffer) {
  *buffer = (Buffer*)(av_malloc(sizeof(Buffer)));
  if (*buffer == 0) {
    return AVERROR(ENOMEM);
  }
  (*buffer)->size = 16*1024;
  (*buffer)->data = (uint8_t*)av_malloc((*buffer)->size);
  if (*buffer == 0) {
    av_freep(buffer);
    return AVERROR(ENOMEM);
  }
  (*buffer)->length = 0;
  (*buffer)->position = 0;

  return 0;
}

void buffer_destroy(Buffer** buffer) {
  if (*buffer) {
    av_free((*buffer)->data);
  }
  av_freep(buffer);
}

int buffer_read(void* opaque, uint8_t *buf, int size) {
  return -1;
}
int buffer_write(void* opaque, uint8_t *buf, int size) {
  Buffer* buffer = (Buffer*)opaque;

  if ((size > 0) && (size > (buffer->size - buffer->position))) {
    size = buffer->size - buffer->position;
    if (size == 0) {
      return AVERROR(EINVAL);
    }
  }

  memcpy(buffer->data+buffer->position, buf, size);
  buffer->position += size;
  if (buffer->position > buffer->length) {
    buffer->length = buffer->position;
  }

  return size;
}

int64_t buffer_seek(void* opaque, int64_t pos, int whence) {
  Buffer* buffer = (Buffer*)opaque;

  int npos;
  if (whence == SEEK_SET) {
    if ((pos >= 0) && (pos <= buffer->length)) {
      npos = (int)(buffer->position+pos);
    } else {
      return AVERROR(EINVAL);
    }
  } else
  if (whence == SEEK_CUR) {
    int npos = (int)(buffer->position + pos);
    if ((npos < 0) || (npos > buffer->length)) {
      return AVERROR(EINVAL);
    }
  } else {
    return AVERROR(EINVAL);
  }

  buffer->position = npos;
  return buffer->position;
}

}
