// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPDATE_CLIENT_TEST_COMPONENT_PATCHER_UNITTEST_H_
#define COMPONENTS_UPDATE_CLIENT_TEST_COMPONENT_PATCHER_UNITTEST_H_

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "courgette/courgette.h"
#include "courgette/third_party/bsdiff.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace update_client {

class MockComponentPatcher;
class ReadOnlyTestInstaller;

const char binary_output_hash[] =
    "599aba6d15a7da390621ef1bacb66601ed6aed04dadc1f9b445dcfe31296142a";

class ComponentPatcherOperationTest : public testing::Test {
 public:
  ComponentPatcherOperationTest();
  ~ComponentPatcherOperationTest() override;

 protected:
  base::ScopedTempDir input_dir_;
  base::ScopedTempDir installed_dir_;
  base::ScopedTempDir unpack_dir_;
  scoped_ptr<ReadOnlyTestInstaller> installer_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

 private:
  base::MessageLoopForIO loop_;
};

}  // namespace update_client

#endif  // COMPONENTS_UPDATE_CLIENT_TEST_COMPONENT_PATCHER_UNITTEST_H_
