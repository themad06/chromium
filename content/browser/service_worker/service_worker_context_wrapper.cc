// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_context_wrapper.h"

#include "base/files/file_path.h"
#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_registration.h"
#include "content/public/browser/browser_thread.h"
#include "ipc/ipc_message.h"
#include "webkit/browser/quota/quota_manager_proxy.h"

namespace {

using content::BrowserThread;
using content::SERVICE_WORKER_OK;
using content::ServiceWorkerContext;
using content::ServiceWorkerRegistration;
using content::ServiceWorkerStatusCode;

void FinishRegistrationOnIO(
    const ServiceWorkerContext::ResultCallback& continuation,
    ServiceWorkerStatusCode status,
    int64 registration_id,
    int64 version_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(continuation, status == SERVICE_WORKER_OK));
}

void PostResultToUIFromStatusOnIO(
    const ServiceWorkerContext::ResultCallback& continuation,
    ServiceWorkerStatusCode status) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!continuation.is_null()) {
    BrowserThread::PostTask(
        BrowserThread::UI,
        FROM_HERE,
        base::Bind(continuation, status == SERVICE_WORKER_OK));
  }
}

void SendMessageAfterFind(
    const IPC::Message& message,
    const ServiceWorkerContext::ResultCallback& callback,
    ServiceWorkerStatusCode status,
    const scoped_refptr<ServiceWorkerRegistration>& registration) {
  if (status != SERVICE_WORKER_OK || !registration->active_version()) {
    callback.Run(false);
    return;
  }
  registration->active_version()->SendMessage(
      message, base::Bind(&PostResultToUIFromStatusOnIO, callback));
}

void PostMessageToUIFromStatusMessageOnIO(
    const ServiceWorkerContext::MessageCallback& continuation,
    ServiceWorkerStatusCode status,
    const IPC::Message& message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(continuation, status == SERVICE_WORKER_OK, message));
}

void SendMessageAndRegisterAfterFind(
    const IPC::Message& message,
    const ServiceWorkerContext::MessageCallback& callback,
    ServiceWorkerStatusCode status,
    const scoped_refptr<ServiceWorkerRegistration>& registration) {
  if (status != SERVICE_WORKER_OK || !registration->active_version()) {
    callback.Run(false, IPC::Message());
    return;
  }
  registration->active_version()->SendMessageAndRegisterCallback(
      message, base::Bind(&PostMessageToUIFromStatusMessageOnIO, callback));
}

}  // namespace

namespace content {

ServiceWorkerContextWrapper::ServiceWorkerContextWrapper() {
}

ServiceWorkerContextWrapper::~ServiceWorkerContextWrapper() {
}

void ServiceWorkerContextWrapper::Init(
    const base::FilePath& user_data_directory,
    quota::QuotaManagerProxy* quota_manager_proxy) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ServiceWorkerContextWrapper::Init, this,
                   user_data_directory,
                   make_scoped_refptr(quota_manager_proxy)));
    return;
  }
  DCHECK(!context_core_);
  context_core_.reset(
      new ServiceWorkerContextCore(
          user_data_directory, quota_manager_proxy));
}

void ServiceWorkerContextWrapper::Shutdown() {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::Bind(&ServiceWorkerContextWrapper::Shutdown, this));
    return;
  }
  context_core_.reset();
}

ServiceWorkerContextCore* ServiceWorkerContextWrapper::context() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return context_core_.get();
}

void ServiceWorkerContextWrapper::RegisterServiceWorker(
    const GURL& pattern,
    const GURL& script_url,
    int source_process_id,
    const ResultCallback& continuation) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&ServiceWorkerContextWrapper::RegisterServiceWorker,
                   this,
                   pattern,
                   script_url,
                   source_process_id,
                   continuation));
    return;
  }

  context()->RegisterServiceWorker(
      pattern,
      script_url,
      source_process_id,
      NULL /* provider_host */,
      base::Bind(&FinishRegistrationOnIO, continuation));
}

void ServiceWorkerContextWrapper::UnregisterServiceWorker(
    const GURL& pattern,
    int source_process_id,
    const ResultCallback& continuation) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&ServiceWorkerContextWrapper::UnregisterServiceWorker,
                   this,
                   pattern,
                   source_process_id,
                   continuation));
    return;
  }

  context()->UnregisterServiceWorker(
      pattern,
      source_process_id,
      NULL /* provider_host */,
      base::Bind(&PostResultToUIFromStatusOnIO, continuation));
}

void ServiceWorkerContextWrapper::SendMessage(const GURL& pattern,
                                              const IPC::Message& message,
                                              const ResultCallback& callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&ServiceWorkerContextWrapper::SendMessage,
                   this,
                   pattern,
                   message,
                   callback));
    return;
  }

  context()->storage()->FindRegistrationForPattern(
      pattern, base::Bind(&SendMessageAfterFind, message, callback));
}

void ServiceWorkerContextWrapper::SendMessageAndRegisterCallback(
    const GURL& pattern,
    const IPC::Message& message,
    const MessageCallback& callback) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    BrowserThread::PostTask(
        BrowserThread::IO,
        FROM_HERE,
        base::Bind(&ServiceWorkerContextWrapper::SendMessageAndRegisterCallback,
                   this,
                   pattern,
                   message,
                   callback));
    return;
  }

  context()->storage()->FindRegistrationForPattern(
      pattern, base::Bind(&SendMessageAndRegisterAfterFind, message, callback));
}

}  // namespace content
