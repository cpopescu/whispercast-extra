// Copyright 2009 WhisperSoft s.r.l.
// Author: Cosmin Tudorache

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>

#include "osd/osd_library/osd_state_keeper/osd_state_keeper_element.h"

namespace {
static const char kStateName[] = "state";

class OsdStateKeeperCallbackData : public streaming::FilteringCallbackData  {
 public:
  OsdStateKeeperCallbackData(streaming::OsdStateKeeperElement* element)
      : streaming::FilteringCallbackData(),
        element_(element),
        bootstrap_played_(false) {
    CHECK_NOT_NULL(element);
  }
  virtual ~OsdStateKeeperCallbackData() {
  }
  /////////////////////////////////////////////////////////////////////
  //
  // FilteringCallbackData methods
  //
  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out) {
    // Always forward the incoming original tag.
    // Some OSD tags may be appended to 'out'.
    out->push_back(FilteredTag(tag, timestamp_ms));

    CHECK_NOT_NULL(element_);
    if ( bootstrap_played_ || tag->type() == streaming::Tag::TYPE_EOS ) {
      return;
    }
    bootstrap_played_ = true;
    element_->AppendBootstrap(out, flavour_mask(), timestamp_ms);
  }
 private:
  streaming::OsdStateKeeperElement* element_;
  bool bootstrap_played_;

  DISALLOW_EVIL_CONSTRUCTORS(OsdStateKeeperCallbackData);
};
}

namespace streaming {

/////////////////////////////////////////////////////////////////////

template <typename CREATE_X_PARAMS>
map<string, CREATE_X_PARAMS> MakeRPCMap(
  const map<string, CREATE_X_PARAMS*>& m) {
  map<string, CREATE_X_PARAMS> creators;
  for ( typename map<string, CREATE_X_PARAMS*>::const_iterator it = m.begin();
        it != m.end(); ++it ) {
    const CREATE_X_PARAMS& x = *it->second;
    creators[it->first] = x;
  }
  return creators;
}
/////////////////////////////////////////////////////////////////////

const char OsdStateKeeperElement::kElementClassName[] = "osd_state_keeper";

OsdStateKeeperElement::OsdStateKeeperElement(const char* name,
                                             const char* id,
                                             ElementMapper* mapper,
                                             net::Selector* selector,
                                             io::StateKeepUser* state_keeper,
                                             const char* media_name,
                                             const char* rpc_path,
                                             rpc::HttpServer* rpc_server)

    : FilteringElement(kElementClassName, name, id, mapper, selector),
      ServiceInvokerOsdInspector(ServiceInvokerOsdInspector::GetClassName()),
      media_name_(media_name),
      internal_req_(NULL),
      process_tag_callback_(NewPermanentCallback(this,
          &OsdStateKeeperElement::ProcessTag)),
      overlays_(),
      show_overlays_(true),
      crawlers_(),
      show_crawlers_(true),
      pip_(),
      movies_(),
      state_keeper_(state_keeper),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      is_registered_(false),
      open_media_alarm_(*selector) {
  open_media_alarm_.Set(NewPermanentCallback(this,
      &OsdStateKeeperElement::OpenMedia), true, 5000, false, false);
}

OsdStateKeeperElement::~OsdStateKeeperElement() {
  if ( is_registered_ ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }
  CloseMedia();
  CHECK_NULL(internal_req_);

  delete process_tag_callback_;
  process_tag_callback_ = NULL;

  delete state_keeper_;
}

bool OsdStateKeeperElement::Initialize() {
  // TODO(cosmin): This is really ugly, should we do something about
  // all the copying down here ?

  // load the state
  string state;
  if (state_keeper_ != NULL && state_keeper_->GetValue(kStateName, &state)) {
    LOG_DEBUG << "Loading OSD state from string: [" << state << "]";

    io::MemoryStream iomis;
    iomis.Write(state);
    rpc::JsonDecoder decoder(iomis);

    {
      map<string, CreateOverlayParams> overlays;
      const rpc::DECODE_RESULT result = decoder.Decode(overlays);
      if (result != rpc::DECODE_RESULT_SUCCESS) {
        LOG_ERROR << "Error decoding the overlays state from the state keeper.";
      } else {
        map<string, CreateOverlayParams>::const_iterator i;
        for (i = overlays.begin(); i != overlays.end(); ++i) {
          overlays_[i->first] =
            static_cast<CreateOverlayParams*>(i->second.Clone());
        }
      }
    }
    {
      map<string, CreateCrawlerParams> crawlers;
      const rpc::DECODE_RESULT result = decoder.Decode(crawlers);
      if (result != rpc::DECODE_RESULT_SUCCESS) {
        LOG_ERROR << "Error decoding the crawlers state from the state keeper.";
      } else {
        map<string, CreateCrawlerParams>::const_iterator i;
        for (i = crawlers.begin(); i != crawlers.end(); ++i) {
          crawlers_[i->first] =
            static_cast<CreateCrawlerParams*>(i->second.Clone());
        }
      }
    }
    {
      ShowOverlaysParams show_overlays;
      const rpc::DECODE_RESULT result = decoder.Decode(show_overlays);
      if (result != rpc::DECODE_RESULT_SUCCESS) {
        LOG_ERROR <<
            "Error decoding the overlays show state from the state keeper..";
      } else {
        show_overlays_ = show_overlays;
      }
    }
    {
      ShowCrawlersParams show_crawlers;
      const rpc::DECODE_RESULT result = decoder.Decode(show_crawlers);
      if (result != rpc::DECODE_RESULT_SUCCESS) {
        LOG_ERROR <<
          "Error decoding the crawlers show state from the state keeper..";
      } else {
        show_crawlers_ = show_crawlers;
      }
    }
    {
      SetPictureInPictureParams pip;
      const rpc::DECODE_RESULT result = decoder.Decode(pip);
      if (result != rpc::DECODE_RESULT_SUCCESS) {
        LOG_ERROR <<
          "Error decoding the PIP state from the state keeper..";
      } else {
        pip_ = pip;
      }
    }
    {
      SetClickUrlParams click_url;
      const rpc::DECODE_RESULT result = decoder.Decode(click_url);
      if (result != rpc::DECODE_RESULT_SUCCESS) {
        LOG_ERROR <<
          "Error decoding the click URL state from the state keeper..";
      } else {
        click_url_ = click_url;
      }
    }
    {
      map<string, CreateMovieParams> movies;
      const rpc::DECODE_RESULT result = decoder.Decode(movies);
      if (result != rpc::DECODE_RESULT_SUCCESS) {
        LOG_ERROR << "Error decoding the movies state from the state keeper.";
      } else {
        map<string, CreateMovieParams>::const_iterator i;
        for (i = movies.begin(); i != movies.end(); ++i) {
          movies_[i->first] =
            static_cast<CreateMovieParams*>(i->second.Clone());
        }
      }
    }
  }
  OpenMedia();
  if ( rpc_server_ != NULL ) {
    is_registered_ = rpc_server_->RegisterService(rpc_path_, this);
  } else {
    LOG_WARNING << "No RPC server for osd state keeper: " << name();
  }
  return is_initialized();
}

void OsdStateKeeperElement::Close(Closure* call_on_close) {
  FilteringElement::Close(call_on_close);
  CloseMedia();
}

void OsdStateKeeperElement::OpenMedia() {
  CHECK(internal_req_ == NULL);
  internal_req_ = new streaming::Request();
  internal_req_->mutable_info()->internal_id_ = id();
  if ( !mapper_->AddRequest(media_name_.c_str(),
                            internal_req_,
                            process_tag_callback_) ) {
    LOG_ERROR << name() << " faild to add itself to: " << media_name_;
    delete internal_req_;
    internal_req_ = NULL;
    open_media_alarm_.Start();
  }
}
void OsdStateKeeperElement::CloseMedia() {
  if ( internal_req_ != NULL ) {
    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    internal_req_ = NULL;
  }
}
bool OsdStateKeeperElement::SaveState() {
  if ( state_keeper_ == NULL ) {
    return true;
  }
  io::MemoryStream mos;
  rpc::JsonEncoder encoder(mos);
  encoder.Encode(MakeRPCMap(overlays_));
  encoder.Encode(MakeRPCMap(crawlers_));
  encoder.Encode(show_overlays_);
  encoder.Encode(show_crawlers_);
  encoder.Encode(pip_);
  encoder.Encode(MakeRPCMap(movies_));

  string s;
  mos.ReadString(&s);
  return state_keeper_->SetValue(kStateName, s);
}

void OsdStateKeeperElement::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    CHECK_NOT_NULL(process_tag_callback_);
    CHECK_NOT_NULL(internal_req_);

    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    internal_req_ = NULL;

    delete process_tag_callback_;
    process_tag_callback_ = NULL;
    return;
  }
  if ( tag->type() != streaming::Tag::TYPE_OSD ) {
    return;
  }
  const OsdTag* data = static_cast<const OsdTag*>(tag);
  OsdTag::FunctionType ftype = data->ftype();

  bool processed = true;
  switch ( ftype ) {
    case OsdTag::CREATE_OVERLAY:
      ProcessOsd(static_cast<const CreateOverlayParams&>(data->fparams()));
      break;
    case OsdTag::DESTROY_OVERLAY:
      ProcessOsd(static_cast<const DestroyOverlayParams&>(data->fparams()));
      break;
    case OsdTag::SHOW_OVERLAYS:
      ProcessOsd(static_cast<const ShowOverlaysParams&>(data->fparams()));
      break;
    case OsdTag::CREATE_CRAWLER:
      ProcessOsd(static_cast<const CreateCrawlerParams&>(data->fparams()));
      break;
    case OsdTag::DESTROY_CRAWLER:
      ProcessOsd(static_cast<const DestroyCrawlerParams&>(data->fparams()));
      break;
    case OsdTag::SHOW_CRAWLERS:
      ProcessOsd(static_cast<const ShowCrawlersParams&>(data->fparams()));
      break;
    case OsdTag::ADD_CRAWLER_ITEM:
      ProcessOsd(static_cast<const AddCrawlerItemParams&>(data->fparams()));
      break;
    case OsdTag::REMOVE_CRAWLER_ITEM:
      ProcessOsd(static_cast<const RemoveCrawlerItemParams&>(data->fparams()));
      break;
    case OsdTag::SET_PICTURE_IN_PICTURE:
      ProcessOsd(
          static_cast<const SetPictureInPictureParams &>(data->fparams()));
      break;
    case OsdTag::SET_CLICK_URL:
      ProcessOsd(
          static_cast<const SetClickUrlParams &>(data->fparams()));
      break;
    case OsdTag::CREATE_MOVIE:
      ProcessOsd(static_cast<const CreateMovieParams&>(data->fparams()));
      break;
    case OsdTag::DESTROY_MOVIE:
      ProcessOsd(static_cast<const DestroyMovieParams&>(data->fparams()));
      break;
    default:
      processed = false;
      LOG_ERROR << "Bad osd tag, with ftype: " << ftype;
  }

  if (processed) {
    LOG_DEBUG << "OSD state updated: " << tag->ToString();
    SaveState();
  }
}

void OsdStateKeeperElement::ProcessOsd(const CreateOverlayParams& fparams) {
  const string& id = fparams.id;
  map<string, CreateOverlayParams*>::iterator it = overlays_.find(id);
  if ( it != overlays_.end() ) {
    CreateOverlayParams *pparams = it->second;
    if (fparams.content.is_set())
        pparams->content = fparams.content;

    if (fparams.fore_color.is_set())
        pparams->fore_color = fparams.fore_color;
    if (fparams.back_color.is_set())
        pparams->back_color = fparams.back_color;
    if (fparams.link_color.is_set())
        pparams->link_color = fparams.link_color;
    if (fparams.hover_color.is_set())
        pparams->hover_color = fparams.hover_color;

    if (fparams.x_location.is_set())
        pparams->x_location = fparams.x_location;
    if (fparams.y_location.is_set())
        pparams->y_location = fparams.y_location;

    if (fparams.width.is_set())
        pparams->width = fparams.width;

    if (fparams.margins.is_set())
        pparams->margins = fparams.margins;

    if (fparams.ttl.is_set())
        pparams->ttl = fparams.ttl;

    if (fparams.show.is_set())
        pparams->show = fparams.show;
    LOG_DEBUG << "CreateOverlay id: " << id
              << ", updated with fparams: " << fparams;
    return;
  }
  LOG_DEBUG << "CreateOverlay id: " << id
            << ", created with fparams: " << fparams;
  overlays_.insert(make_pair(id, new CreateOverlayParams(fparams)));
}
void OsdStateKeeperElement::ProcessOsd(const DestroyOverlayParams& fparams) {
  const string& id = fparams.id;
  map<string, CreateOverlayParams*>::iterator it = overlays_.find(id);
  if ( it == overlays_.end() ) {
    LOG_ERROR << "DestroyOverlay unknown id: " << id
              << ", fparams: " << fparams;
    return;
  }
  LOG_DEBUG << "DestroyOverlay id: " << id
            << ", fparams: " << fparams;
  delete it->second;
  overlays_.erase(it);
}
void OsdStateKeeperElement::ProcessOsd(const ShowOverlaysParams& fparams) {
  LOG_DEBUG << "ShowOverlays, fparams: " << fparams;
  show_overlays_.show = fparams.show;
}
void OsdStateKeeperElement::ProcessOsd(const CreateCrawlerParams& fparams) {
  const string& id = fparams.id;
  map<string, CreateCrawlerParams*>::iterator it = crawlers_.find(id);
  if ( it != crawlers_.end() ) {
    CreateCrawlerParams *pparams = it->second;

    if (fparams.speed.is_set())
        pparams->speed = fparams.speed;

    if (fparams.fore_color.is_set())
        pparams->fore_color = fparams.fore_color;
    if (fparams.back_color.is_set())
        pparams->back_color = fparams.back_color;
    if (fparams.link_color.is_set())
        pparams->link_color = fparams.link_color;
    if (fparams.hover_color.is_set())
        pparams->hover_color = fparams.hover_color;

    if (fparams.y_location.is_set())
        pparams->y_location = fparams.y_location;

    if (fparams.margins.is_set())
        pparams->margins = fparams.margins;

    if (fparams.ttl.is_set())
        pparams->ttl = fparams.ttl;

    if (fparams.show.is_set())
        pparams->show = fparams.show;

    if (fparams.items.is_set()) {
      pparams->items = fparams.items;
    }
    LOG_DEBUG << "CreateCrawler id: " << id
              << ", updated with fparams: " << fparams;
    return;
  }
  LOG_DEBUG << "CreateCrawler id: " << id
            << ", created with fparams: " << fparams;
  crawlers_.insert(make_pair(id, new CreateCrawlerParams(fparams)));
}
void OsdStateKeeperElement::ProcessOsd(const DestroyCrawlerParams& fparams) {
  const string& id = fparams.id;
  map<string, CreateCrawlerParams*>::iterator it = crawlers_.find(id);
  if ( it == crawlers_.end() ) {
    LOG_ERROR << "DestroyCrawler unknown id: " << id
              << ", fparams: " << fparams;
    return;
  }
  LOG_DEBUG << "DestroyCrawler id: " << id
            << ", fparams: " << fparams;
  delete it->second;
  crawlers_.erase(it);
}
void OsdStateKeeperElement::ProcessOsd(const ShowCrawlersParams& fparams) {
  LOG_DEBUG << "ShowCrawlers, fparams: " << fparams;
  show_crawlers_.show = fparams.show;
}
void OsdStateKeeperElement::ProcessOsd(const AddCrawlerItemParams& fparams) {
  const string& crawler_id = fparams.crawler_id;
  map<string, CreateCrawlerParams*>::iterator
    it = crawlers_.find(crawler_id);
  if ( it == crawlers_.end() ) {
    LOG_ERROR << "AddCrawlerItem unknown crawler, crawler_id: "
              << crawler_id << ", fparams: " << fparams;
    return;
  }
  CreateCrawlerParams* pparams = it->second;
  if (pparams->items.is_set()) {
    map<string, CrawlerItem>::iterator it_item =
        pparams->items.ref().find(fparams.item_id);
    if (it_item != pparams->items.ref().end()) {
      CrawlerItem& item = it_item->second;
      if (fparams.item.get().content.is_set())
        item.content = fparams.item.get().content;

      if (fparams.item.get().fore_color.is_set())
        item.fore_color = fparams.item.get().fore_color;
      if (fparams.item.get().back_color.is_set())
        item.back_color = fparams.item.get().back_color;
      if (fparams.item.get().link_color.is_set())
        item.link_color = fparams.item.get().link_color;
      if (fparams.item.get().hover_color.is_set())
        item.hover_color = fparams.item.get().hover_color;
      LOG_DEBUG << "AddCrawlerItem crawler_id: " << crawler_id
                << ", item_id: " << fparams.item_id.ToString()
                << ", updated with fparams: " << fparams.ToString();
    } else {
      LOG_DEBUG << "AddCrawlerItem crawler_id: " << crawler_id
                << ", item_id: " << fparams.item_id.ToString()
                << ", created with fparams: " << fparams.ToString();
      pparams->items.ref()[fparams.item_id] = fparams.item;
    }
  } else {
    LOG_DEBUG << "AddCrawlerItem crawler_id: " << crawler_id
              << ", item_id: " << fparams.item_id.ToString()
              << ", updated with fparams: " << fparams.ToString();
    pparams->items.set(map<string, CrawlerItem>());
    pparams->items.ref()[fparams.item_id] = fparams.item;
  }
}
void OsdStateKeeperElement::ProcessOsd(const RemoveCrawlerItemParams& fparams) {
  const string& crawler_id = fparams.crawler_id;
  map<string, CreateCrawlerParams*>::iterator it = crawlers_.find(crawler_id);
  if ( it == crawlers_.end() ) {
    LOG_ERROR << "RemoveCrawlerItem unknown crawler, crawler_id: "
              << crawler_id << ", fparams: " << fparams.ToString();
    return;
  }
  LOG_DEBUG << "RemoveCrawlerItem crawler_id: " << crawler_id
            << ", item_id: " << fparams.item_id.ToString()
            << ", removed with fparams: " << fparams.ToString();
  CreateCrawlerParams* pparams = it->second;
  if (pparams->items.is_set()) {
    pparams->items.ref().erase(fparams.item_id);
  }
}
void OsdStateKeeperElement::ProcessOsd(
    const SetPictureInPictureParams& fparams) {
  LOG_DEBUG << "SetPictureInPicture with fparams: " << fparams;
  pip_ = fparams;
}
void OsdStateKeeperElement::ProcessOsd(
    const SetClickUrlParams& fparams) {
  LOG_DEBUG << "SetClickUrl with fparams: " << fparams;
  click_url_ = fparams;
}
void OsdStateKeeperElement::ProcessOsd(const CreateMovieParams& fparams) {
  const string& id = fparams.id;
  map<string, CreateMovieParams*>::iterator it = movies_.find(id);
  if ( it != movies_.end() ) {
    CreateMovieParams *pparams = it->second;
    if (fparams.url.is_set())
      pparams->url = fparams.url;

    if (fparams.x_location.is_set())
      pparams->x_location = fparams.x_location;
    if (fparams.y_location.is_set())
      pparams->y_location = fparams.y_location;

    if (fparams.width.is_set())
      pparams->width = fparams.width;
    if (fparams.height.is_set())
      pparams->height = fparams.height;

    if (fparams.main_volume.is_set())
      pparams->main_volume = fparams.main_volume;
    if (fparams.movie_volume.is_set())
      pparams->movie_volume = fparams.movie_volume;

    if (fparams.ttl.is_set())
      pparams->ttl = fparams.ttl;

    LOG_DEBUG << "CreateMovie id: " << id
              << ", updated with fparams: " << fparams;
    return;
  }
  LOG_DEBUG << "CreateMovie id: " << id
            << ", created with fparams: " << fparams;
  movies_.insert(make_pair(id, new CreateMovieParams(fparams)));
}
void OsdStateKeeperElement::ProcessOsd(const DestroyMovieParams& fparams) {
  const string& id = fparams.id;
  map<string, CreateMovieParams*>::iterator it = movies_.find(id);
  if ( it == movies_.end() ) {
    LOG_ERROR << "DestroyMovie unknown id: " << id
              << ", fparams: " << fparams;
    return;
  }
  LOG_DEBUG << "DestroyMovie id: " << id
            << ", fparams: " << fparams;
  delete it->second;
  movies_.erase(it);
}

void OsdStateKeeperElement::AppendBootstrap(
    FilteringCallbackData::TagList* tags,
    uint32 flavour_mask,
    int64 timestamp_ms) {
  // play Overlays state
  for ( map<string, CreateOverlayParams*>::const_iterator
            it = overlays_.begin(); it != overlays_.end(); ++it ) {
    const CreateOverlayParams& create_overlay = *it->second;
    AppendOsd(create_overlay, tags, flavour_mask, timestamp_ms);
  }
  if ( !show_overlays_.show.get() ) {
    AppendOsd(show_overlays_, tags, flavour_mask, timestamp_ms);
  }

  // play Crawlers state
  for ( map<string, CreateCrawlerParams*>::const_iterator
            it = crawlers_.begin();
        it != crawlers_.end(); ++it ) {
    const CreateCrawlerParams& create_crawler = *it->second;
    AppendOsd(create_crawler, tags, flavour_mask, timestamp_ms);
  }
  if ( !show_crawlers_.show.get() ) {
    AppendOsd(show_crawlers_, tags, flavour_mask, timestamp_ms);
  }
  AppendOsd(pip_, tags, flavour_mask, timestamp_ms);
  AppendOsd(click_url_, tags, flavour_mask, timestamp_ms);
  for ( map<string, CreateMovieParams*>::const_iterator
            it = movies_.begin(); it != movies_.end(); ++it ) {
    const CreateMovieParams& create_movie = *it->second;
    AppendOsd(create_movie, tags, flavour_mask, timestamp_ms);
  }
}

FilteringCallbackData* OsdStateKeeperElement::CreateCallbackData(
    const char* media_name, streaming::Request* req) {
  return new OsdStateKeeperCallbackData(this);
}

void OsdStateKeeperElement::GetOverlays(
    rpc::CallContext<OverlaysState>* call) {
  OverlaysState state;
  state.overlays = MakeRPCMap(overlays_);
  state.show = show_overlays_.show;
  call->Complete(state);
}

void OsdStateKeeperElement::GetCrawlers(
    rpc::CallContext<CrawlersState>* call) {
  CrawlersState state;
  state.crawlers = MakeRPCMap(crawlers_);
  state.show = show_crawlers_.show;
  call->Complete(state);
}

void OsdStateKeeperElement::GetMovies(
    rpc::CallContext<MoviesState>* call) {
  MoviesState state;
  state.movies = MakeRPCMap(movies_);
  call->Complete(state);
}

void OsdStateKeeperElement::GetOverlay(
    rpc::CallContext<CreateOverlayParams>* call,
    const string& id) {
  map<string, CreateOverlayParams*>::const_iterator
      it = overlays_.find(id);
  if ( it == overlays_.end() ) {
    LOG_WARNING << "Invalid overlay id: " << id;
    CreateOverlayParams p;
    p.id.ref() = "";
    call->Complete(p);
    return;
  }
  call->Complete(*it->second);
}

void OsdStateKeeperElement::GetCrawler(
    rpc::CallContext<CreateCrawlerParams>* call,
    const string& id) {
  map<string, CreateCrawlerParams*>::const_iterator
      it = crawlers_.find(id);
  if ( it == crawlers_.end() ) {
    LOG_WARNING << "Invalid crawler id: " << id;
    CreateCrawlerParams p;
    p.id = "";
    call->Complete(p);
    return;
  }
  call->Complete(*it->second);
}

void OsdStateKeeperElement::GetPictureInPicture(
    rpc::CallContext< PictureInPictureState >* call) {
  PictureInPictureState state(pip_);
  call->Complete(state);
}
void OsdStateKeeperElement::GetClickUrl(
    rpc::CallContext< ClickUrlState >* call) {
  ClickUrlState state(click_url_);
  call->Complete(state);
}

void OsdStateKeeperElement::GetMovie(
    rpc::CallContext<CreateMovieParams>* call,
    const string& id) {
  map<string, CreateMovieParams*>::const_iterator
      it = movies_.find(id);
  if ( it == movies_.end() ) {
    LOG_WARNING << "Invalid movie id: " << id;
    CreateMovieParams p;
    p.id.ref();
    call->Complete(p);
    return;
  }
  call->Complete(*it->second);
}
}
