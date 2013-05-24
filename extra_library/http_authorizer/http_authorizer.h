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
class HttpAuthorizer : public Authorizer {
 public:
  enum Escapement {
    ESC_NONE   = 0,        // nothing baby
    ESC_URL    = 1,        // escape like in URLS (%xx)
    ESC_JSON   = 2,        // escape as a JSON stream
    ESC_BASE64 = 3,        // do a base64 on strings
  };
  static const char kAuthorizerClassName[];

  HttpAuthorizer(const string& name,
                 net::Selector* selector,
                 const vector<net::HostPort>& servers,
                 bool use_https,
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

  ////////////////////////////////////////////////
  // Authorizer methods
  virtual bool Initialize();
  virtual void Authorize(const AuthorizerRequest& req,
                         CompletionCallback* completion);
  virtual void Cancel(CompletionCallback* completion);

 private:
  void EscapeMap(const map<string, string>& in,
                 map<string, string>* out,
                 Escapement esc_type);
  http::ClientRequest* PrepareRequest(const AuthorizerRequest& req);
  void RequestCallback(http::ClientRequest* http_request,
                       CompletionCallback* completion);

  net::Selector* selector_;
  net::NetFactory net_factory_;
  http::FailSafeClient* failsafe_client_;

  // how to format the path for servers
  const string query_path_format_;
  // set this headers for all requests
  vector< pair<string, string> > http_headers_;
  // sending these lines. if these are specified we use POST
  vector<string> body_format_lines_;
  // Include Basic Authentication headers or cookie ?
  const bool include_auth_headers_;

  // If not empty, we look for this content in the answer,
  // else we just look at HTTP codes
  const string success_body_;

  // how to escape the header ?
  const Escapement header_escapement_;
  // how to escape the body ?
  const Escapement body_escapement_;
  // shall we allow operations for this long if we connect to (any) server ?
  // You don't want this normally..
  const int default_allowed_ms_;
  // If this is on we try to parse an RpcAuthorizerReply in the reply
  const bool parse_json_authorizer_reply_;

  static const int32 kReopenHttpConnectionIntervalMs = 2000;

  http::ClientParams client_params_;
  ResultClosure<http::BaseClientConnection*>* connection_factory_;

  // Active authorization requests (so we can cancel them).
  // Because FailSafeClient does not support Cancel() we this list.
  set<CompletionCallback*> active_;

  DISALLOW_EVIL_CONSTRUCTORS(HttpAuthorizer);
};
}
#endif   // __MEDIA_STANDARD_LIBRARY_HTTP_AUTHORIZER_H__
