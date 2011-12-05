// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include "osd/base/osd_tag.h"

namespace streaming {

const Tag::Type OsdTag::kType = Tag::TYPE_OSD;

// static
const char* OsdTag::FunctionName(FunctionType type) {
  switch ( type ) {
    case CREATE_OVERLAY:         return "CreateOverlay";
    case DESTROY_OVERLAY:        return "DestroyOverlay";
    case SHOW_OVERLAYS:          return "ShowOverlays";
    case CREATE_CRAWLER:         return "CreateCrawler";
    case DESTROY_CRAWLER:        return "DestroyCrawler";
    case SHOW_CRAWLERS:          return "ShowCrawlers";
    case ADD_CRAWLER_ITEM:       return "AddCrawlerItem";
    case REMOVE_CRAWLER_ITEM:    return "RemoveCrawlerItem";
    case SET_PICTURE_IN_PICTURE: return "SetPictureInPicture";
    case SET_CLICK_URL:          return "SetClickUrl";
    case CREATE_MOVIE:           return "CreateMovie";
    case DESTROY_MOVIE:          return "DestroyMovie";
    default:                     return "Unknown";
  }
}

//////////////////////////////////////////////////////////////////////

// static
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<CreateOverlayParams>() {
  return CREATE_OVERLAY;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<DestroyOverlayParams>() {
  return DESTROY_OVERLAY;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<ShowOverlaysParams>() {
  return SHOW_OVERLAYS;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<CreateCrawlerParams>() {
  return CREATE_CRAWLER;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<DestroyCrawlerParams>() {
  return DESTROY_CRAWLER;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<ShowCrawlersParams>() {
  return SHOW_CRAWLERS;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<AddCrawlerItemParams>() {
  return ADD_CRAWLER_ITEM;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<RemoveCrawlerItemParams>() {
  return REMOVE_CRAWLER_ITEM;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<SetPictureInPictureParams>() {
  return SET_PICTURE_IN_PICTURE;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<SetClickUrlParams>() {
  return SET_CLICK_URL;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<CreateMovieParams>() {
  return CREATE_MOVIE;
}
template <> OsdTag::FunctionType
OsdTag::FunctionTypeCode<DestroyMovieParams>() {
  return DESTROY_MOVIE;
}

//////////////////////////////////////////////////////////////////////
}
