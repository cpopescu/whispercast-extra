// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperlib/common/io/ioutil.h>
#include "osd/osd_library/osd_associator/osd_associator_element.h"

namespace streaming {

struct ClipOsds {
  ClipOsds(const string& media);
  ClipOsds(const AssociatedOsds& s);
  ~ClipOsds();

  typedef map<string, CreateOverlayParams*> OverlayMap;
  typedef map<string, CreateCrawlerParams*> CrawlerMap;
  typedef map<string, CrawlerItem*> ItemMap;
  typedef map<string, ItemMap*> CrawlerItemMap;

  void DeleteItemMap(ItemMap* item_map);

  bool DeleteOverlay(const string& overlay_id) {
    return overlays_.erase(overlay_id);
  }
  bool DeleteCrawler(const string& crawler_id) {
    CrawlerItemMap::iterator it = crawlers_.find(crawler_id);
    if ( it == crawlers_.end() ) {
      return false;
      }
    DeleteItemMap(it->second);
    crawlers_.erase(it);
    return true;
  }
  void GetAssociatedOsds(AssociatedOsds* osds) const;

  void GetBeginOsdTags(const OverlayMap& overlay_types,
                       const CrawlerMap& crawler_types,
                       vector< scoped_ref<streaming::OsdTag> >*
                         begin_tags) const;
  void GetEndOsdTags(const OverlayMap& overlay_types,
                     const CrawlerMap& crawler_types,
                     vector< scoped_ref<streaming::OsdTag> >*
                       end_tags) const;

  string ToString() const {
    ostringstream oss;
    oss << "ClipOsds{media_: " << media_
        << ", overlays_: " << strutil::ToString(overlays_)
        << ", crawlers_: " << strutil::ToStringP(crawlers_)
        << ", pip_: " << (pip_ == NULL ? "NULL" : pip_->ToString().c_str())
        << ", click_url_: " << (click_url_ == NULL ? "NULL" : click_url_->ToString().c_str())
        << ", eos_clear_overlays_: " << strutil::BoolToString(eos_clear_overlays_)
        << ", eos_clear_crawlers_: " << strutil::BoolToString(eos_clear_crawlers_)
        << ", eos_clear_crawl_items_: " << strutil::BoolToString(eos_clear_crawl_items_)
        << ", eos_clear_pip_: " << strutil::BoolToString(eos_clear_pip_)
        << ", eos_clear_click_url_: " << strutil::BoolToString(eos_clear_click_url_)
        << "}";
    return oss.str();
  }

  //////////////////////////////////////////////////////////////////////

  const string media_;
  map<string, string> overlays_;
  CrawlerItemMap crawlers_;
  SetPictureInPictureParams* pip_;
  SetClickUrlParams* click_url_;
  bool eos_clear_overlays_;
  bool eos_clear_crawlers_;
  bool eos_clear_crawl_items_;
  bool eos_clear_pip_;
  bool eos_clear_click_url_;
};


class OsdAssocCallbackData : public streaming::FilteringCallbackData {
 public:
  typedef map<string, CreateOverlayParams*> OverlayMap;
  typedef map<string, CreateCrawlerParams*> CrawlerMap;
  OsdAssocCallbackData(OsdAssociatorElement* master,
                       const OverlayMap& overlay_types,
                       const CrawlerMap& crawler_types)
      : streaming::FilteringCallbackData(),
        master_(master),
        overlay_types_(overlay_types),
        crawler_types_(crawler_types) {
  }
  virtual ~OsdAssocCallbackData() {
    for ( map<string, ClipOsds*>::const_iterator it = assoc_paths().begin();
          it != assoc_paths().end(); ++it ) {
      master_->DelUserFromMedia(it->first, this);
    }
  }

  // Schedules for injection a copy of the given OSD tag.
  void InjectOsdTag(const streaming::OsdTag& tag) {
    pending_osd_tags_.push_back(
        static_cast<streaming::OsdTag*>(tag.Clone(-1)));
  }

  vector< scoped_ref<streaming::OsdTag> >*
  mutable_pending_osd_tags() {
    return &pending_osd_tags_;
  }
  const map<string, ClipOsds*>& assoc_paths() {
    return assoc_paths_;
  }

  void ChangeOsds(const string& media, ClipOsds* new_osd);
  ///////////////////////////////////////////////////////////////////////
  //
  // FilteringCallbackData methods
  //
  virtual void FilterTag(const streaming::Tag* tag, TagList* out);
 private:
  void PrependOsdTags(TagList* out, int64 crt_timestamp_ms,
      const vector< scoped_ref<streaming::OsdTag> >& tags) const;

  // Osd tags to be sent on next media tag.
  OsdAssociatorElement* const master_;
  vector< scoped_ref<streaming::OsdTag> > pending_osd_tags_;
  // WARN: Do not delete ClipOsds* ! master_->associated_osds_ owns them !
  // It's legal to have: [media, NULL] pairs in this map -> It means we are
  // filtering that media, but we don't have any OSDs associated with it.
  map<string, ClipOsds*> assoc_paths_;
  const OverlayMap& overlay_types_;
  const CrawlerMap& crawler_types_;
};

void OsdAssocCallbackData::ChangeOsds(const string& media, ClipOsds* new_osd) {
  DLOG_DEBUG << media_name_ << ": Changing OSD for media: " << media;
  map<string, ClipOsds*>::iterator it = assoc_paths_.find(media);
  if ( it == assoc_paths_.end() ) {
    it = assoc_paths_.insert(make_pair(media, new_osd)).first;
  }
  CHECK(it != assoc_paths_.end());
  ClipOsds* old_osd = it->second;
  if ( old_osd != NULL && old_osd != new_osd ) {
    old_osd->GetEndOsdTags(overlay_types_,
                           crawler_types_,
                           &pending_osd_tags_);
    LOG_DEBUG << media_name_ << ": Removing old OSDs: "
             << pending_osd_tags_.size();
  }
  it->second = new_osd;
  if ( new_osd != NULL ) {
    new_osd->GetBeginOsdTags(overlay_types_,
                             crawler_types_,
                             &pending_osd_tags_);
    LOG_DEBUG << media_name_ << ": Adding new OSDs: "
             << pending_osd_tags_.size();
  }
}

void OsdAssocCallbackData::PrependOsdTags(
    TagList* out,
    int64 timestamp_ms,
    const vector< scoped_ref<streaming::OsdTag> >& tags) const {
  LOG_DEBUG << media_name_ << ": Injecting " << tags.size() << " OSDs"
           << " with timestamp: " << timestamp_ms;
  for ( int i = tags.size() - 1; i >= 0; --i ) {
    streaming::OsdTag* const osd = tags[i].get();
    LOG_DEBUG << media_name_ << ": Inserting: " << osd->ToString();
    osd->set_flavour_mask(flavour_mask());
    osd->set_timestamp_ms(timestamp_ms);
    out->push_front(osd);
  }
}

void OsdAssocCallbackData::FilterTag(const streaming::Tag* tag, TagList* out) {

  // Always forward the original incoming tag.
  // Some OSD tags may prepended before the original.
  out->push_back(tag);

  // EOS - clean all tags..
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    if ( !pending_osd_tags_.empty() ) {
      PrependOsdTags(out, tag->timestamp_ms(), pending_osd_tags_);
    }
    pending_osd_tags_.clear();
    vector< scoped_ref<streaming::OsdTag> > eos_osd_tags;
    for ( map<string, ClipOsds*>::const_iterator it = assoc_paths_.begin();
          it != assoc_paths_.end(); ++it ) {
      ClipOsds* clip = it->second;
      if ( clip != NULL ) {
        clip->GetEndOsdTags(overlay_types_, crawler_types_, &eos_osd_tags);
      }
      master_->DelUserFromMedia(it->first, this);
    }
    assoc_paths_.clear();
    if ( !eos_osd_tags.empty() ) {
      PrependOsdTags(out, tag->timestamp_ms(), eos_osd_tags);
    }
    return;
  }

  // START a new stream - add new osds
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ) {
    const SourceStartedTag* source_started_tag =
        static_cast<const SourceStartedTag*>(tag);
    const string& new_media = source_started_tag->path();
    LOG_DEBUG << media_name_ << ": New OSD media: [" << new_media << "]";
    map<string, ClipOsds*>::const_iterator it = assoc_paths_.find(new_media);
    if ( it == assoc_paths_.end() || it->second == NULL ) {
      ClipOsds* clip_osds = master_->GetMediaOsds(new_media);
      // WARN: clip_osds is NULL for every media we have nothing associated with
      ChangeOsds(new_media, clip_osds);
      master_->AddUserForMedia(new_media, this);
    }
  }

  // END a stream - delete associated osds
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    const SourceEndedTag* source_ended_tag =
        static_cast<const SourceEndedTag*>(tag);
    const string& old_media = source_ended_tag->path();
    LOG_DEBUG << media_name_ << ": Old OSD media: [" << old_media << "]";
    ChangeOsds(old_media, NULL);
    master_->DelUserFromMedia(old_media, this);
  }
  if ( !pending_osd_tags_.empty() && tag->is_video_tag() ) {
    // we inject osd tags using the timestamp of the current media tag
    PrependOsdTags(out, tag->timestamp_ms(), pending_osd_tags_);
    pending_osd_tags_.clear();
  }
  return;
}

}

namespace streaming {

const char OsdAssociatorElement::kElementClassName[] = "osd_associator";

OsdAssociatorElement::OsdAssociatorElement(const char* name,
                                           const char* id,
                                           ElementMapper* mapper,
                                           net::Selector* selector,
                                           io::StateKeepUser* state_keeper,
                                           const char* rpc_path,
                                           rpc::HttpServer* rpc_server)
    : FilteringElement(kElementClassName, name, id, mapper, selector),
      ServiceInvokerOsdAssociator(ServiceInvokerOsdAssociator::GetClassName()),
      state_keeper_(state_keeper),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      is_registered_(false) {
}

OsdAssociatorElement::~OsdAssociatorElement() {
  if ( is_registered_ ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }
  for ( OverlayMap::iterator it = overlay_types_.begin();
        it != overlay_types_.end(); ++it ) {
    delete it->second;
  }
  for ( CrawlerMap::iterator it = crawler_types_.begin();
        it != crawler_types_.end(); ++it ) {
    delete it->second;
  }
  for ( OsdMap::iterator it = associated_osds_.begin();
        it != associated_osds_.end(); ++it ) {
    delete it->second;
  }
  for ( AssocMap::iterator it = active_requests_.begin();
        it != active_requests_.end(); ++it ) {
    delete it->second;
  }
}


bool OsdAssociatorElement::Initialize() {
  if ( rpc_server_ != NULL ) {
    is_registered_ = rpc_server_->RegisterService(rpc_path_, this);
  }
  return LoadState() && (is_registered_ || rpc_server_ == NULL);
}

//////////////////////////////////////////////////////////////////////

bool OsdAssociatorElement::LoadState() {
  if ( !state_keeper_ ) {
    return false;
  }
  map<string, string>::const_iterator begin, end;
  state_keeper_->GetBounds("o-", &begin, &end);

  bool ret = true;
  ///// Overlays

  for ( map<string, string>::const_iterator it = begin; it != end; ++it ) {
    CreateOverlayParams* params = new CreateOverlayParams();
    if ( !rpc::JsonDecoder::DecodeObject(it->second, params) ) {
      LOG_ERROR << "Error decoding overlays:\n" << it->second;
      delete params;
      ret = false;
    } else {
      LOG_DEBUG << name() << ": Adding overlay Type: " << params->ToString();
      overlay_types_.insert(make_pair(params->id,
                                      params));
    }
  }

  ///// Crawlers

  state_keeper_->GetBounds("c-", &begin, &end);
  for ( map<string, string>::const_iterator it = begin; it != end; ++it ) {
    CreateCrawlerParams* params = new CreateCrawlerParams();
    if ( !rpc::JsonDecoder::DecodeObject(it->second, params) ) {
      LOG_ERROR << "Error decoding crawler:\n" << it->second;
      delete params;
      ret = false;
    } else {
      LOG_DEBUG << name() << ": Adding crawler Type: " << params->ToString();
      crawler_types_.insert(make_pair(params->id,
                                      params));
    }
  }

  ///// Clip data

  state_keeper_->GetBounds("a-", &begin, &end);
  for ( map<string, string>::const_iterator it = begin; it != end; ++it ) {
    AssociatedOsds params;
    if ( !rpc::JsonDecoder::DecodeObject(it->second, &params) ) {
      LOG_ERROR << "Error decoding associated data:\n" << it->second;
      ret = false;
    } else {
      LOG_DEBUG << name() << ": Adding associated OSD: "
               << it->first.substr(state_keeper_->prefix().size() + 2) << " : "
               << params.ToString();
      associated_osds_.insert(
          make_pair(it->first.substr(state_keeper_->prefix().size() + 2),
                    new ClipOsds(params)));
    }
  }

  return ret;
}

//////////////////////////////////////////////////////////////////////

FilteringCallbackData* OsdAssociatorElement::CreateCallbackData(
    const char* media_name,
    streaming::Request* req) {
  OsdAssocCallbackData* ret = new OsdAssocCallbackData(this,
                                                       overlay_types_,
                                                       crawler_types_);
  all_requests_.insert(ret);
  return ret;
}

void OsdAssociatorElement::AddUserForMedia(const string& media,
                                           OsdAssocCallbackData* data) {
  DLOG_DEBUG << name() << ": Add callback data on media: " << media
      << ", data: " << hex << data;
  AssocMap::iterator assoc_it = active_requests_.find(media);
  if ( assoc_it == active_requests_.end() ) {
    AssocSet* assoc_set = new AssocSet();
    assoc_set->insert(data);
    active_requests_.insert(make_pair(media, assoc_set));
  } else {
    assoc_it->second->insert(data);
  }
}
void OsdAssociatorElement::DelUserFromMedia(const string& media,
                                            OsdAssocCallbackData* data) {
  DLOG_DEBUG << name() << ": Delete callback data on media: " << media
      << ", data: " << hex << data;
  AssocMap::iterator assoc_it = active_requests_.find(media);
  if ( assoc_it != active_requests_.end() ) {
    assoc_it->second->erase(data);
    if ( assoc_it->second->empty() ) {
      delete assoc_it->second;
      active_requests_.erase(assoc_it);
    }
  }
}

ClipOsds* OsdAssociatorElement::GetMediaOsds(const string& media) {
  const OsdMap::const_iterator it = associated_osds_.find(media);
  if ( it != associated_osds_.end() ) {
    return it->second;
  }
  return NULL;
}
const ClipOsds* OsdAssociatorElement::GetMediaOsds(const string& media) const {
  const OsdMap::const_iterator it = associated_osds_.find(media);
  return it == associated_osds_.end() ? NULL : it->second;
}

void OsdAssociatorElement::DeleteCallbackData(FilteringCallbackData* data) {
  FilteringElement::DeleteCallbackData(data);
}

//////////////////////////////////////////////////////////////////////

void OsdAssociatorElement::SetAssociatedOsds(
    const AssociatedOsds& osds) {
  const string& media(osds.media);
  OsdMap::iterator it = associated_osds_.find(media);
  ClipOsds* new_osd = new ClipOsds(osds);
  ClipOsds* old_osd = NULL;
  if ( it != associated_osds_.end() ) {
    old_osd = it->second;
    it->second = new_osd;
    LOG_DEBUG << name() << ": Changed associated OSDs for media: " << media;
  } else {
    associated_osds_.insert(make_pair(media, new_osd));
  }
  // We need to do an update on clips..
  AssocMap::const_iterator assoc_it = active_requests_.find(media);
  if ( assoc_it != active_requests_.end() ) {
    for ( AssocSet::const_iterator data_it = assoc_it->second->begin();
          data_it != assoc_it->second->end(); ++data_it ) {
      (*data_it)->ChangeOsds(media, new_osd);
    }
  }
  delete old_osd;
  if ( state_keeper_ ) {
    string s;
    rpc::JsonEncoder::EncodeToString(osds, &s);
    state_keeper_->SetValue(
        strutil::StringPrintf("a-%s", media.c_str()), s);
  }
}

void OsdAssociatorElement::DeleteAssociatedOsds(const string& media) {
  OsdMap::iterator it = associated_osds_.find(media);
  if ( it != associated_osds_.end() ) {
    LOG_DEBUG << name() << ": Deleted associated OSDs for media: "
              << media;
    // We need to do an update on clips..
    AssocMap::iterator assoc_it = active_requests_.find(media);
    if ( assoc_it != active_requests_.end() ) {
      for ( AssocSet::const_iterator data_it = assoc_it->second->begin();
            data_it != assoc_it->second->end(); ++data_it ) {
        (*data_it)->ChangeOsds(media, NULL);
      }
    }
    delete it->second;
    associated_osds_.erase(it);
  }
}

void OsdAssociatorElement::CreateOverlayType(
    const CreateOverlayParams& params) {
  OverlayMap::iterator it = overlay_types_.find(params.id);
  if ( it != overlay_types_.end() ) {
    delete it->second;
    it->second = reinterpret_cast<CreateOverlayParams*>(params.Clone());
    LOG_DEBUG << name() << ": Changed overlay type: " << params.ToString();
  } else {
    overlay_types_.insert(
        make_pair(params.id,
                  reinterpret_cast<CreateOverlayParams*>(params.Clone())));
    LOG_DEBUG << name() << ": Added overlay type: " << params.ToString();
  }
  if ( state_keeper_ ) {
    string s;
    rpc::JsonEncoder::EncodeToString(params, &s);
    state_keeper_->SetValue(
        strutil::StringPrintf("o-%s", params.id.c_str()), s);
  }

  // Update the style for all that use it..
  for ( OsdMap::const_iterator it = associated_osds_.begin();
        it != associated_osds_.end(); ++it ) {
    if ( it->second->overlays_.find(params.id) !=
         it->second->overlays_.end() ) {
      // We need to do an update here..
      AssocMap::iterator assoc_it = active_requests_.find(it->first);
      if ( assoc_it != active_requests_.end() ) {
        for ( AssocSet::const_iterator data_it = assoc_it->second->begin();
              data_it != assoc_it->second->end(); ++data_it ) {
          (*data_it)->mutable_pending_osd_tags()->push_back(
              new streaming::OsdTag(0, streaming::kDefaultFlavourMask,
                  0, params));
        }
      }
    }
  }
}

void OsdAssociatorElement::DestroyOverlayType(const string& id) {
  OverlayMap::iterator it = overlay_types_.find(id);
  if ( it != overlay_types_.end() ) {
    delete it->second;
    overlay_types_.erase(it);
    if ( state_keeper_ ) {
      state_keeper_->DeleteValue(
          strutil::StringPrintf("o-%s", id.c_str()));
    }
    for ( OsdMap::iterator it = associated_osds_.begin();
          it != associated_osds_.end(); ++it ) {
      if ( it->second->DeleteOverlay(id) ) {
        // We need to do an update here..
        AssocMap::iterator assoc_it = active_requests_.find(it->first);
        if ( assoc_it != active_requests_.end() ) {
          for ( AssocSet::const_iterator data_it = assoc_it->second->begin();
                data_it != assoc_it->second->end(); ++data_it ) {
            (*data_it)->mutable_pending_osd_tags()->push_back(
                new streaming::OsdTag(
                    0, streaming::kDefaultFlavourMask, 0,
                    DestroyOverlayParams(id)));
          }
        }
      }
    }
    LOG_DEBUG << name() << ": Deleted overlay type: " << id;
  }
}

void OsdAssociatorElement::CreateCrawlerType(
    const CreateCrawlerParams& params) {
  CrawlerMap::iterator it = crawler_types_.find(params.id);
  if ( it != crawler_types_.end() ) {
    LOG_DEBUG << name() << ": Added crawler type: " << params.ToString();
    delete it->second;
    it->second = static_cast<CreateCrawlerParams*>(params.Clone());
  } else {
    LOG_DEBUG << name() << ": Added crawler type: " << params.ToString();
    crawler_types_.insert(
        make_pair(params.id,
                  static_cast<CreateCrawlerParams*>(params.Clone())));
  }
  if ( state_keeper_ ) {
    string s;
    rpc::JsonEncoder::EncodeToString(params, &s);
    state_keeper_->SetValue(
        strutil::StringPrintf("c-%s", params.id.c_str()), s);
  }

  // Update the style for all that use it..
  for ( OsdMap::const_iterator it = associated_osds_.begin();
        it != associated_osds_.end(); ++it ) {
    if ( it->second->crawlers_.find(params.id) !=
         it->second->crawlers_.end() ) {
      // We need to do an update here..
      AssocMap::iterator assoc_it = active_requests_.find(it->first);
      if ( assoc_it != active_requests_.end() ) {
        for ( AssocSet::const_iterator data_it = assoc_it->second->begin();
              data_it != assoc_it->second->end(); ++data_it ) {
          (*data_it)->mutable_pending_osd_tags()->push_back(
              new streaming::OsdTag(0, streaming::kDefaultFlavourMask,
                  0, params));
        }
      }
    }
  }
}

void OsdAssociatorElement::DestroyCrawlerType(const string& id) {
  CrawlerMap::iterator it = crawler_types_.find(id);
  if ( it != crawler_types_.end() ) {
    delete it->second;
    crawler_types_.erase(it);
    if ( state_keeper_ ) {
      state_keeper_->DeleteValue(
          strutil::StringPrintf("c-%s", id.c_str()));
    }
    for ( OsdMap::iterator it = associated_osds_.begin();
          it != associated_osds_.end(); ++it ) {
      if ( it->second->DeleteCrawler(id) ) {
        // We need to do an update here..
        AssocMap::iterator assoc_it = active_requests_.find(it->first);
        if ( assoc_it != active_requests_.end() ) {
          for ( AssocSet::const_iterator data_it = assoc_it->second->begin();
                data_it != assoc_it->second->end(); ++data_it ) {
            (*data_it)->mutable_pending_osd_tags()->push_back(
                new streaming::OsdTag(
                    0, streaming::kDefaultFlavourMask, 0,
                    DestroyCrawlerParams(id)));
          }
        }
      }
    }
    LOG_DEBUG << name() << ": Deleted crawler type: " << id;
  }
}

void OsdAssociatorElement::GetFullState(FullOsdState* state) {
  for ( OverlayMap::iterator it = overlay_types_.begin();
        it != overlay_types_.end(); ++it ) {
    state->overlays.ref().push_back(*it->second);
  }
  for ( CrawlerMap::iterator it = crawler_types_.begin();
        it != crawler_types_.end(); ++it ) {
    state->crawlers.ref().push_back(*it->second);
  }
  for ( OsdMap::iterator it = associated_osds_.begin();
        it != associated_osds_.end(); ++it ) {
    AssociatedOsds osd;
    it->second->GetAssociatedOsds(&osd);
    state->osds.ref().push_back(osd);
  }
}

void OsdAssociatorElement::SetFullState(const FullOsdState& state,
                                        bool just_add) {
  //////////////////////////////////////////////////////////////////////
  //
  // Read the state in some temporary structures
  //
  LOG_DEBUG << name() << ": Setting new state";
  OverlayMap new_overlays;
  CrawlerMap new_crawlers;
  map<string, AssociatedOsds*> new_associated_osds;
  for ( int i = 0; i < state.overlays.size(); ++i ) {
    if ( new_overlays.find(state.overlays[i].id)
         == new_overlays.end() ) {
      new_overlays.insert(
          make_pair(state.overlays[i].id,
                    new CreateOverlayParams(state.overlays[i])));
    }
  }
  for ( int i = 0; i < state.crawlers.size(); ++i ) {
    if ( new_crawlers.find(state.crawlers[i].id)
         == new_crawlers.end() ) {
      new_crawlers.insert(
          make_pair(state.crawlers[i].id,
                    new CreateCrawlerParams(state.crawlers[i])));
    }
  }
  for ( int i = 0; i < state.osds.size(); ++i ) {
    if ( new_associated_osds.find(state.osds[i].media)
         == new_associated_osds.end() ) {
      new_associated_osds.insert(
          make_pair(state.osds[i].media,
                    new AssociatedOsds(state.osds[i])));
    }
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Compute the differences between the new state and old stat
  //
  vector<string> overlays_to_add;
  vector<string> overlays_to_delete;
  vector<string> crawlers_to_add;
  vector<string> crawlers_to_delete;
  vector<string> osds_to_add;
  vector<string> osds_to_delete;

  // diff for OVERLAYs
  for ( OverlayMap::iterator it = new_overlays.begin();
        it != new_overlays.end(); ++it ) {
    OverlayMap::const_iterator it_tmp = overlay_types_.find(it->first);
    if ( it_tmp == overlay_types_.end() ) {
      LOG_DEBUG << name() << ": New overlay: " << it->first;
      overlays_to_add.push_back(it->first);
    } else if ( !(*it_tmp->second == *it->second) ) {
      LOG_DEBUG << name() << ": Changed overlay: " << it->first;
      overlays_to_add.push_back(it->first);
    }
  }
  if ( !just_add ) {
    for ( OverlayMap::iterator it = overlay_types_.begin();
          it != overlay_types_.end(); ++it ) {
      if ( new_overlays.find(it->first) == new_overlays.end() ) {
        LOG_DEBUG << name() << ": Deleted overlay: " << it->first;
        overlays_to_delete.push_back(it->first);
      }
    }
  }

  // diff for CRAWLERs
  for ( CrawlerMap::iterator it = new_crawlers.begin();
        it != new_crawlers.end(); ++it ) {
    CrawlerMap::const_iterator it_tmp = crawler_types_.find(it->first);
    if ( it_tmp == crawler_types_.end() ) {
      LOG_DEBUG << name() << ": New crawler: " << it->first;
      crawlers_to_add.push_back(it->first);
    } else if ( !(*it_tmp->second == *it->second) ) {
      LOG_DEBUG << name() << ": Changed crawler: " << it->first;
      crawlers_to_add.push_back(it->first);
    }
  }
  if ( !just_add ) {
    for ( CrawlerMap::iterator it = crawler_types_.begin();
          it != crawler_types_.end(); ++it ) {
      if ( new_crawlers.find(it->first) == new_crawlers.end() ) {
        LOG_DEBUG << name() << ": Deleted crawler: " << it->first;
        crawlers_to_delete.push_back(it->first);
      }
    }
  }
  // diff the OSDs
  for ( map<string, AssociatedOsds*>::iterator it = new_associated_osds.begin();
        it != new_associated_osds.end(); ++it ) {
    OsdMap::const_iterator it_tmp = associated_osds_.find(it->first);
    if ( it_tmp == associated_osds_.end() ) {
      LOG_DEBUG << name() << ": New media OSD: " << it->first;
      osds_to_add.push_back(it->first);
    } else {
      AssociatedOsds osd;
      it_tmp->second->GetAssociatedOsds(&osd);
      if ( !(osd == *it->second) ) {
        LOG_DEBUG << name() << ": Changed media OSD: " << it->first;
        osds_to_add.push_back(it->first);
      }
    }
  }
  if ( !just_add ) {
    for ( OsdMap::iterator it = associated_osds_.begin();
          it != associated_osds_.end(); ++it ) {
      if ( new_associated_osds.find(it->first) == new_associated_osds.end() ) {
        LOG_DEBUG << name() << ": Deleted OSD: " << it->first;
        osds_to_delete.push_back(it->first);
      }
    }
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Apply the differences
  //

  for ( int i = 0; i < osds_to_delete.size(); ++i ) {
    DeleteAssociatedOsds(osds_to_delete[i]);
  }
  for ( int i = 0; i < crawlers_to_delete.size(); ++i ) {
    DestroyCrawlerType(crawlers_to_delete[i]);
  }
  for ( int i = 0; i < overlays_to_delete.size(); ++i ) {
    DestroyOverlayType(overlays_to_delete[i]);
  }
  for ( int i = 0; i < overlays_to_add.size(); ++i ) {
    CreateOverlayType(*new_overlays[overlays_to_add[i]]);
  }
  for ( int i = 0; i < crawlers_to_add.size(); ++i ) {
    CreateCrawlerType(*new_crawlers[crawlers_to_add[i]]);
  }
  for ( int i = 0; i < osds_to_add.size(); ++i ) {
    SetAssociatedOsds(*new_associated_osds[osds_to_add[i]]);
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Clear temp structures
  //
  for ( OverlayMap::const_iterator it = new_overlays.begin();
        it != new_overlays.end(); ++it ) {
    delete it->second;
  }
  for ( CrawlerMap::const_iterator it = new_crawlers.begin();
        it != new_crawlers.end(); ++it ) {
    delete it->second;
  }
  for ( map<string, AssociatedOsds*>::const_iterator
            it = new_associated_osds.begin();
        it != new_associated_osds.end(); ++it ) {
    delete it->second;
  }
}

void OsdAssociatorElement::GetRequestOsd(
    const MediaRequestInfoSpec& request,
    OsdAssociatorRequestOsd* out_osd) const {
  // Get all matching OSDs for given request path
  LOG_DEBUG << name() << ": TODO: Get all matching OSDs for the request path";
  string path = request.path_;
  const ClipOsds* clip = io::FindPathBased(&associated_osds_, path);
  if ( clip == NULL ) {
    LOG_ERROR << "No OSDs on path: [" << request.path_.ToString() << "]";
    return;
  }

  // Fill in out_osd.global
  OsdAssociatorCompleteOsd& out = out_osd->global.ref();

  out.overlays.ref();
  for ( map<string,string>::const_iterator it = clip->overlays_.begin();
        it != clip->overlays_.end(); ++it ) {
    const string& overlay_id = it->first;
    const string& overlay_value = it->second;
    OverlayMap::const_iterator ito = overlay_types_.find(overlay_id);
    if ( ito == overlay_types_.end() ) {
      continue;
    }
    const CreateOverlayParams* overlay_type = ito->second;

    CreateOverlayParams create_overlay(*overlay_type);
    create_overlay.content.ref() = overlay_value;
    out.overlays.ref().push_back(create_overlay);
  }
  out.crawlers.ref();
  for ( ClipOsds::CrawlerItemMap::const_iterator it = clip->crawlers_.begin();
        it != clip->crawlers_.end(); ++it ) {
    const string& crawler_id = it->first;
    const ClipOsds::ItemMap* item_map = it->second;
    CrawlerMap::const_iterator itc = crawler_types_.find(crawler_id);
    if ( itc == crawler_types_.end() ) {
      continue;
    }
    const CreateCrawlerParams* crawler_type = itc->second;

    CreateCrawlerParams create_crawler(*crawler_type);
    for ( ClipOsds::ItemMap::const_iterator iit = item_map->begin();
          iit != item_map->end(); ++iit ) {
      const string& item_id = iit->first;
      const CrawlerItem* item = iit->second;
      create_crawler.items.ref().insert(make_pair(item_id, *item));
    }
    out.crawlers.ref().push_back(create_crawler);
  }
  out.crawl_items.ref();
  if ( clip->pip_ != NULL ) {
    out.pip = *clip->pip_;
  }
  if ( clip->click_url_ != NULL ) {
    out.click_url = *clip->click_url_;
  }
  out.eos_clear_overlays = clip->eos_clear_overlays_;
  out.eos_clear_crawlers = clip->eos_clear_crawlers_;
  out.eos_clear_crawl_items = clip->eos_clear_crawl_items_;
  out.eos_clear_pip = clip->eos_clear_pip_;
  out.eos_clear_click_url = clip->eos_clear_click_url_;

  return;
}


//////////////////////////////////////////////////////////////////////

void OsdAssociatorElement::CreateOverlayType(
    rpc::CallContext< rpc::Void >* call,
    const CreateOverlayParams& params) {
  CreateOverlayType(params);
  call->Complete();
}

void OsdAssociatorElement::DestroyOverlayType(
    rpc::CallContext< rpc::Void >* call,
    const DestroyOverlayParams& params) {
  DestroyOverlayType(params.id);
  call->Complete();
}

void OsdAssociatorElement::GetOverlayType(
    rpc::CallContext< CreateOverlayParams >* call,
    const string& id) {
  OverlayMap::const_iterator it = overlay_types_.find(id);
  if ( it == overlay_types_.end() ) {
    call->Complete(rpc::RPC_GARBAGE_ARGS);
  }
  CreateOverlayParams ret;
  call->Complete(*it->second);
}

void OsdAssociatorElement::GetAllOverlayTypeIds(
    rpc::CallContext< vector<string> >* call) {
  vector<string> ret;
  for ( OverlayMap::const_iterator it = overlay_types_.begin();
        it != overlay_types_.end(); ++it ) {
    ret.push_back(it->first);
  }
  call->Complete(ret);
}

//////////////////////////////////////////////////////////////////////

void OsdAssociatorElement::CreateCrawlerType(
    rpc::CallContext< rpc::Void >* call,
    const CreateCrawlerParams& params) {
  CreateCrawlerType(params);
  call->Complete();
}

void OsdAssociatorElement::DestroyCrawlerType(
    rpc::CallContext< rpc::Void >* call,
    const DestroyCrawlerParams& params) {
  DestroyCrawlerType(params.id);
  call->Complete();
}

void OsdAssociatorElement::GetCrawlerType(
    rpc::CallContext< CreateCrawlerParams >* call,
    const string& id) {
  CrawlerMap::const_iterator it = crawler_types_.find(id);
  if ( it == crawler_types_.end() ) {
    call->Complete(rpc::RPC_GARBAGE_ARGS);
  }
  call->Complete(*it->second);
}

void OsdAssociatorElement::GetAllCrawlerTypeIds(
    rpc::CallContext< vector<string> >* call) {
  vector<string> ret;
  for ( CrawlerMap::const_iterator it = crawler_types_.begin();
        it != crawler_types_.end(); ++it ) {
    ret.push_back(it->first);
  }
  call->Complete(ret);
}

//////////////////////////////////////////////////////////////////////

void OsdAssociatorElement::GetAssociatedOsds(
    rpc::CallContext< AssociatedOsds >* call,
    const string& media) {
  AssociatedOsds osd;
  OsdMap::const_iterator it = associated_osds_.find(media);
  if ( it != associated_osds_.end() ) {
    it->second->GetAssociatedOsds(&osd);
  } else {
    osd.media = "";
  }
  call->Complete(osd);
}

void OsdAssociatorElement::SetAssociatedOsds(
    rpc::CallContext< rpc::Void >* call,
    const AssociatedOsds& osds) {
  SetAssociatedOsds(osds);
  call->Complete(rpc::Void());
}

void OsdAssociatorElement::DeleteAssociatedOsds(
    rpc::CallContext< rpc::Void >* call,
    const string& media) {
  DeleteAssociatedOsds(media);
  call->Complete(rpc::Void());
}

void OsdAssociatorElement::GetFullState(
    rpc::CallContext< FullOsdState >* call) {
  FullOsdState state;
  GetFullState(&state);
  call->Complete(state);
}

void OsdAssociatorElement::SetFullState(rpc::CallContext< rpc::Void >* call,
                                        const FullOsdState& state) {
  SetFullState(state, false);
  call->Complete(rpc::Void());
}

void OsdAssociatorElement::GetRequestOsd(
    rpc::CallContext< OsdAssociatorRequestOsd >* call,
    const MediaRequestInfoSpec& request) {
  OsdAssociatorRequestOsd req_osd;
  GetRequestOsd(request, &req_osd);
  call->Complete(req_osd);
}

//////////////////////////////////////////////////////////////////////

ClipOsds::ClipOsds(const string& media)
    : media_(media),
      pip_(NULL),
      click_url_(NULL),
      eos_clear_overlays_(true),
      eos_clear_crawlers_(true),
      eos_clear_crawl_items_(true),
      eos_clear_pip_(false),
      eos_clear_click_url_(true) {
}

ClipOsds::ClipOsds(const AssociatedOsds& s)
    : media_(s.media),
      pip_(NULL),
      click_url_(NULL),
      eos_clear_overlays_(s.eos_clear_overlays.is_set() &&
                          s.eos_clear_overlays.get()),
      eos_clear_crawlers_(s.eos_clear_crawlers.is_set() &&
                          s.eos_clear_crawlers.get()),
      eos_clear_crawl_items_(s.eos_clear_crawl_items.is_set() &&
                             s.eos_clear_crawl_items.get()),
      eos_clear_pip_(s.eos_clear_pip.is_set() &&
                     s.eos_clear_pip.get()),
      eos_clear_click_url_(s.eos_clear_click_url.is_set() &&
                           s.eos_clear_click_url.get()) {
  if ( s.pip.is_set() ) {
    pip_ = new SetPictureInPictureParams(s.pip);
  }
  if ( s.click_url.is_set() ) {
    click_url_ = new SetClickUrlParams(s.click_url);
  }
  for ( int i = 0; i < s.overlays.size(); ++i ) {
    overlays_.insert(make_pair(s.overlays[i].id,
                               s.overlays[i].content));
  }
  for ( int i = 0; i < s.crawlers.size(); ++i ) {
    CrawlerItemMap::iterator it = crawlers_.find(
        s.crawlers[i].crawler_id);
    ItemMap* m = NULL;
    if ( it != crawlers_.end() ) {
      m = it->second;
    } else {
      m = new ItemMap;
      crawlers_.insert(make_pair(
                           s.crawlers[i].crawler_id,
                           m));
    }
    (*m)[s.crawlers[i].item_id] =
        reinterpret_cast<CrawlerItem*>(
            s.crawlers[i].item.get().Clone());
  }
}

ClipOsds::~ClipOsds() {
  for ( CrawlerItemMap::iterator it = crawlers_.begin();
        it != crawlers_.end(); ++it ) {
    DeleteItemMap(it->second);
  }
}

void ClipOsds::DeleteItemMap(ItemMap* item_map) {
  for ( ItemMap::iterator it = item_map->begin();
        it != item_map->end(); ++it ) {
    delete it->second;
  }
  delete item_map;
}

void ClipOsds::GetAssociatedOsds(
    AssociatedOsds* osds) const {
  if ( pip_ != 0 ) {
    osds->pip.ref() = *pip_;
  }
  if ( click_url_ != 0 ) {
    osds->click_url.ref() = *click_url_;
  }
  osds->media.ref() = media_;
  osds->eos_clear_overlays.ref() = eos_clear_overlays_;
  osds->eos_clear_crawlers.ref() = eos_clear_crawlers_;
  osds->eos_clear_crawl_items.ref() = eos_clear_crawl_items_;
  osds->eos_clear_pip.ref() = eos_clear_pip_;
  osds->eos_clear_click_url.ref() = eos_clear_click_url_;
  for ( map<string, string>::const_iterator it = overlays_.begin();
        it != overlays_.end(); ++it ) {
    osds->overlays.ref().push_back(AddOverlayParams(it->first,
                                                    it->second));
  }
  for ( CrawlerItemMap::const_iterator it = crawlers_.begin();
        it != crawlers_.end(); ++it ) {
    for ( ItemMap::const_iterator cit = it->second->begin();
          cit != it->second->end(); ++cit ) {
      osds->crawlers.ref().push_back(
          AddCrawlerItemParams(it->first, cit->first,
                               *cit->second));
    }
  }
}

void ClipOsds::GetBeginOsdTags(
    const OverlayMap& overlay_types,
    const CrawlerMap& crawler_types,
    vector< scoped_ref<streaming::OsdTag> >* begin_tags) const {
  // TODO:: do we support something like this ?

  for ( map<string, string>::const_iterator it = overlays_.begin();
        it != overlays_.end(); ++it ) {
    OverlayMap::const_iterator ito = overlay_types.find(it->first);
    if ( ito != overlay_types.end() ) {
      CreateOverlayParams params(*ito->second);
      params.content = it->second;
      begin_tags->push_back(new streaming::OsdTag(
          0, streaming::kDefaultFlavourMask, 0, params));
    }
  }
  for ( CrawlerItemMap::const_iterator it = crawlers_.begin();
        it != crawlers_.end(); ++it ) {
    CrawlerMap::const_iterator itc = crawler_types.find(it->first);
    if ( itc != crawler_types.end() ) {
      CreateCrawlerParams params(*itc->second);
      begin_tags->push_back(new streaming::OsdTag(
          0, streaming::kDefaultFlavourMask, 0, params));
      for ( ItemMap::const_iterator item_it = it->second->begin();
            item_it != it->second->end(); ++item_it ) {
        begin_tags->push_back(
            new streaming::OsdTag(0, streaming::kDefaultFlavourMask, 0,
                AddCrawlerItemParams(
                    it->first, item_it->first, *item_it->second)));
      }
    }
  }
  if ( pip_ != NULL ) {
    begin_tags->push_back(new streaming::OsdTag(
        0, streaming::kDefaultFlavourMask, 0, *pip_));
  }
  if ( click_url_ != NULL ) {
    begin_tags->push_back(new streaming::OsdTag(
        0, streaming::kDefaultFlavourMask, 0, *click_url_));
  }
}

void ClipOsds::GetEndOsdTags(
    const OverlayMap& overlay_types,
    const CrawlerMap& crawler_types,
    vector< scoped_ref<streaming::OsdTag> >* end_tags) const {
  // TODO:: do we support something like this ?
  if ( eos_clear_overlays_ ) {
    for ( map<string, string>::const_iterator it = overlays_.begin();
          it != overlays_.end(); ++it ) {
      OverlayMap::const_iterator ito = overlay_types.find(it->first);
      if ( ito != overlay_types.end() ) {
        LOG_DEBUG << media_ << ": Destroy OSD: " << it->first;
        end_tags->push_back(new streaming::OsdTag(
            0, streaming::kDefaultFlavourMask, 0,
            DestroyOverlayParams(it->first)));

      }
    }
  }

  for ( CrawlerItemMap::const_iterator it = crawlers_.begin();
        it != crawlers_.end(); ++it ) {
    CrawlerMap::const_iterator itc = crawler_types.find(it->first);
    if ( itc != crawler_types.end() ) {
      if ( eos_clear_crawl_items_ ) {
        for ( ItemMap::const_iterator item_it = it->second->begin();
              item_it != it->second->end(); ++item_it ) {
          LOG_DEBUG << media_ << ": Remove crawler item: " << it->first
                   << " / " << item_it->first;
          end_tags->push_back(new streaming::OsdTag(
              0, streaming::kDefaultFlavourMask, 0,
              RemoveCrawlerItemParams(it->first, item_it->first)));
        }
      }
      if ( eos_clear_crawlers_ ) {
        LOG_DEBUG << media_ << ": Destroy crawler item: " << it->first;
        end_tags->push_back(new streaming::OsdTag(
            0, streaming::kDefaultFlavourMask, 0,
            DestroyCrawlerParams(it->first)));
      }
    }
  }
  if ( pip_ != NULL && eos_clear_pip_ ) {
    end_tags->push_back(new streaming::OsdTag(
        0, streaming::kDefaultFlavourMask, 0,
        SetPictureInPictureParams("", 0, 0, 0, 0, false)));
  }
  if ( click_url_ != NULL && eos_clear_click_url_ ) {
    end_tags->push_back(new streaming::OsdTag(
        0, streaming::kDefaultFlavourMask, 0,
        SetClickUrlParams("")));
  }
}

}
