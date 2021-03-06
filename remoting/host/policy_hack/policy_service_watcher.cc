// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/policy_hack/policy_service_watcher.h"

#include "base/files/file_path.h"
#include "base/values.h"
#include "components/policy/core/common/async_policy_loader.h"
#include "components/policy/core/common/async_policy_provider.h"
#include "components/policy/core/common/policy_namespace.h"
#include "components/policy/core/common/policy_service_impl.h"
#include "components/policy/core/common/schema.h"
#include "components/policy/core/common/schema_registry.h"
#include "policy/policy_constants.h"

#if defined(OS_CHROMEOS)
#include "content/public/browser/browser_thread.h"
#elif defined(OS_WIN)
#include "components/policy/core/common/policy_loader_win.h"
#elif defined(OS_MACOSX)
#include "components/policy/core/common/policy_loader_mac.h"
#include "components/policy/core/common/preferences_mac.h"
#elif defined(OS_POSIX) && !defined(OS_ANDROID)
#include "components/policy/core/common/config_dir_policy_loader.h"
#endif

using namespace policy;

namespace remoting {
namespace policy_hack {

namespace {

PolicyNamespace GetPolicyNamespace() {
  return PolicyNamespace(POLICY_DOMAIN_CHROME, std::string());
}

}  // namespace

PolicyServiceWatcher::PolicyServiceWatcher(
    const scoped_refptr<base::SingleThreadTaskRunner>&
        policy_service_task_runner,
    PolicyService* policy_service)
    : PolicyWatcher(policy_service_task_runner) {
  policy_service_ = policy_service;
}

PolicyServiceWatcher::PolicyServiceWatcher(
    const scoped_refptr<base::SingleThreadTaskRunner>&
        policy_service_task_runner,
    scoped_ptr<PolicyService> owned_policy_service,
    scoped_ptr<ConfigurationPolicyProvider> owned_policy_provider,
    scoped_ptr<SchemaRegistry> owned_schema_registry)
    : PolicyWatcher(policy_service_task_runner),
      owned_schema_registry_(owned_schema_registry.Pass()),
      owned_policy_provider_(owned_policy_provider.Pass()),
      owned_policy_service_(owned_policy_service.Pass()) {
  policy_service_ = owned_policy_service_.get();
}

PolicyServiceWatcher::~PolicyServiceWatcher() {
  if (owned_policy_provider_) {
    owned_policy_provider_->Shutdown();
  }
}

void PolicyServiceWatcher::OnPolicyUpdated(const PolicyNamespace& ns,
                                           const PolicyMap& previous,
                                           const PolicyMap& current) {
  scoped_ptr<base::DictionaryValue> policy_dict(new base::DictionaryValue());
  for (PolicyMap::const_iterator it = current.begin(); it != current.end();
       it++) {
    // TODO(lukasza): Use policy::Schema::Normalize() for schema verification.
    policy_dict->Set(it->first, it->second.value->DeepCopy());
  }
  UpdatePolicies(policy_dict.get());
}

void PolicyServiceWatcher::OnPolicyServiceInitialized(PolicyDomain domain) {
  PolicyNamespace ns = GetPolicyNamespace();
  const PolicyMap& current = policy_service_->GetPolicies(ns);
  OnPolicyUpdated(ns, current, current);
}

void PolicyServiceWatcher::StartWatchingInternal() {
  // Listen for future policy changes.
  policy_service_->AddObserver(POLICY_DOMAIN_CHROME, this);

  // Process current policy state.
  if (policy_service_->IsInitializationComplete(POLICY_DOMAIN_CHROME)) {
    OnPolicyServiceInitialized(POLICY_DOMAIN_CHROME);
  }
}

void PolicyServiceWatcher::StopWatchingInternal() {
  policy_service_->RemoveObserver(POLICY_DOMAIN_CHROME, this);
}

scoped_ptr<PolicyServiceWatcher> PolicyServiceWatcher::CreateFromPolicyLoader(
    const scoped_refptr<base::SingleThreadTaskRunner>&
        policy_service_task_runner,
    scoped_ptr<AsyncPolicyLoader> async_policy_loader) {
  // TODO(lukasza): Schema below should ideally only cover Chromoting-specific
  // policies (expecting perf and maintanability improvement, but no functional
  // impact).
  Schema schema = Schema::Wrap(GetChromeSchemaData());

  scoped_ptr<SchemaRegistry> schema_registry(new SchemaRegistry());
  schema_registry->RegisterComponent(GetPolicyNamespace(), schema);

  scoped_ptr<AsyncPolicyProvider> policy_provider(new AsyncPolicyProvider(
      schema_registry.get(), async_policy_loader.Pass()));
  policy_provider->Init(schema_registry.get());

  PolicyServiceImpl::Providers providers;
  providers.push_back(policy_provider.get());
  scoped_ptr<PolicyService> policy_service(new PolicyServiceImpl(providers));

  return make_scoped_ptr(new PolicyServiceWatcher(
      policy_service_task_runner, policy_service.Pass(), policy_provider.Pass(),
      schema_registry.Pass()));
}

scoped_ptr<PolicyWatcher> PolicyWatcher::Create(
    policy::PolicyService* policy_service,
    const scoped_refptr<base::SingleThreadTaskRunner>& network_task_runner) {
#if defined(OS_CHROMEOS)
  DCHECK(policy_service);
  return make_scoped_ptr(new PolicyServiceWatcher(
      content::BrowserThread::GetMessageLoopProxyForThread(
          content::BrowserThread::UI),
      policy_service));
#elif defined(OS_WIN)
  DCHECK(!policy_service);
  static const wchar_t kRegistryKey[] = L"SOFTWARE\\Policies\\Google\\Chrome";
  return PolicyServiceWatcher::CreateFromPolicyLoader(
      network_task_runner,
      PolicyLoaderWin::Create(network_task_runner, kRegistryKey));
#elif defined(OS_MACOSX)
  CFStringRef bundle_id = CFSTR("com.google.Chrome");
  DCHECK(!policy_service);
  return PolicyServiceWatcher::CreateFromPolicyLoader(
      network_task_runner,
      make_scoped_ptr(new PolicyLoaderMac(
          network_task_runner,
          policy::PolicyLoaderMac::GetManagedPolicyPath(bundle_id),
          new MacPreferences(), bundle_id)));
#elif defined(OS_POSIX) && !defined(OS_ANDROID)
  DCHECK(!policy_service);
  // Always read the Chrome policies (even on Chromium) so that policy
  // enforcement can't be bypassed by running Chromium.
  static const base::FilePath::CharType kPolicyDir[] =
      FILE_PATH_LITERAL("/etc/opt/chrome/policies");
  return PolicyServiceWatcher::CreateFromPolicyLoader(
      network_task_runner, make_scoped_ptr(new ConfigDirPolicyLoader(
                               network_task_runner, base::FilePath(kPolicyDir),
                               POLICY_SCOPE_MACHINE)));
#else
#error OS that is not yet supported by PolicyWatcher code.
#endif
}

}  // namespace policy_hack
}  // namespace remoting
