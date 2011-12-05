// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>

#include "osd/osd_library/osd_library.h"
#include "osd/osd_library/osd_decoder/osd_decoder_element.h"
#include "osd/osd_library/osd_injector/osd_injector_element.h"
#include "osd/osd_library/osd_associator/osd_associator_element.h"
#include "osd/osd_library/osd_encoder/osd_encoder_element.h"
#include "osd/osd_library/osd_state_keeper/osd_state_keeper_element.h"
#include "osd/osd_library/osd_remote_associator/osd_associator_client_element.h"

#include "osd/osd_library/auto/osd_library_invokers.h"

//////////////////////////////////////////////////////////////////////

// Library Stub

#ifdef __cplusplus
extern "C" {
#endif

  void* GetElementLibrary() {
    return new streaming::OsdElementLibrary();
  }

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////

namespace streaming {
OsdElementLibrary::OsdElementLibrary()
    : ElementLibrary("osd_library"),
      rpc_impl_(NULL) {
}

OsdElementLibrary::~OsdElementLibrary() {
  CHECK(rpc_impl_ == NULL)
      << " Deleting the library before unregistering the rpc";
}

void OsdElementLibrary::GetExportedElementTypes(vector<string>* element_types) {
  element_types->push_back(OsdDecoderElement::kElementClassName);
  element_types->push_back(OsdEncoderElement::kElementClassName);
  element_types->push_back(OsdInjectorElement::kElementClassName);
  element_types->push_back(OsdAssociatorElement::kElementClassName);
  element_types->push_back(OsdStateKeeperElement::kElementClassName);
  element_types->push_back(OsdAssociatorClientElement::kElementClassName);
}

void OsdElementLibrary::GetExportedPolicyTypes(vector<string>* policy_types) {
  // no policies
}


int64 OsdElementLibrary::GetElementNeeds(const string& element_type) {
  if ( element_type == OsdDecoderElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == OsdEncoderElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == OsdInjectorElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_RPC_SERVER);
  } else if ( element_type == OsdAssociatorElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_RPC_SERVER |
            NEED_STATE_KEEPER);
  } else if ( element_type == OsdStateKeeperElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_RPC_SERVER |
            NEED_STATE_KEEPER);
  } else if ( element_type == OsdAssociatorClientElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_RPC_SERVER |
            NEED_STATE_KEEPER);
  }
  LOG_FATAL << "Unknown element_type: " << element_type;
  return 0;
}

int64 OsdElementLibrary::GetPolicyNeeds(const string& policy_type) {
  return 0;    // no policies
}

streaming::Element* OsdElementLibrary::CreateElement(
    const string& element_type,
    const string& name,
    const string& element_params,
    const streaming::Request* req,
    const CreationObjectParams& params,
    // Output:
    vector<string>* needed_policies,
    string* error_description) {
  streaming::Element* ret = NULL;
  if ( element_type == OsdDecoderElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(OsdDecoder);
    return ret;
  } else if ( element_type == OsdEncoderElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(OsdEncoder);
    return ret;
  } else if ( element_type == OsdInjectorElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(OsdInjector);
    return ret;
  } else if ( element_type == OsdAssociatorElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(OsdAssociator);
    return ret;
  } else if ( element_type == OsdStateKeeperElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(OsdStateKeeper);
    return ret;
  } else if ( element_type == OsdAssociatorClientElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(OsdAssociatorClient);
    return ret;
  }
  LOG_ERROR << "Dunno how to create element of type: '" << element_type << "'";
  return NULL;
}


streaming::Policy* OsdElementLibrary::CreatePolicy(
      const string& policy_type,
      const string& name,
      const string& policy_params,
      streaming::PolicyDrivenElement* element,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      string* error_description) {
  return NULL;  // no policies
}

//////////////////////////////////////////////////////////////////////
//
// Element Creation functions:
//

///////  OsdDecoderElement

streaming::Element* OsdElementLibrary::CreateOsdDecoderElement(
    const string& element_name,
    const OsdDecoderElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  CHECK(mapper_ != NULL);
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new OsdDecoderElement(element_name.c_str(),
                               id.c_str(),
                               mapper_,
                               params.selector_);
}

///////  OsdEncoderElement

streaming::Element* OsdElementLibrary::CreateOsdEncoderElement(
    const string& element_name,
    const OsdEncoderElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  CHECK(mapper_ != NULL);
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new OsdEncoderElement(element_name.c_str(),
                               id.c_str(),
                               mapper_,
                               params.selector_);
}

///////  OsdInjectorElement
streaming::Element* OsdElementLibrary::CreateOsdInjectorElement(
    const string& element_name,
    const OsdInjectorElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  CHECK(mapper_ != NULL);
  string rpc_path(name() + "/osd_injector/");
  rpc_path += element_name + "/";
  string local_rpc_path(rpc_path);
  if ( req != NULL ) {
    local_rpc_path += req->info().GetPathId() + "/";
  }
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new OsdInjectorElement(element_name.c_str(),
                                id.c_str(),
                                mapper_,
                                params.selector_,
                                rpc_path.c_str(),
                                local_rpc_path.c_str(),
                                params.rpc_server_);
}

///////  OsdAssociatorElement
streaming::Element* OsdElementLibrary::CreateOsdAssociatorElement(
    const string& element_name,
    const OsdAssociatorElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  delete params.local_state_keeper_;
  if ( req != NULL ) {
    LOG_ERROR << " Cannot create temporary OsdAssociatorElement";
    delete params.state_keeper_;
    return NULL;
  }
  CHECK(mapper_ != NULL);
  string rpc_path(name() + "/osd_associator/");
  rpc_path += element_name + "/";
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new OsdAssociatorElement(element_name.c_str(),
                                  id.c_str(),
                                  mapper_,
                                  params.selector_,
                                  params.state_keeper_,
                                  rpc_path.c_str(),
                                  params.rpc_server_);
}


///////  OsdStateKeeperElement
streaming::Element* OsdElementLibrary::CreateOsdStateKeeperElement(
    const string& element_name,
    const OsdStateKeeperElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  CHECK(mapper_ != NULL);
  string rpc_path(name() + "/osd_state_keeper/");
  rpc_path += element_name + "/";
  if ( req != NULL ) {
    rpc_path += req->info().GetPathId() + "/";
  }
  io::StateKeepUser* state_keeper = NULL;
  delete params.state_keeper_;
  if ( spec.use_only_memory_.is_set() && spec.use_only_memory_.get() ) {
    delete params.local_state_keeper_;
  } else {
    state_keeper = params.local_state_keeper_;
    if ( state_keeper != NULL ) {
      const int64 to_ms = spec.state_timeout_sec_.is_set()
                          ? 1000 * spec.state_timeout_sec_
                          : static_cast<int64>(-1);
      state_keeper->set_timeout_ms(to_ms);
    }
  }
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new OsdStateKeeperElement(element_name.c_str(),
                                   id.c_str(),
                                   mapper_,
                                   params.selector_,
                                   state_keeper,
                                   spec.media_name_.c_str(),
                                   rpc_path.c_str(),
                                   params.rpc_server_);
}

///////  OsdAssociatorClientElement
streaming::Element* OsdElementLibrary::CreateOsdAssociatorClientElement(
    const string& element_name,
    const OsdAssociatorClientElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  delete params.local_state_keeper_;
  if ( req != NULL ) {
    LOG_ERROR << " Cannot create temporary OsdAssociatorClientElement";
    delete params.state_keeper_;
    return NULL;
  }
  CHECK(mapper_ != NULL);
  string rpc_path(name() + "/osd_associator_client/");
  rpc_path += element_name + "/";
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new OsdAssociatorClientElement(element_name.c_str(),
                                        id.c_str(),
                                        mapper_,
                                        params.selector_,
                                        params.state_keeper_,
                                        rpc_path.c_str(),
                                        params.rpc_server_);
}

//////////////////////////////////////////////////////////////////////

class ServiceInvokerOsdLibraryServiceImpl
    : public ServiceInvokerOsdLibraryService {
 public:
  ServiceInvokerOsdLibraryServiceImpl(
      streaming::ElementMapper* mapper,
      streaming::ElementLibrary::ElementSpecCreationCallback*
      create_element_spec_callback,
      streaming::ElementLibrary::PolicySpecCreationCallback*
      create_policy_spec_callback)
      : ServiceInvokerOsdLibraryService(
          ServiceInvokerOsdLibraryService::GetClassName()),
        mapper_(mapper),
        create_element_spec_callback_(create_element_spec_callback),
        create_policy_spec_callback_(create_policy_spec_callback) {
  }

  virtual void AddOsdDecoderElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const OsdDecoderElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(OsdDecoder);
  }
  virtual void AddOsdEncoderElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const OsdEncoderElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(OsdEncoder);
  }
  virtual void AddOsdInjectorElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const OsdInjectorElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(OsdInjector);
  }
  virtual void AddOsdAssociatorElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const OsdAssociatorElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(OsdAssociator);
  }
  virtual void AddOsdStateKeeperElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const OsdStateKeeperElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(OsdStateKeeper);
  }
  virtual void AddOsdAssociatorClientElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const OsdAssociatorClientElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(OsdAssociatorClient);
  }

 private:
  streaming::ElementMapper* mapper_;
  streaming::ElementLibrary::ElementSpecCreationCallback*
  create_element_spec_callback_;
  streaming::ElementLibrary::PolicySpecCreationCallback*
  create_policy_spec_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(ServiceInvokerOsdLibraryServiceImpl);
};

//////////////////////////////////////////////////////////////////////

bool OsdElementLibrary::RegisterLibraryRpc(rpc::HttpServer* rpc_server,
                                           string* rpc_http_path) {
  CHECK(rpc_impl_ == NULL);
  rpc_impl_ = new ServiceInvokerOsdLibraryServiceImpl(
      mapper_, create_element_spec_callback_, create_policy_spec_callback_);
  if ( !rpc_server->RegisterService(name() + "/", rpc_impl_) ) {
    delete rpc_impl_;
    return false;
  }
  *rpc_http_path = strutil::NormalizePath(
      rpc_server->path() + "/" + name() + "/" + rpc_impl_->GetName());
  return true;
}

bool OsdElementLibrary::UnregisterLibraryRpc(rpc::HttpServer* rpc_server) {
  CHECK(rpc_impl_ != NULL);
  const bool success = rpc_server->UnregisterService(name() + "/", rpc_impl_);
  delete rpc_impl_;
  rpc_impl_ = NULL;
  return success;
}
}
