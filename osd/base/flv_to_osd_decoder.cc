// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/common/base/common.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include "osd/base/flv_to_osd_decoder.h"

namespace streaming {
FlvToOsdDecoder::FlvToOsdDecoder() {
}
FlvToOsdDecoder::~FlvToOsdDecoder() {
}

scoped_ref<OsdTag> FlvToOsdDecoder::Decode(const FlvTag& flv_tag) {
  if(flv_tag.body().type() != FLV_FRAMETYPE_METADATA) {
    // not a metadata tag
    return NULL;
  }
  const FlvTag::Metadata& metadata = flv_tag.metadata_body();

  ///////////////////////////////////////////////////////////
  // verify tag name, expecting "onCuePoint"
  //
  if ( metadata.name().value() != "onCuePoint" ) {
    // not a cue point
    return NULL;
  }

  ///////////////////////////////////////////////////////////
  // verify parameters map
  //

  // "type" should be "actionscript"
  const rtmp::CObject* type_object = metadata.values().Find("type");
  if ( type_object == NULL ||
       type_object->object_type() != rtmp::CObject::CORE_STRING ||
       static_cast<const rtmp::CString*>(
           type_object)->value() != "actionscript" ) {
    return NULL;
  }

  // "time" should be tag timestamp in seconds
  const rtmp::CObject* time_object = metadata.values().Find("time");
  if( time_object == NULL ||
      time_object->object_type() != rtmp::CObject::CORE_NUMBER ) {
    return NULL;
  }

  // "name" should be just a unique name
  const rtmp::CObject* name_object = metadata.values().Find("name");
  if ( name_object == NULL ||
       name_object->object_type() != rtmp::CObject::CORE_STRING ) {
    return NULL;
  }

  // "parameters" is our mixed map, containing our custom parameters
  const rtmp::CObject* parameters_object = metadata.values().Find("parameters");
  if( parameters_object == NULL ||
      parameters_object->object_type() != rtmp::CObject::CORE_MIXED_MAP ) {
    return NULL;
  }
  const rtmp::CMixedMap* parameters =
    static_cast<const rtmp::CMixedMap*>(parameters_object);

  ///////////////////////////////////////////////////////////
  // decode parameters, part of our encoding convention

  // "target" should be "flash"
  const rtmp::CObject* target_obj = parameters->Find("target");
  if( target_obj == NULL ||
      target_obj->object_type() != rtmp::CObject::CORE_STRING ||
      static_cast<const rtmp::CString*>(target_obj)->value() != "flash" ) {
    return NULL;
  }

  // "function" is the name of the called function
  const rtmp::CObject* function_obj = parameters->Find("function");
  if( function_obj == NULL ||
      function_obj->object_type() != rtmp::CObject::CORE_STRING ) {
    return NULL;
  }
  const string& fname = static_cast<const rtmp::CString*>(
      function_obj)->value();

  // "parameters" is a string containing the encoded parameter for fname
  const rtmp::CObject* params_object = parameters->Find("parameters");
  if( params_object == NULL ||
      params_object->object_type() != rtmp::CObject::CORE_STRING ) {
    return NULL;
  }
  const string& fparams = static_cast<const rtmp::CString*>(
      params_object)->value();

  ///////////////////////////////////////////////////////////////
  // now, decode fparams according to "function" name
  //
  io::MemoryStream tmp;
  tmp.Write(fparams);
  rpc::JsonDecoder decoder;
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::CREATE_OVERLAY) ) {
    CreateOverlayParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode CreateOverlayParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::CREATE_OVERLAY, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::DESTROY_OVERLAY) ) {
    DestroyOverlayParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode DestroyOverlayParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::DESTROY_OVERLAY, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::SHOW_OVERLAYS) ) {
    ShowOverlaysParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode ShowOverlaysParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::SHOW_OVERLAYS, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::CREATE_CRAWLER) ) {
    CreateCrawlerParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode CreateCrawlerParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::CREATE_CRAWLER, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::DESTROY_CRAWLER) ) {
    DestroyCrawlerParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode DestroyCrawlerParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::DESTROY_CRAWLER, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::SHOW_CRAWLERS) ) {
    ShowCrawlersParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode ShowCrawlersParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::SHOW_CRAWLERS, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::ADD_CRAWLER_ITEM) ) {
    AddCrawlerItemParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode AddCrawlerItemParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::ADD_CRAWLER_ITEM, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::REMOVE_CRAWLER_ITEM) ) {
    RemoveCrawlerItemParams params;
    if ( rpc::DECODE_RESULT_SUCCESS !=  decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode RemoveCralerItemParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::REMOVE_CRAWLER_ITEM, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::SET_PICTURE_IN_PICTURE) ) {
    SetPictureInPictureParams params;
    if ( rpc::DECODE_RESULT_SUCCESS !=  decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode SetPictureInPicture: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::SET_PICTURE_IN_PICTURE, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::SET_CLICK_URL) ) {
    SetClickUrlParams params;
    if ( rpc::DECODE_RESULT_SUCCESS !=  decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode SetClickUrl: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::SET_CLICK_URL, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::CREATE_MOVIE) ) {
    CreateMovieParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode CreateMovieParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::CREATE_MOVIE, params);
  }
  if ( fname == streaming::OsdTag::FunctionName(
                streaming::OsdTag::DESTROY_MOVIE) ) {
    DestroyMovieParams params;
    if ( rpc::DECODE_RESULT_SUCCESS != decoder.Decode(tmp, &params) ) {
      LOG_ERROR << "Failed to decode DestroyMovieParams: " << params
                << "from string: [" << fparams << "]";
      return NULL;
    }
    return new streaming::OsdTag(0,streaming::kDefaultFlavourMask,
        streaming::OsdTag::DESTROY_MOVIE, params);
  }
  LOG_ERROR << "Unrecognized OSD function: [" << fname << "]";
  return NULL;
}
}
