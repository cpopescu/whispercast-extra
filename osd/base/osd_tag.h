// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#ifndef __MEDIA_OSD_OSD_TAG_H__
#define __MEDIA_OSD_OSD_TAG_H__

#include <string>
#include <whisperstreamlib/base/tag.h>
#include "osd/base/auto/osd_types.h"

namespace streaming {

class OsdTag : public Tag {
 public:
  // Identifies the OSD function.
  enum FunctionType {
    CREATE_OVERLAY        = 0,
    DESTROY_OVERLAY,
    SHOW_OVERLAYS,
    CREATE_CRAWLER,
    DESTROY_CRAWLER,
    SHOW_CRAWLERS,
    ADD_CRAWLER_ITEM,
    REMOVE_CRAWLER_ITEM,
    SET_PICTURE_IN_PICTURE,
    SET_CLICK_URL,
    CREATE_MOVIE,
    DESTROY_MOVIE
  };
  static const char* FunctionName(FunctionType type);

  // template specialized by OSD function params.
  // e.g. if T == CreateInfoParams, returns: CREATE_INFO.
  template <typename T>
  static FunctionType FunctionTypeCode();

 public:
  static const Type kType;
  OsdTag(uint32 attributes,
         uint32 flavour_mask,
         int64 timestamp_ms,
         FunctionType ftype,
         const rpc::Custom& fparams)
      : Tag(kType, attributes, flavour_mask),
        timestamp_ms_(timestamp_ms),
        ftype_(ftype),
        fparams_(fparams.Clone()) {
  }

  template <typename T>
  OsdTag(uint32 attributes,
         uint32 flavour_mask,
         int64 timestamp_ms,
         const T& fparams)
      : Tag(kType, attributes, flavour_mask),
        timestamp_ms_(timestamp_ms),
        ftype_(FunctionTypeCode<T>()),
        fparams_(fparams.Clone()) {
  }
  OsdTag(const OsdTag& other, int64 timestamp_ms)
      : Tag(other),
        timestamp_ms_(timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms_),
        ftype_(other.ftype_),
        fparams_(other.fparams_->Clone()) {
  }
  virtual ~OsdTag() {
    delete fparams_;
  }

  // returns the osd function name. This is used for both logging and
  // encoding the OSD tag in protocol specific format.
  const char* fname() const            { return FunctionName(ftype()); }
  // returns the osd function type code
  FunctionType ftype() const           { return ftype_; }
  // returns the osd function parameters
  const rpc::Custom& fparams() const   { return  *fparams_; }

  void set_timestamp_ms(int64 timestamp_ms) { timestamp_ms_ = timestamp_ms; }

  ///////////////////////////////////////////
  //
  // Tag interface methods
  //
  virtual int64 timestamp_ms() const { return timestamp_ms_; }
  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return 0; }
  virtual const char* ContentType() {
    return "Osd";
  }
  virtual void AppendToStream(io::MemoryStream* out) const {
  }
  virtual string ToStringBody() const {
    return strutil::StringPrintf("OsdTag{ftype=%d(%s), fparams=%s}",
                                 static_cast<int>(ftype()), fname(),
                                 fparams().ToString().c_str());
  }
  virtual Tag* Clone(int64 timestamp_ms) const {
    return new OsdTag(*this, timestamp_ms);
  }

 protected:
  int64 timestamp_ms_;
  const FunctionType ftype_;
  const rpc::Custom* const fparams_;
};
std::ostream& operator<<(std::ostream& os, const OsdTag& osd);
}
#endif   // __MEDIA_OSD_OSD_TAG_H__
