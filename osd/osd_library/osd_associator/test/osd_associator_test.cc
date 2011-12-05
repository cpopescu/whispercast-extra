// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include "osd/osd_library/osd_associator/osd_associator_element.h"

DEFINE_int32(rand_seed,
             3274,
             "Seed the random number generator w/ this");
DEFINE_int32(min_overlays_per_state,
             3,
             "We set at least these many overlays templates per state");
DEFINE_int32(max_overlays_per_state,
             8,
             "We set at most these many overlays templates per state");
DEFINE_int32(min_crawlers_per_state,
             3,
             "We set at least these many overlays templates per state");
DEFINE_int32(max_crawlers_per_state,
             8,
             "We set at most these many overlays templates per state");
DEFINE_int32(min_medias_per_state,
             1,
             "We set at least these many media paths per state");
DEFINE_int32(max_medias_per_state,
             1,
             "We set at most these many media paths per state");
DEFINE_int32(num_overlays,
             10,
             "We create these many possible overlays");
DEFINE_int32(num_crawlers,
             10,
             "We create these many possible crawlers");
DEFINE_int32(num_states,
             3,
             "We create and test these many states");
DEFINE_bool(destroy_osds_on_change,
            true,
            "Do we create our associated osds such that to destroy "
            "osds when changing the clip ?");

//////////////////////////////////////////////////////////////////////

static unsigned int rand_seed = 0;

inline int RandBetween(int min_val, int max_val) {
  if ( min_val == max_val ) return min_val;
  return min_val + rand_r(&rand_seed) % (max_val - min_val);
}

template<typename C>
void Choose(int num, set<C>* from, vector<C>* to) {
  CHECK_LE(num, from->size());
  while ( to->size() < num ) {
    const int which = rand_r(&rand_seed) % from->size();
    typename set<C>::iterator it = from->begin();
    for ( int i = 0; i < which; ++i ) {
      ++it;
    }
    to->push_back(*it);
    from->erase(it);
  }
}

template<typename C>
void Choose(int num, set<C>* from, set<C>* to) {
  CHECK_LE(num, from->size());
  while ( to->size() < num ) {
    const int which = rand_r(&rand_seed) % from->size();
    typename set<C>::iterator it = from->begin();
    for ( int i = 0; i < which; ++i ) {
      ++it;
    }
    to->insert(*it);
    from->erase(it);
  }
}

FullOsdState* BuildState(
    set<CreateOverlayParams*>* overlays,
    set<CreateCrawlerParams*>* crawlers) {

  const int num_o = RandBetween(FLAGS_min_overlays_per_state,
                                FLAGS_max_crawlers_per_state);
  const int num_c = RandBetween(FLAGS_min_overlays_per_state,
                                FLAGS_max_crawlers_per_state);
  // pull out stuff from overlays and crawlers
  set<CreateOverlayParams*> chosen_overlays;
  set<CreateCrawlerParams*> chosen_crawlers;
  Choose(num_o, overlays, &chosen_overlays);
  Choose(num_c, crawlers, &chosen_crawlers);

  FullOsdState* state = new FullOsdState();

  const int num_medias = RandBetween(FLAGS_min_medias_per_state,
                                     FLAGS_max_medias_per_state);
  for ( int i = 0; i < num_medias; ++i ) {
    AssociatedOsds osd;
    osd.media.Ref() = strutil::StringPrintf("%03d/", i);
    const int crt_num_o = rand_r(&rand_seed) % num_o + 1;
    const int crt_num_c = rand_r(&rand_seed) % num_c + 1;
    vector<CreateOverlayParams*> crt_overlays;
    vector<CreateCrawlerParams*> crt_crawlers;
    Choose(crt_num_o, &chosen_overlays, &crt_overlays);
    Choose(crt_num_c, &chosen_crawlers, &crt_crawlers);
    for ( int j = 0; j < crt_num_o; ++j ) {
      osd.overlays.Ref().PushBack(
          AddOverlayParams(
              crt_overlays[j]->id.Get(),
              rpc::String(strutil::StringPrintf(
                              "overlay-%s-%03d-%03d",
                              crt_overlays[j]->id.Get().CStr(), i, j))));
      chosen_overlays.insert(crt_overlays[j]);
    }
    for ( int j = 0; j < crt_num_c; ++j ) {
      CrawlerItem item;
      item.content.Ref() = strutil::StringPrintf(
          "item-%s-%03d-%03d",
          crt_crawlers[j]->id.Get().CStr(),
          i, j);
      osd.crawlers.Ref().PushBack(
          AddCrawlerItemParams(crt_crawlers[j]->id.Get(),
                               rpc::String(strutil::StringPrintf(
                                               "%03d-%03d", i, j)),
                               item));
      chosen_crawlers.insert(crt_crawlers[j]);
    }
    osd.eos_clear_overlays.Ref() = FLAGS_destroy_osds_on_change;
    osd.eos_clear_crawlers.Ref() = FLAGS_destroy_osds_on_change;
    osd.eos_clear_crawl_items.Ref() = FLAGS_destroy_osds_on_change;
    osd.eos_clear_pip.Ref() = FLAGS_destroy_osds_on_change;

    state->osds.Ref().PushBack(osd);
  }

  // undo the changes to overlays and crawlers
  for ( set<CreateOverlayParams*>::const_iterator it = chosen_overlays.begin();
        it != chosen_overlays.end(); ++it ) {
    state->overlays.Ref().PushBack(**it);
    overlays->insert(*it);
  }
  for ( set<CreateCrawlerParams*>::const_iterator it = chosen_crawlers.begin();
        it != chosen_crawlers.end(); ++it ) {
    state->crawlers.Ref().PushBack(**it);
    crawlers->insert(*it);
  }
  return state;
}

typedef list<const streaming::Tag*> TagList;

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  rand_seed = FLAGS_rand_seed;
  srand(FLAGS_rand_seed);
  CHECK_LE(FLAGS_min_crawlers_per_state,
           FLAGS_max_crawlers_per_state);
  CHECK_LE(FLAGS_min_overlays_per_state,
           FLAGS_max_overlays_per_state);
  CHECK_LE(FLAGS_max_crawlers_per_state,
           FLAGS_num_crawlers);
  CHECK_LE(FLAGS_max_overlays_per_state,
           FLAGS_num_overlays);

  set<CreateOverlayParams*> overlays;
  set<CreateCrawlerParams*> crawlers;

  for ( int i = 0; i < FLAGS_num_overlays; ++i ) {
    CreateOverlayParams* crt = new CreateOverlayParams();
    crt->id.Ref() = strutil::StringPrintf("o-%03d", i);
    overlays.insert(crt);
  }

  for ( int i = 0; i < FLAGS_num_crawlers; ++i ) {
    CreateCrawlerParams* crt = new CreateCrawlerParams();
    crt->id.Ref() = strutil::StringPrintf("c-%03d", i);
    crawlers.insert(crt);
  }

  vector<FullOsdState*> full_states;
  for ( int i = 0; FLAGS_num_states > i; ++i ) {
    FullOsdState* state = BuildState(&overlays,
                                     &crawlers);
    string s;
    rpc::JsonEncoder::EncodeToString(*state, &s);
    full_states.push_back(state);
    LOG_INFO << " State built:\n" << s;
  }
  streaming::OsdAssociatorElement element("test", "test", NULL, NULL, NULL, "", NULL);
  vector<streaming::FilteringCallbackData*> media_tests;
  vector<streaming::Request*> requests;
  for ( int i = 0; i < FLAGS_max_medias_per_state; ++i ) {
    const string path(strutil::StringPrintf("%03d/", i));
    LOG_INFO << " Adding callback on path: " << path;
    streaming::Request* req = new streaming::Request();
    streaming::FilteringCallbackData* cd =
        element.CreateCallbackData(path.c_str(), req);
    CHECK(cd != NULL);
    media_tests.push_back(cd);
    requests.push_back(req);
  }

  for ( int i = 0; i < FLAGS_num_states; ++i ) {
    string s;
    rpc::JsonEncoder::EncodeToString(*full_states[i], &s);
    printf("================================================== SET STATE:\n");
    printf("%s\n", s.c_str());
    element.SetFullState(full_states[i], false);
    for ( int j = 0; j < FLAGS_max_medias_per_state; ++j ) {
      TagList crt;
      crt.push_back(
          new streaming::Tag(new streaming::SimpleTag(""),
                             0, streaming::UNKNOWN_STREAM_TYPE,
                             streaming::Tag::VIDEO_TAG,
                             streaming::kDefaultFlavourMask,
                             i, 0));
      media_tests[j]->FilterTag(&crt, NULL);
      printf("--------------------\n");
      printf("STATE: %d, MEDIA: %d\n",  i, j);
      int k = 0;
      for ( TagList::const_iterator it = crt.begin(); it != crt.end(); ++it ) {
        if ( (*it)->tag_type() == streaming::TYPE_OSD ) {
          const streaming::OsdTag* const osd =
              reinterpret_cast<const streaming::OsdTag*>((*it)->data());
          printf("%03d >>> %s\n", k, osd->ToString().c_str());
          ++k;
        }
      }
    }
  }

  LOG_INFO << " SOMETHING";
  for ( int i = 0; i < FLAGS_max_medias_per_state; ++i ) {
    element.DeleteCallbackData(media_tests[i]);
    delete requests[i];
  }

  for ( set<CreateOverlayParams*>::const_iterator it = overlays.begin();
        it != overlays.end(); ++it ) {
    delete *it;
  }
  for ( set<CreateCrawlerParams*>::const_iterator it = crawlers.begin();
        it != crawlers.end(); ++it ) {
    delete *it;
  }
  for ( int i = 0; FLAGS_num_states > i; ++i ) {
    delete full_states[i];
  }
}
