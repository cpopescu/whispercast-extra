// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu
//
// Library class for osd-related elements
//
#ifndef __STREAMING_ELEMENTS_OSD_LIBRARY_OSD_LIBRARY_H__
#define __STREAMING_ELEMENTS_OSD_LIBRARY_OSD_LIBRARY_H__

#include <string>
#include <whisperstreamlib/elements/element_library.h>
#include "osd/osd_library/auto/osd_library_types.h"

namespace streaming {

class ServiceInvokerOsdLibraryServiceImpl;

class OsdElementLibrary : public ElementLibrary {
 public:
  OsdElementLibrary();
  virtual ~OsdElementLibrary();

  // Returns in element_types the ids for the type of elements that are
  // exported through this library
  virtual void GetExportedElementTypes(vector<string>* ellement_types);

  // Returns in policy_types the ids for the type of policies that are
  // exported through this library
  virtual void GetExportedPolicyTypes(vector<string>* policy_types);

  // Returns in authorizer_types the ids for the type of authorizers that are
  // exported through this library
  virtual void GetExportedAuthorizerTypes(vector<string>* authorizer_types) {
    return;
  }

  // Registers any global RPC-s necessary for the library
  virtual bool RegisterLibraryRpc(rpc::HttpServer* rpc_server,
                                  string* rpc_http_path);
  // The opposite of RegisterLibraryRpc
  virtual bool UnregisterLibraryRpc(rpc::HttpServer* rpc_server);

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to create an element of given type
  virtual int64 GetElementNeeds(const string& element_type);

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to a policy  of given type
  virtual int64 GetPolicyNeeds(const string& element_type);

  // Returns a combination of Needs (OR-ed), for what is needed in order
  // to create an authorizer of a given type
  virtual int64 GetAuthorizerNeeds(const string& policy_type) {
    return 0;
  }

  // Main creation function for an element.
  virtual streaming::Element* CreateElement(
      const string& element_type,
      const string& name,
      const string& element_params,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      vector<string>* needed_policies,
      string* error_description);
  // Main creation function for a policy.
  virtual streaming::Policy* CreatePolicy(
      const string& policy_type,
      const string& name,
      const string& policy_params,
      streaming::PolicyDrivenElement* element,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      string* error_description) ;
  // Main creation function for an authorizer
  virtual streaming::Authorizer* CreateAuthorizer(
      const string& authorizer_type,
      const string& name,
      const string& authorizer_params,
      const CreationObjectParams& params,
      // Output:
      string* error_description) {
    return NULL;
  }

 private:
  /////// Creation Helpers
  streaming::Element*
  CreateOsdDecoderElement(const string& name,
                          const OsdDecoderElementSpec& spec,
                          const streaming::Request* req,
                          const CreationObjectParams& params,
                          vector<string>* needed_policies,
                          string* error);
  streaming::Element*
  CreateOsdEncoderElement(const string& name,
                          const OsdEncoderElementSpec& spec,
                          const streaming::Request* req,
                          const CreationObjectParams& params,
                          vector<string>* needed_policies,
                          string* error);
  streaming::Element*
  CreateOsdInjectorElement(const string& name,
                           const OsdInjectorElementSpec& spec,
                           const streaming::Request* req,
                           const CreationObjectParams& params,
                           vector<string>* needed_policies,
                           string* error);
  streaming::Element*
  CreateOsdAssociatorElement(const string& name,
                             const OsdAssociatorElementSpec& spec,
                             const streaming::Request* req,
                             const CreationObjectParams& params,
                             vector<string>* needed_policies,
                             string* error);
  streaming::Element*
  CreateOsdStateKeeperElement(const string& name,
                              const OsdStateKeeperElementSpec& spec,
                              const streaming::Request* req,
                              const CreationObjectParams& params,
                              vector<string>* needed_policies,
                              string* error);
  streaming::Element*
  CreateOsdAssociatorClientElement(const string& name,
                                   const OsdAssociatorClientElementSpec& spec,
                                   const streaming::Request* req,
                                   const CreationObjectParams& params,
                                   vector<string>* needed_policies,
                                   string* error);

 private:
  ServiceInvokerOsdLibraryServiceImpl* rpc_impl_;
  DISALLOW_EVIL_CONSTRUCTORS(OsdElementLibrary);
};
}

#endif  // __STREAMING_ELEMENTS_OSD_LIBRARY_OSD_LIBRARY_H__
