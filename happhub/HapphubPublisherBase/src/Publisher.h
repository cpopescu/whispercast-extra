#include "Log.h"

extern "C" {

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"

}

#ifdef __WINDOWS
#include <Windows.h>
#endif // __WINDOWS

#include <list>
#include "Threads.h"

namespace happhub {

class Publisher {
public:
  Publisher();
  ~Publisher();

public:
  typedef
  void (*ThreadCallback)(void* pOpaque, bool started, int result);

private:
  int create(
    const char* url,
    AVPixelFormat videoFormat,
    int videoInWidth,
    int videoInHeight,
    int videoOutWidth,
    int videoOutHeight,
    int videoFrameRate,
    int videoBitRate,

    AVSampleFormat audioFormat,
    int audioInChannels,
    int audioInSampleRate,
    int audioOutChannels,
    int audioOutSampleRate,
    int audioBitRate,

    long maxVideoLag,
    long maxAudioLag,

    const char* szX264Profile,
    const char* szX264Level,
    const char* szX264Preset,
    const char* szX264Tune,

    bool createEncoders,

    ThreadCallback threadCallback,
    void* threadCallbackArg
  );

public:
  // creates encoders too
  int create(
    const char* url,

    AVPixelFormat videoFormat,
    int videoInWidth,
    int videoInHeight,
    int videoOutWidth,
    int videoOutHeight,
    int videoFramerate,
    int videoBitRate,

    AVSampleFormat audioFormat,
    int audioInChannels,
    int audioInSampleRate,
    int audioOutChannels,
    int audioOutSampleRate,
    int audioBitRate,

    long maxVideoLag,
    long maxAudioLag,

    const char* szX264Profile,
    const char* szX264Level,
    const char* szX264Preset,
    const char* szX264Tune,

    ThreadCallback threadCallback = NULL,
    void* threadCallbackArg = NULL
  );
  // creates only the muxer
  int create(
    const char* url,
    AVPixelFormat videoFormat,
    int videoOutWidth,
    int videoOutHeight,
    int videoFramerate,
    int videoBitRate,

    AVSampleFormat audioFormat,
    int audioOutChannels,
    int audioOutSampleRate,
    int audioBitRate,

    const void* videoHeaders,
    int videoHeadersLength,
    const void* audioHeaders,
    int audioHeadersLength,

    long maxVideoLag,
    long maxAudioLag,

    ThreadCallback threadCallback = NULL,
    void* threadCallbackArg = NULL
  );

  void destroy();

private:
  void free();

private:
    int run();

public:
  enum FrameType {
    RAW_AUDIO,
    RAW_VIDEO,
    ENCODED_AUDIO,
    ENCODED_VIDEO,
    PACKET_AUDIO,
    PACKET_VIDEO
  };

  static const unsigned int FRAME_KEYFRAME = 0x0001;
  static const unsigned int FRAME_FLIP_Y = 0x0002;
  static const unsigned int FRAME_FLIP_X = 0x0004;

  typedef
  void (*FrameDestructor)(FrameType type, void* pOpaque, const void* pData, int nSize, long nTimestamp);

private:
  struct Frame {
    FrameType m_Type;

    void* m_pOpaque;

    const void* m_pData;
    int m_nSize;

    long m_nTimestamp;
    unsigned int m_nFlags;

    FrameDestructor m_Destructor;

    Frame(FrameType type, void* pOpaque, const void* pData, int nSize, long nTimestamp, unsigned int nFlags, FrameDestructor destructor) :
      m_Type(type),
      m_pOpaque(pOpaque),
      m_pData(pData),
      m_nSize(nSize),
      m_nTimestamp(nTimestamp),
      m_nFlags(nFlags), 
      m_Destructor(destructor) {
    }
    ~Frame() {
      if (m_Destructor) {
        m_Destructor(m_Type, m_pOpaque, m_pData, m_nSize, m_nTimestamp);
      }
    }
  };

private:
  int processQueueFrame(Frame* frame);

private:
  static
  void* thread_entry_point(void* arg);
  void* thread_entry_point();

private:
  int outputVideo(AVPacket* video);
  int outputAudio(AVPacket* audio);

  int encodeAndPushVideo(AVFrame* video, int* gotPacket);
  int encodeAndPushAudio(AVFrame* audio, int* gotPacket);

public:
  int pushRawAudio(void* opaque, FrameDestructor destructor, const void* audio, int size, long timestamp, unsigned int flags);
  int pushRawVideo(void* opaque, FrameDestructor destructor, const void* video, int size, long timestamp, unsigned int flags);

  int pushEncodedAudio(void* opaque, FrameDestructor destructor, const void* audio, int size, long timestamp, unsigned int flags) {
    LOGD("Publisher AUDIO: Pushing encoded at %ld - %s", timestamp, (flags & FRAME_KEYFRAME) ? "KEYFRAME" : "");
    return pushFrame(ENCODED_AUDIO, opaque, destructor, audio, size, timestamp, flags | FRAME_KEYFRAME);
  }
  int pushEncodedVideo(void* opaque, FrameDestructor destructor, const void* video, int size, long timestamp, unsigned int flags) {
    LOGD("Publisher VIDEO: Pushing encoded at %ld - %s", timestamp, (flags & FRAME_KEYFRAME) ? "KEYFRAME" : "");
    return pushFrame(ENCODED_VIDEO, opaque, destructor, video, size, timestamp, flags);
  }

private:
  int pushPacketAudio(void* opaque, FrameDestructor destructor, const void* audio, int size, long timestamp, unsigned int flags) {
    LOGD("Publisher AUDIO: Pushing packet at %ld - %s", timestamp, (flags & FRAME_KEYFRAME) ? "KEYFRAME" : "");
    return pushFrame(PACKET_AUDIO, opaque, destructor, audio, size, timestamp, flags | FRAME_KEYFRAME);
  }
  int pushPacketVideo(void* opaque, FrameDestructor destructor, const void* audio, int size, long timestamp, unsigned int flags) {
    LOGD("Publisher VIDEO: Pushing packet %ld - %s", timestamp, (flags & FRAME_KEYFRAME) ? "KEYFRAME" : "");
    return pushFrame(PACKET_VIDEO, opaque, destructor, audio, size, timestamp, flags | FRAME_KEYFRAME);
  }

private:
  int pushFrame(FrameType type, void* opaque, FrameDestructor destructor, const void* data, int size, long timestamp, unsigned int flags);

private:
  int processEncodedVideo(Frame* frame);
  int processEncodedAudio(Frame* frame);

  int processPacketVideo(Frame* frame);
  int processPacketAudio(Frame* frame);

  int processFrame(Frame* frame) {
    int result = 0;

    switch (frame->m_Type) {
      case ENCODED_VIDEO:
        result = processEncodedVideo(frame);
        break;
      case ENCODED_AUDIO:
        result = processEncodedAudio(frame);
        break;
      case PACKET_VIDEO:
        result = processPacketVideo(frame);
        break;
      case PACKET_AUDIO:
        result = processPacketAudio(frame);
        break;
      default:
        result = -1;
    }

    m_QueueMutex.unlock();
    delete frame;
    m_QueueMutex.lock();

    return result;
  }

private:
  static
  int avioInterruptCB(void* opaque);

private:
  const char* m_szUrl;
  AVPixelFormat m_nVideoFormat;
  int m_nVideoInWidth;
  int m_nVideoInHeight;
  int m_nVideoOutWidth;
  int m_nVideoOutHeight;
  int m_nVideoFramerate;
  int m_nVideoBitRate;

  AVSampleFormat m_nAudioFormat;
  int m_nAudioInChannels;
  int m_nAudioInSampleRate;
  int m_nAudioOutChannels;
  int m_nAudioOutSampleRate;
  int m_nAudioBitRate;

  long m_nMaxVideoLag;
  long m_nMaxAudioLag;

  bool m_bHasEncoders;

  ThreadCallback m_pThreadCallback;
  void* m_pThreadCallbackArg;

  AVFormatContext* m_pOutputContext;
  AVStream* m_pVideoStream;
  AVStream* m_pAudioStream;
  AVFrame* m_pAudioFrame;
  AVFrame* m_pVideoFrame;

  AVPicture m_SwsPicture;
  SwsContext* m_pSwsContext;

  SwrContext* m_pSwrContext;
  unsigned char* m_pAudioBuffer[SWR_CH_MAX];
  int m_nAudioBufferSamples;

  bool m_bStop;
  bool m_bStopped;
  AVIOInterruptCB m_sInterruptedCB;

  Thread m_Thread;

  Mutex m_QueueMutex;
  Condition m_QueueCondition;

  std::list<Frame*> m_Queue;

  Mutex m_AudioMutex;
  Mutex m_VideoMutex;

  bool m_bNeedsVideoKeyframe;
  bool m_bNeedsAudioKeyframe;

  long m_nLastPushedTimestamp;
};

}
