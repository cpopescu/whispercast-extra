// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Catalin Popescu
//

#include <whisperlib/net/util/base64.h>
#include "extra_library/http_authorizer/http_authorizer.h"
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperstreamlib/elements/auto/factory_types.h>

namespace {
http::BaseClientConnection* CreateConnection(net::Selector* selector,
                                             net::NetFactory* net_factory,
                                             net::PROTOCOL net_protocol) {
  return new http::SimpleClientConnection(selector, *net_factory, net_protocol);
}
}
namespace streaming {

const char HttpAuthorizer::kAuthorizerClassName[] = "http_authorizer";

HttpAuthorizer::HttpAuthorizer(
    const string& name,
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
    int req_timeout_ms)
    : Authorizer(name),
      selector_(selector),
      net_factory_(selector_),
      failsafe_client_(NULL),
      query_path_format_(query_path_format),
      http_headers_(http_headers),
      body_format_lines_(body_format_lines),
      include_auth_headers_(include_auth_headers),
      success_body_(success_body),
      header_escapement_(header_escapement),
      body_escapement_(body_escapement),
      default_allowed_ms_(default_allowed_ms),
      parse_json_authorizer_reply_(parse_json_authorizer_reply) {
  client_params_.default_request_timeout_ms_ = req_timeout_ms;
  client_params_.max_concurrent_requests_ = max_concurrent_requests;
  connection_factory_ = NewPermanentCallback(&CreateConnection,
                                             selector, &net_factory_,
                                             net::PROTOCOL_TCP);
  CHECK(!servers.empty());
  failsafe_client_ = new http::FailSafeClient(
      selector,
      &client_params_,
      servers,
      connection_factory_,
      num_retries,
      req_timeout_ms,
      kReopenHttpConnectionIntervalMs,
      force_host_header);
}

HttpAuthorizer::~HttpAuthorizer()  {
  delete failsafe_client_;
  delete connection_factory_;
}

bool HttpAuthorizer::Initialize() {
  return true;
}

void HttpAuthorizer::Authorize(const AuthorizerRequest& areq,
                               CompletionCallback* completion) {
  http::ClientRequest* http_request = PrepareRequest(areq);
  active_.insert(completion);
  failsafe_client_->StartRequest(http_request,
      NewCallback(this, &HttpAuthorizer::RequestCallback,
          http_request, completion));
}
void HttpAuthorizer::Cancel(CompletionCallback* completion) {
  active_.erase(completion);
}

void HttpAuthorizer::EscapeMap(const map<string, string>& in,
                               map<string, string>* out,
                               Escapement esc_type) {
  switch ( esc_type ) {
    case ESC_NONE:
      for ( map<string, string>::const_iterator it = in.begin();
            it != in.end(); ++it ) {
        out->insert(make_pair(it->first, it->second));
      }
      break;
    case ESC_JSON:
      for ( map<string, string>::const_iterator it = in.begin();
            it != in.end(); ++it ) {
        out->insert(make_pair(it->first,
                              strutil::JsonStrEscape(it->second)));
      }
      break;
    case ESC_URL:
      for ( map<string, string>::const_iterator it = in.begin();
            it != in.end(); ++it ) {
        out->insert(make_pair(it->first,
                              URL::UrlEscape(it->second)));
      }
      break;
    case ESC_BASE64:
      for ( map<string, string>::const_iterator it = in.begin();
            it != in.end(); ++it ) {
        out->insert(make_pair(it->first,
                              base64::EncodeString(it->second)));
      }
      break;
  }
}

http::ClientRequest* HttpAuthorizer::PrepareRequest(const AuthorizerRequest& areq) {
  const http::HttpMethod http_method = body_format_lines_.empty()
                                       ? http::METHOD_GET : http::METHOD_POST;
  map<string, string> params;
  params["USER"] = areq.user_;
  params["PASSWD"] = areq.passwd_;
  params["TOKEN"] = areq.token_;
  params["ADDR"] = areq.net_address_;
  params["RES"] = areq.resource_;
  params["ACTION"] = areq.action_;
  params["USAGE_MS"] = strutil::StringPrintf(
      "%"PRId64"", areq.action_performed_ms_);

  map<string, string> url_esc_params;
  EscapeMap(params, &url_esc_params, ESC_URL);

  const string query_path = strutil::StrMapFormat(
      query_path_format_.c_str(), url_esc_params);

  http::ClientRequest* const r = new http::ClientRequest(http_method,
                                                         query_path);
  if ( !http_headers_.empty() ) {
    map<string, string> header_esc_params;
    EscapeMap(params, &header_esc_params, header_escapement_);
    for ( vector< pair<string, string> >::const_iterator
              it = http_headers_.begin();
          it != http_headers_.end(); ++it ) {
      r->request()->client_header()->AddField(
          it->first,
          strutil::StrMapFormat(it->second.c_str(), header_esc_params),
          true, true);
    }
  }
  if ( !body_format_lines_.empty() ) {
    map<string, string> body_esc_params;
    EscapeMap(params, &body_esc_params, header_escapement_);
    vector<string> body;
    for ( vector<string>::const_iterator it = body_format_lines_.begin();
          it != body_format_lines_.end(); ++it ) {
      body.push_back(strutil::StrMapFormat(it->c_str(),
                                           body_esc_params));
    }
    r->request()->client_data()->Write(strutil::JoinStrings(body, "\n"));
  }
  if ( include_auth_headers_ ) {
    r->request()->client_header()->SetAuthorizationField(areq.user_,
                                                         areq.passwd_);
  }
  // LOG_INFO << " Sending request: "
  //          << r->request()->client_header()->ToString()
  //          << "\n --- BODY: \n:"
  //          << r->request()->client_data()->ToString();;
  return r;
}

void HttpAuthorizer::RequestCallback(http::ClientRequest* http_req,
                                     CompletionCallback* completion) {
  /*
  LOG_INFO << " Authorization completed:\n "
           << http_req->request()->server_header()->ToString()
           << "\n --- BODY: \n:"
           << http_req->request()->server_data()->ToString()
           << "\n---- REQ:  "
           << http_req->request()->client_header()->ToString()
           << "\n---- BODY: "
           << http_req->request()->client_data()->ToString();
  */
  if ( active_.erase(completion) == 0 ) {
    // this authorization request got canceled in the meanwhile
    return;
  }

  AuthorizerReply reply(false, 0);
  if ( http_req->error() != http::CONN_OK && default_allowed_ms_ > 0 ) {
    reply = AuthorizerReply(true, default_allowed_ms_);
  } else if ( http_req->request()->server_header()->status_code() !=
      http::OK ) {
    // refuse
  } else if ( parse_json_authorizer_reply_ ) {
    const string body = http_req->request()->server_data()->ToString();
    RpcAuthorizerReply auth_reply;
    if ( rpc::JsonDecoder::DecodeObject(body, &auth_reply) ) {
      reply = AuthorizerReply(auth_reply.allowed_.get(),
                              auth_reply.time_limit_ms_.get());
    }
  } else if ( !success_body_.empty() ) {
    reply = AuthorizerReply(
        http_req->request()->server_data()->ToString() == success_body_,
        default_allowed_ms_);
  }
  completion->Run(reply);
}

}
