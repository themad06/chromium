// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "net/quic/congestion_control/tcp_receiver.h"
#include "net/quic/test_tools/mock_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace test {

class QuicTcpReceiverTest : public ::testing::Test {
 protected:
  void SetUp() override { receiver_.reset(new TcpReceiver()); }
  scoped_ptr<TcpReceiver> receiver_;
};

TEST_F(QuicTcpReceiverTest, SimpleReceiver) {
  QuicTime timestamp(QuicTime::Zero());
  receiver_->RecordIncomingPacket(1, 1, timestamp);
}

}  // namespace test
}  // namespace net
