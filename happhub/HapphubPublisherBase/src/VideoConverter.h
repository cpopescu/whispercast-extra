#include "Log.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"

}

namespace happhub {

class VideoConverter {
public:
  VideoConverter();
  ~VideoConverter();

public:
  int create(
    int inFormat,
    int inWidth,
    int inHeight,

    int outFormat,
    int outWidth,
    int outHeight
  );
  void destroy();

private:
  void free();

public:
  int getInputSize();
  int getOutputSize();

public:
  int convert(void* in, void* out);

private:
  int m_nInFormat;
  int m_nInWidth;
  int m_nInHeight;

  int m_nOutFormat;
  int m_nOutWidth;
  int m_nOutHeight;

  SwsContext* m_pSwsContext;
};

}
