// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_CLIENT_DISPATCHER_CLIENT_H_
#define UI_AURA_CLIENT_DISPATCHER_CLIENT_H_
#pragma once

#include "ui/aura/aura_export.h"
#include "base/message_loop.h"

namespace aura {

namespace client {

// An interface implemented by an object which handles nested dispatchers.
class AURA_EXPORT DispatcherClient {
 public:
  virtual void RunWithDispatcher(MessageLoop::Dispatcher* dispatcher,
                                 bool nestable_tasks_allowed) = 0;
};

AURA_EXPORT void SetDispatcherClient(DispatcherClient* client);
AURA_EXPORT DispatcherClient* GetDispatcherClient();

}  // namespace client
}  // namespace aura

#endif  // UI_AURA_CLIENT_DISPATCHER_CLIENT_H_
