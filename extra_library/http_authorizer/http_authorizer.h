// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#ifndef __MEDIA_STANDARD_LIBRARY_HTTP_AUTHORIZER_H__
#define __MEDIA_STANDARD_LIBRARY_HTTP_AUTHORIZER_H__

#include <string>
#include <map>
#include <vector>

#include <whisperstreamlib/base/stream_auth.h>
#include <whisperlib/common/base/types.h>
#include <whisperlib/net/http/failsafe_http_client.h>

namespace streaming {

// This is an authorizer that asks extrenal http server(s) for permission.
//
// Parameters you can use in formatting url paths / bodies:
//  ${USER}    - user (as provided by request)
//  ${PASSWD}  - password (as provided by request)
//  ${TOKEN}   - the auth token (cookie) - if present
//  ${ADDR}    - host:port for the client
//  ${RES}     - the resource requested
//  ${ACTION}  - the action requested ('view' / 'publish')
//  ${USAGE_MS}- for how long the action is alreadi performed (for
//               re-authorization)
//
class HttpAuthorizer
    : public Authorizer {
 public:
  enum Escapement {
    ESC_NONE   = 0,        // nothing baby
    ESC_URL    = 1,        // escape like in URLS (%xx)
    ESC_JSON   = 2,        // escape as a JSON stream
    ESC_BASE64 = 3,        // do a base64 on strings
  };

  HttpAuthorizer(const char* name,
                 net::Selector* selector,
                 const vector<net::HostPort>& servers,
                 const string& query_path_format,
                 const vector< pair<string, string> >& http_headers,
                 const vector<string>& body_format_lines,
                 bool include_auth_headers,
                 const string& success_body,
                 Escapement header_escapement,
                 Escapement body_escapement,
                 int default_allowed_ms,
                 bool parse_json_authorizer_reply,
                 const string& force_host_header,
                 int num_retries,
                 int max_concurrent_requests,
                 int req_timeout_ms);
  ~HttpAuthorizer();

  virtual bool Initialize();
  virtual void Authorize(const AuthorizerRequest& req,
                         AuthorizerReply* reply,
                         Closure* completion);

  static const char kAuthorizerClassName[];
 private:
  struct ReqStruct {
    AuthorizerReply* reply_;
    Closure* completion_;
    http::ClientRequest* http_request_;
  };

  void EscapeMap(const map<string, string>& in,
                 map<string, string>* out,
                 Escapement esc_type);
  ReqStruct* PrepareRequest(const AuthorizerRequest& req,
                            AuthorizerReply* reply,
                            Closure* completion);
  void RequestCallback(ReqStruct* req);

  net::Selector* selector_;
  net::NetFactory net_factory_;
  http::FailSafeClient* failsafe_client_;

  const string query_path_format_;     // how to format the path for servers
  vector< pair<string, string> > http_headers_;
                                       // set this headers for all requests
  vector<string> body_format_lines_;   // sending these lines
                                       // if these are specified we use POST
  const bool include_auth_headers_;    // Include Basic Authentication headers
                                       // or cookie ?
  // bool receive_json_encoded_;
  const string success_body_;          // If not empty, we look for this
                                       // content in the answer, else we
                                       // just look at HTTP codes
  const Escapement header_escapement_; // how to escape the header ?
  const Escapement body_escapement_;   // how to escape the body ?
  const int default_allowed_ms_;       // shall we allow operations for this
                                       // long if we connect to (any) server ?
                                       // You don't want this normally..
  const bool parse_json_authorizer_reply_;
                                       // If this is on we try to parse
                                       // an RpcAuthorizerReply in the reply

  static const int32 kReopenHttpConnectionIntervalMs = 2000;

  http::ClientParams client_params_;
  ResultClosure<http::BaseClientConnection*>* connection_factory_;

  DISALLOW_EVIL_CONSTRUCTORS(HttpAuthorizer);
};
}
#endif   // __MEDIA_STANDARD_LIBRARY_HTTP_AUTHORIZER_H__
