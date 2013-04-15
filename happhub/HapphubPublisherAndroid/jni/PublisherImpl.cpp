#define TAG "HAPPHUB/Publisher"

#include <jni.h>

#include "Publisher.h"
#include "VideoConverter.h"

extern "C" {

#include "libavutil/log.h"
#include "libavformat/avformat.h"

static
void ffmpeg_log(void* p, int level, const char* fmt, va_list ap) {
  if (level == AV_LOG_ERROR) {
    __android_log_vprint(ANDROID_LOG_ERROR, "FFMPEG", fmt, ap);
  } else
  if (level == AV_LOG_WARNING) {
    __android_log_vprint(ANDROID_LOG_WARN, "FFMPEG", fmt, ap);
  } else {
    __android_log_vprint(ANDROID_LOG_VERBOSE, "FFMPEG", fmt, ap);
  }
}

static
JavaVM* __localJM = NULL;

static
void publisher_thread_callback(void* pOpaque, bool started, int result) {
  JNIEnv* env;
  jobject thiz = (jobject)pOpaque;

  if (started) {
    if (__localJM->AttachCurrentThread(&env, NULL) == 0) {
      jclass clazz = env->GetObjectClass(thiz);

      jmethodID callback = env->GetMethodID(clazz, "onNativeStart", "()V");
      if (callback != NULL) {
        env->CallVoidMethod(thiz, callback);
      }
    }
  } else {
    if (__localJM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK) {
      jclass clazz = env->GetObjectClass(thiz);

      jmethodID callback = env->GetMethodID(clazz, "onNativeStop", "(I)V");
      if (callback != NULL) {
        env->CallVoidMethod(thiz, callback, (jint)result);
      }

      // make sure we release the reference to the Java publisher...
      env->DeleteGlobalRef(thiz);
    }
    __localJM->DetachCurrentThread();
  }
}

jint
JNI_OnLoad(JavaVM* vm, void* reserved) {
  av_log_set_callback(ffmpeg_log);

  av_register_all();
  avformat_network_init();

  __localJM = vm;
  return JNI_VERSION_1_4;
}

// Helpers

struct PublisherHolder {
  happhub::Publisher* publisher;
  jobject thiz;

  /*
  jmethodID returnVideoByteBuffer;
  jmethodID returnAudioByteBuffer;
  */

  PublisherHolder(JNIEnv* env, jobject thiz, happhub::Publisher* publisher) {
    this->publisher = publisher;
    this->thiz = env->NewGlobalRef(thiz);

    jclass clazz = env->GetObjectClass(this->thiz);
    /*
    returnVideoByteBuffer = env->GetMethodID(clazz, "returnVideoByteBuffer", "(Ljava/lang/Object;J)V");
    returnAudioByteBuffer = env->GetMethodID(clazz, "returnAudioByteBuffer", "(Ljava/lang/Object;J)V");
    */
  }
  ~PublisherHolder() {
    JNIEnv* env;
    if (__localJM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK) {
      env->DeleteGlobalRef(this->thiz);
    }
  }
};
struct PublisherFrame {
  PublisherHolder* publisher;
  jlong opaque;
  jobject object;

  PublisherFrame(JNIEnv* env, PublisherHolder* publisher, jlong opaque, jobject object) {
    this->publisher = publisher;
    this->opaque = opaque;
    this->object = env->NewGlobalRef(object);
  }
};

static
void videoByteBufferFrameDestructor(happhub::Publisher::FrameType type, void* pOpaque, const void* pData, int nSize, long nTimestamp) {
  PublisherFrame* d = (PublisherFrame*)pOpaque;

  JNIEnv* env;
  if (__localJM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK) {
    //env->CallVoidMethod(d->publisher->thiz, d->publisher->returnVideoByteBuffer, d->object, d->opaque);
    env->DeleteGlobalRef(d->object);
  }
  delete d;
}
static
void audioByteBufferFrameDestructor(happhub::Publisher::FrameType type, void* pOpaque, const void* pData, int nSize, long nTimestamp) {
  PublisherFrame* d = (PublisherFrame*)pOpaque;

  JNIEnv* env;
  if (__localJM->GetEnv((void**)&env, JNI_VERSION_1_4) == JNI_OK) {
    //env->CallVoidMethod(d->publisher->thiz, d->publisher->returnAudioByteBuffer, d->object, d->opaque);
    env->DeleteGlobalRef(d->object);
  }
  delete d;
}

// PublisherHolder.java

jlong
Java_com_happhub_android_media_SoftwarePublisher_publisherCreate(
  JNIEnv* env, jobject thiz,

  jstring url,

  jint videoFormat,
  jint videoInWidth,
  jint videoInHeight,
  jint videoOutWidth,
  jint videoOutHeight,
  jint videoFramerate,
  jint videoBitRate,

  jint audioFormat,
  jint audioInChannels,
  jint audioInSampleRate,
  jint audioOutChannels,
  jint audioOutSampleRate,
  jint audioBitRate,

  jlong maxVideoLag,
  jlong maxAudioLag,

  jstring x264Profile,
  jstring x264Level,
  jstring x264Preset,
  jstring x264Tune
) {
  const char* purl = env->GetStringUTFChars(url, 0) ;

  const char* px264Profile = env->GetStringUTFChars(x264Profile, 0) ;
  const char* px264Level = env->GetStringUTFChars(x264Level, 0) ;
  const char* px264Preset = env->GetStringUTFChars(x264Preset, 0) ;
  const char* px264Tune = env->GetStringUTFChars(x264Tune, 0) ;

  happhub::Publisher* publisher = new happhub::Publisher();
  if (publisher == NULL) {
    LOGE("Couldn't allocate the native publisher");
    return 0;
  }

  if (publisher->create(
    purl,

    (AVPixelFormat)videoFormat,
    videoInWidth,
    videoInHeight,
    videoOutWidth,
    videoOutHeight,
    videoFramerate,
    videoBitRate,

    (AVSampleFormat)audioFormat,
    audioInChannels,
    audioInSampleRate,
    audioOutChannels,
    audioOutSampleRate,
    audioBitRate,

    maxVideoLag,
    maxAudioLag,

    px264Profile,
    px264Level,
    px264Preset,
    px264Tune,

    publisher_thread_callback,
    env->NewGlobalRef(thiz) // will be deleted in the callback, on thread termination
  ) != 0) {
    LOGE("Couldn't setup the native publisher");
    delete publisher;
    return 0;
  }

  PublisherHolder* p = new PublisherHolder(env, thiz, publisher);
  if (p == NULL) {
    publisher->destroy();
    delete publisher;

    return 0;
  }
  return (long)p;
}
void
Java_com_happhub_android_media_SoftwarePublisher_publisherDestroy(
  JNIEnv* env, jobject thiz,
  jlong publisher
) {
  PublisherHolder* p = (PublisherHolder*)publisher;
  if (p != NULL) {
    if (p->publisher != NULL) {
      p->publisher->destroy();
      delete p->publisher;
    }
    delete p;
  }
}

jint
Java_com_happhub_android_media_SoftwarePublisher_publisherPushVideo(
  JNIEnv* env, jobject thiz,
  jlong publisher,
  jlong opaque,
  jobject video,
  jint offset,
  jint size,
  jlong timestamp
) {
  PublisherHolder* p = (PublisherHolder*)publisher;
  if (p == NULL || p->publisher == NULL) {
    return -ENOENT;
  }
  PublisherFrame* d = new PublisherFrame(env, p, opaque, video);
  if (d == NULL) {
    return -ENOMEM;
  }

  unsigned char* pVideo = NULL;
  if (video != NULL) {
    pVideo = (unsigned char*)env->GetDirectBufferAddress(video);
    pVideo += offset;
  }

  int result = p->publisher->pushRawVideo(d, videoByteBufferFrameDestructor, pVideo, size, timestamp, 0);
  if (result != 0) {
    delete d;
  }
  return result;
}
jint
Java_com_happhub_android_media_SoftwarePublisher_publisherPushAudio(
  JNIEnv* env, jobject thiz,
  jlong publisher,
  jlong opaque,
  jobject audio,
  jint offset,
  jint size,
  jlong timestamp
) {
  PublisherHolder* p = (PublisherHolder*)publisher;
  if (p == NULL || p->publisher == NULL) {
    return -ENOENT;
  }
  PublisherFrame* d = new PublisherFrame(env, p, opaque, audio);
  if (d == NULL) {
    return -ENOMEM;
  }

  unsigned char* pAudio = NULL;
  if (audio != NULL) {
    pAudio = (unsigned char*)env->GetDirectBufferAddress(audio);
    pAudio += offset;
  }

  int result = p->publisher->pushRawAudio(d, audioByteBufferFrameDestructor, pAudio, size, timestamp, 0);
  if (result != 0) {
    delete d;
  }
  return result;
}

// HardwarePublisher.java

jlong
Java_com_happhub_android_media_HardwarePublisher_publisherCreate(
  JNIEnv* env, jobject thiz,

  jstring url,

  jint videoFormat,
  jint videoWidth,
  jint videoHeight,
  jint videoFramerate,
  jint videoBitRate,

  jint audioFormat,
  jint audioChannels,
  jint audioSampleRate,
  jint audioBitRate,

  jbyteArray videoHeaders,
  jbyteArray audioHeaders,

  long maxVideoLag,
  long maxAudioLag
) {
  happhub::Publisher* publisher = new happhub::Publisher();
  if (publisher == NULL) {
    LOGE("Couldn't allocate the codec publisher");
    return 0;
  }

  const char* purl = env->GetStringUTFChars(url, 0) ;

  jboolean isCopy;

  jbyte* pVideoHeaders = NULL;
  int nVideoHeadersSize = 0;
  if (videoHeaders) {
    pVideoHeaders = env->GetByteArrayElements(videoHeaders, &isCopy);
    nVideoHeadersSize = env->GetArrayLength(videoHeaders);
  }

  jbyte* pAudioHeaders = NULL;
  int nAudioHeadersSize =0;
  if (audioHeaders) {
    pAudioHeaders = env->GetByteArrayElements(audioHeaders, &isCopy);
    nAudioHeadersSize = env->GetArrayLength(audioHeaders);
  }

  int result =
  publisher->create(
    purl,

    (AVPixelFormat)videoFormat,
    videoWidth,
    videoHeight,
    videoFramerate,
    videoBitRate,

    (AVSampleFormat)audioFormat,
    audioChannels,
    audioSampleRate,
    audioBitRate,

    pVideoHeaders,
    nVideoHeadersSize,
    pAudioHeaders,
    nAudioHeadersSize,

    maxVideoLag,
    maxAudioLag,

    publisher_thread_callback,
    env->NewGlobalRef(thiz) // will be deleted in the callback, on thread termination
  ) ;

  env->ReleaseByteArrayElements(videoHeaders, pVideoHeaders, JNI_ABORT);
  env->ReleaseByteArrayElements(audioHeaders, pAudioHeaders, JNI_ABORT);

  if (result != 0) {
    LOGE("Couldn't setup the codec publisher");
    delete publisher;
    return 0;
  }

  PublisherHolder* p = new PublisherHolder(env, thiz, publisher);
  if (p == NULL) {
    publisher->destroy();
    delete publisher;

    return 0;
  }
  return (long)p;
}
void
Java_com_happhub_android_media_HardwarePublisher_publisherDestroy(
  JNIEnv* env, jobject thiz,
  jlong publisher
) {
  PublisherHolder* p = (PublisherHolder*)publisher;
  if (p != NULL) {
    if (p->publisher != NULL) {
      p->publisher->destroy();
      delete p->publisher;
    }
    delete p;
  }
}

jint
Java_com_happhub_android_media_HardwarePublisher_publisherPushVideo(
  JNIEnv* env, jobject thiz,
  jlong publisher,
  jlong opaque,
  jobject video,
  jint offset,
  jint size,
  jlong timestamp,
  jboolean keyframe
) {
  PublisherHolder* p = (PublisherHolder*)publisher;
  if (p == NULL || p->publisher == NULL) {
    return -ENOENT;
  }
  PublisherFrame* d = new PublisherFrame(env, p, opaque, video);
  if (d == NULL) {
    return -ENOMEM;
  }

  unsigned char* pVideo = NULL;
  if (video != NULL) {
    pVideo = (unsigned char*)env->GetDirectBufferAddress(video);
    pVideo += offset;
  }

  int result = p->publisher->pushEncodedVideo(d, videoByteBufferFrameDestructor, pVideo, size, timestamp, keyframe ? happhub::Publisher::FRAME_KEYFRAME : 0);
  if (result != 0) {
    delete d;
  }
  return result;
}

jint
Java_com_happhub_android_media_HardwarePublisher_publisherPushAudio(
  JNIEnv* env, jobject thiz,
  jlong publisher,
  jlong opaque,
  jobject audio,
  jint offset,
  jint size,
  jlong timestamp
) {
  PublisherHolder* p = (PublisherHolder*)publisher;
  if (p == NULL || p->publisher == NULL) {
    return -ENOENT;
  }
  PublisherFrame* d = new PublisherFrame(env, p, opaque, audio);
  if (d == NULL) {
    return -ENOMEM;
  }

  unsigned char* pAudio = NULL;
  if (audio != NULL) {
    pAudio = (unsigned char*)env->GetDirectBufferAddress(audio);
    pAudio += offset;
  }

  int result = p->publisher->pushEncodedAudio(d, audioByteBufferFrameDestructor, pAudio, size, timestamp, 0);
  if (result != 0) {
    delete d;
  }
  return result;
}

// VideoConverter.java

jlong
Java_com_happhub_android_media_VideoConverter_converterCreate(
  JNIEnv* env, jobject thiz,
  jint inFormat,
  jint inWidth,
  jint inHeight,
  jint outFormat,
  jint outWidth,
  jint outHeight
) {
  happhub::VideoConverter* converter = new happhub::VideoConverter();
  if (converter == NULL) {
    LOGE("Couldn't allocate the native video converter");
    return 0;
  }

  if (converter->create(
    inFormat,
    inWidth,
    inHeight,
    outFormat,
    outWidth,
    outHeight
  ) != 0) {
    LOGE("Couldn't setup the native video converter");
    delete converter;
    return 0;
  }

  return (long)converter;
}

void
Java_com_happhub_android_media_VideoConverter_converterDestroy(
  JNIEnv* env, jobject thiz,
  jlong converter
) {
  happhub::VideoConverter* pconverter = (happhub::VideoConverter*)converter;
  if (pconverter != NULL) {
    pconverter->destroy();
    delete pconverter;
  }
}

jint
Java_com_happhub_android_media_VideoConverter_converterGetInputSize(
  JNIEnv* env, jobject thiz,
  jlong converter
) {
  happhub::VideoConverter* pconverter = (happhub::VideoConverter*)converter;
  if (pconverter == NULL) {
    return -1;
  }
  return pconverter->getInputSize();
}
jint
Java_com_happhub_android_media_VideoConverter_converterGetOutputSize(
  JNIEnv* env, jobject thiz,
  jlong converter
) {
  happhub::VideoConverter* pconverter = (happhub::VideoConverter*)converter;
  if (pconverter == NULL) {
    return -1;
  }
  return pconverter->getOutputSize();
}

jint
Java_com_happhub_android_media_VideoConverter_converterConvert(
  JNIEnv* env, jobject thiz,
  jlong converter,
  jobject in,
  jobject out
) {
  happhub::VideoConverter* pconverter = (happhub::VideoConverter*)converter;
  if (pconverter == NULL) {
    return -1;
  }

  void* pIn = env->GetDirectBufferAddress(in);
  void* pOut = env->GetDirectBufferAddress(out);

  return pconverter->convert(pIn, pOut);
}

}
