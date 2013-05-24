// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Authors: Catalin Popescu
//


#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include  "features/decoding/feature_extractor.h"
#include  "features/util/jpeg_util.h"

DEFINE_string(flv_file,
              "",
              "The file to process");
DEFINE_string(feature_file,
              "",
              "We grep this feature in the flv file");

DEFINE_int64(start_timestamp_ms,
             0,
             "We start extracting features from this time");

DEFINE_int64(end_timestamp_ms,
             kMaxInt64,
             "We stop extracting features at this time");


DEFINE_double(max_C0,
              1.25,
              "Max distance in Y channel to consider a match");
DEFINE_double(max_C1,
              1.0,
              "Max distance in Pb channel to consider a match");
DEFINE_double(max_C2,
              1.0,
              "Max distance in Pr channel to consider a match");
DEFINE_double(max_C_all,
              1.5,
              "Max distance in Pr channel to consider a match");

DEFINE_double(region_C0,
              1.5,
              "Max distance in Y channel to consider a match");
DEFINE_double(region_C1,
              1.1,
              "Max distance in Pb channel to consider a match");
DEFINE_double(region_C2,
              1.1,
              "Max distance in Pr channel to consider a match");
DEFINE_double(region_C_all,
              1.7,
              "Max distance in Pr channel to consider a match");

DEFINE_bool(log_periodic_diffs,
            false,
            "Log periodicallyu something..");

DEFINE_int64(similarity_interval_ms,
             2000,
             "If we got a similar image at a time, do not output similar "
             "images that follow that soon");
DEFINE_string(match_dir,
              "",
              "If non empty we log the matching (or close matching) files");
static double glb_min_limit[feature::kNumFeatureChannels + 1];

static double glb_max_distance[feature::kNumFeatureChannels + 1];
static int last_match = -1;
static int64 last_similar_image = 0;
void FindFeature(feature::FeatureSet* to_find,
                 const feature::BitmapExtractor::PictureStruct* pic,
                 const feature::FeatureData* crt) {
  feature::MatchResult result = to_find->MatchNext(
      pic->timestamp_,
      crt,
      glb_max_distance,
      feature::SHORTEST_PATH);
  double distance[feature::kNumFeatureChannels + 1];
  if ( result == feature::NO_MATCH ||
       result == feature::MATCH_BROKEN ) {
    if ( FLAGS_log_periodic_diffs &&
         (last_similar_image == 0 ||
          (pic->timestamp_ - last_similar_image >
           FLAGS_similarity_interval_ms)) ) {
      const int ndx = result == feature::MATCH_BROKEN
                      ? last_match + 1 : 0;
      to_find->features()[ndx].second->ShortestPathDistance(crt, distance);
      if ( result == feature::MATCH_BROKEN ||
           (distance[0] < glb_min_limit[0] &&
            distance[1] < glb_min_limit[1] &&
            distance[2] < glb_min_limit[2] &&
            distance[3] < glb_min_limit[3]) ) {
        last_similar_image = pic->timestamp_;
        LOG_INFO << (result == feature::MATCH_BROKEN
                     ? string(" MATCH BROKEN @") : string(" NO MATCH @"))
                 << pic->timestamp_
                 << " seq: " << ndx << " of " << to_find->features().size()
                 << " Distance: " << distance[0]
                 << " / " << distance[1]
                 << " / " << distance[2]
                 << " / " << distance[3];
        //<< " \n "
        // << " CRT:  " << crt->ToString()
        // << " ndx: " << ndx
        // << " COMP: " << to_find->features()[ndx].second->ToString();
        if ( !FLAGS_match_dir.empty() ) {
          const string filename = strutil::JoinPaths(
              FLAGS_match_dir,
              strutil::StringPrintf("%s-%010"PRId64"-close_match.jpg",
                                    strutil::Basename(FLAGS_flv_file).c_str(),
                                    pic->timestamp_));
          CHECK(util::JpegSave(pic, 75, filename.c_str()));
        }
      }
    }
  } else if ( result == feature::MATCH_BEGIN ) {
    last_match = to_find->last_match();
    printf("MATCH_BEGIN: @ %"PRId64"\n",
           (pic->timestamp_));
    if ( !FLAGS_match_dir.empty() ) {
      const string filename = strutil::JoinPaths(
          FLAGS_match_dir,
          strutil::StringPrintf("%s-%010"PRId64"-match_begin.jpg",
                                strutil::Basename(FLAGS_flv_file).c_str(),
                                pic->timestamp_));
      CHECK(util::JpegSave(pic, 75, filename.c_str()));
    }
    last_match = to_find->last_match();
    if ( FLAGS_log_periodic_diffs ) {
      to_find->features()[last_match].second->ShortestPathDistance(
          crt, distance);
      LOG_INFO << " MATCH CONTBEGIN @"<< pic->timestamp_
               << " Distance: " << distance[0]
               << " / " << distance[1]
               << " / " << distance[2]
               << " / " << distance[3];
      // << " \n "
      // << " CRT:  " << crt->ToString()
      // << " COMP: " << to_find->features()[last_match].second->ToString();
    }
  } else if ( result == feature::MATCH_CONT ) {
    last_match = to_find->last_match();
  } else if ( result == feature::MATCH_COMPLETED ) {
    last_match = to_find->last_match();
    printf("MATCH_COMPLETED: @ %"PRId64"\n",
           (pic->timestamp_));
    if ( !FLAGS_match_dir.empty() ) {
      const string filename = strutil::JoinPaths(
          FLAGS_match_dir,
          strutil::StringPrintf("%s-%010"PRId64"-match_complete.jpg",
                                strutil::Basename(FLAGS_flv_file).c_str(),
                                pic->timestamp_));
      CHECK(util::JpegSave(pic, 75, filename.c_str()));
    }
  } else if ( result == feature::MATCH_OVER_TIME ) {
    printf("MATCH_OVER_TIME: @ %"PRId64"\n", pic->timestamp_);
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_flv_file.empty()) << " Please specify a flv file !";

  glb_max_distance[0] = FLAGS_max_C0;
  glb_max_distance[1] = FLAGS_max_C1;
  glb_max_distance[2] = FLAGS_max_C2;
  glb_max_distance[3] = FLAGS_max_C_all;

  glb_min_limit[0] = FLAGS_region_C0;
  glb_min_limit[1] = FLAGS_region_C1;
  glb_min_limit[2] = FLAGS_region_C2;
  glb_min_limit[3] = FLAGS_region_C_all;
  /* libavcodec */
#if LIBAVCODEC_VERSION_INT <= AV_VERSION_INT(53,34,0)
  avcodec_init();
#endif
  avcodec_register_all();

  feature::FeatureSet to_find;
  string content = io::FileInputStream::ReadFileOrDie(
      FLAGS_feature_file.c_str());
  io::MemoryStream ms;
  ms.Write(content);
  CHECK(to_find.Load(&ms) == feature::READ_OK);

  CHECK(!to_find.features().empty()) << " No feature in the set to find";

  // LOG_INFO << " Feature to find: " << to_find.ToString();

  feature::FlvFeatureExtractor extractor(
      FLAGS_flv_file, string(""),
      FLAGS_start_timestamp_ms, FLAGS_end_timestamp_ms,
      to_find.feature_width(),
      to_find.feature_height(),
      to_find.features()[0].second->x0(),
      to_find.features()[0].second->x1(),
      to_find.features()[0].second->y0(),
      to_find.features()[0].second->y1());

  if ( !extractor.ProcessFile(
           to_find.features()[0].second->feature_type(),
           to_find.features()[0].second->use_yuv(),
           to_find.features()[0].second->feature_size(),
           NULL,
           NewPermanentCallback(&FindFeature, &to_find)) ) {
    LOG_FATAL << " Error extracting features... ";
  }
  return 0;
}
