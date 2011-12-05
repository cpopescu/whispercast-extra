#ifndef COMMON_H_
#define COMMON_H_

#include <string>
#include "net/base/address.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/codec/rpc_codec_id.h"
#include "constants.h"

namespace manager {

// Parses a filename with a numeric short name.
// Returns the first number in file short name, or -1 on failure.
// e.g. "/tmp/123.avi" => 123
//      "/tmp/123.avi.bak" => 123
//      "/tmp/123.321.avi" => 123
//      "321.flv" => 321
//      "/tmp/456" => 456
//      "456" => 456
//      "/tmp/abc.avi" => -1
//      "123-23.flv" => -1
//      "" => -1
int64 ReadIdFromFilename(const std::string & filename);

// Returns the relative path of "big_path" to "sub_path".
// Or "big_path" if the "big_path" does not contain the "sub_path".
// e.g.
//  Good:
//    RelativePath("/tmp/a/b/x.avi", "/tmp") => "a/b/x.avi"
//    RelativePath("/tmp/a/b", "/tmp/a") => "b"
//    RelativePath("/tmp/a/b/", "/tmp/a/") => "b"
//    RelativePath("/tmp/a/b", "/tmp/a/b") => ""
//  Bad:
//    RelativePath("/tmp/a/b", "/tmp/a/b/x") => "/tmp/a/b"
//    RelativePath("/tmp/ab", "/tmp/a") => "/tmp/ab"
//    RelativePath("a/b/x.avi", "/tmp") => "a/b/x.avi"
//    RelativePath("/tmp/x.avi", "/home") => "/tmp/x.avi"
std::string RelativePath(const std::string & big_path,
                         const std::string & sub_path);

// src [IN]: the input file
// dst [IN/OUT]: contains the suggested output filename
//               returns the chosen filename
// max_retry [IN]: how many different names to try for 'dst'
// Returns success.
//
// e.g. dst = "b.txt"
//      Tries to create file "b.txt",
//      If that doesn't succeed, it tries "b(1).txt", "b(2).txt", ...
// e.g. dst = "b"
//      Tries: "b", "b(1)", "b(2)", ... , "b(max_retry)"
// Returns success and the chosen destination name (e.g. "b(7).txt") in 'dst';
//
bool TryRename(const std::string& src,
               std::string& dst,
               uint32 max_retry);

// username: some username
// ip: ip address
// path: path
// Returns: a string usable for SCP transfers, either src or dst.
//
// e.g. for username: whispercast
//          ip: 10.16.200.2
//          path: /tmp/a/b/x.flv
//  returns:
//    "whispercast@10.16.200.2:/tmp/a/b/x.flv
std::string MakeScpPath(const std::string& username,
                        const net::IpAddress& ip,
                        const std::string& path);
// Test if the given 'path' has format "user@host:path"
bool IsScpPath(const std::string& path);

// returns the first external IP address found.
//int32 ExternalIpAddress();

};

#endif /*COMMON_H_*/
