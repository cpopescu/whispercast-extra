// Copyright 2009 WhisperSoft s.r.l.
// Author: Catalin Popescu
//
// Library of extra(private) elements
//

#ifndef __STREAMING_ELEMENTS_EXTRA_LIBRARY_EXTRA_LIBRARY_H__
#define __STREAMING_ELEMENTS_EXTRA_LIBRARY_EXTRA_LIBRARY_H__

#include <string>
#include <whisperstreamlib/elements/element_library.h>
#include "extra_library/auto/extra_library_types.h"

namespace streaming {

class ServiceInvokerExtraLibraryServiceImpl;

class ExtraElementLibrary : public ElementLibrary {
 public:
  ExtraElementLibrary();
  virtual ~ExtraElementLibrary();

  // Returns in element_types the ids for the type of elements that are
  // exported through this library
  virtual void GetExportedElementTypes(vector<string>* ellement_types);

  // Returns in policy_types the ids for the type of policies that are
  // exported through this library
  virtual void GetExportedPolicyTypes(vector<string>* policy_types);

  // Returns in authorizer_types the ids for the type of authorizers that are
  // exported through this library
  virtual void GetExportedAuthorizerTypes(vector<string>* authorizer_types);

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
  virtual int64 GetAuthorizerNeeds(const string& policy_type);

  // Main creation function for an element.
  virtual streaming::Element* CreateElement(
      const string& element_type,
      const string& name,
      const string& element_params,
      const streaming::Request* req,
      const CreationObjectParams& params,
      bool is_temporary_template,
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
      string* error_description);
  // Main creation function for an authorizer
  virtual streaming::Authorizer* CreateAuthorizer(
      const string& authorizer_type,
      const string& name,
      const string& authorizer_params,
      const CreationObjectParams& params,
      // Output:
      string* error_description);

 private:
  /////// Creation Helpers
  streaming::Element*
  CreateFeatureDetectorElement(const string& name,
                               const FeatureDetectorElementSpec& spec,
                               const streaming::Request* req,
                               const CreationObjectParams& params,
                               vector<string>* needed_policies,
                               bool is_temporary_template,
                               string* error);
  streaming::Element*
  CreateTimeRangeElement(const string& name,
                         const TimeRangeElementSpec& spec,
                         const streaming::Request* req,
                         const CreationObjectParams& params,
                         vector<string>* needed_policies,
                         bool is_temporary_template,
                         string* error);
  streaming::Element*
  CreateTimeShiftElement(const string& name,
                         const TimeShiftElementSpec& spec,
                         const streaming::Request* req,
                         const CreationObjectParams& params,
                         vector<string>* needed_policies,
                         bool is_temporary_template,
                         string* error);
  streaming::Element*
  CreateFlavouringElement(const string& name,
                          const FlavouringElementSpec& spec,
                          const streaming::Request* req,
                          const CreationObjectParams& params,
                          vector<string>* needed_policies,
                          bool is_temporary_template,
                          string* error);
  streaming::Policy*
  CreateSchedulePlaylistPolicy(const string& name,
                               const SchedulePlaylistPolicySpec& spec,
                               streaming::PolicyDrivenElement* element,
                               const streaming::Request* req,
                               const CreationObjectParams& params,
                               string* error);
  streaming::Policy*
  CreateLogicPlaylistPolicy(const string& name,
                            const LogicPlaylistPolicySpec& spec,
                            streaming::PolicyDrivenElement* element,
                            const streaming::Request* req,
                            const CreationObjectParams& params,
                            string* error);
  streaming::Policy*
  CreateTimeDelayPolicy(const string& name,
                        const TimeDelayPolicySpec& spec,
                        streaming::PolicyDrivenElement* element,
                        const streaming::Request* req,
                        const CreationObjectParams& params,
                        string* error);
  streaming::Authorizer*
  CreateHttpAuthorizer(const string& name,
                       const HttpAuthorizerSpec& spec,
                       const CreationObjectParams& params,
                       string* error);

 private:
  ServiceInvokerExtraLibraryServiceImpl* rpc_impl_;
  DISALLOW_EVIL_CONSTRUCTORS(ExtraElementLibrary);
};
}

#endif  // __STREAMING_ELEMENTS_EXTRA_LIBRARY_EXTRA_LIBRARY_H__
