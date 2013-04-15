#include "Log.h"

extern "C" {

#include "libavcodec/avcodec.h"

typedef struct {
  uint8_t* data;
  int size;
  int length;
  int position;
} Buffer;

int buffer_create(Buffer** buffer);
void buffer_destroy(Buffer** buffer);

int buffer_read(void* opaque, uint8_t *buf, int size);
int buffer_write(void* opaque, uint8_t *buf, int size);

int64_t buffer_seek(void* opaque, int64_t pos, int whence);

}
