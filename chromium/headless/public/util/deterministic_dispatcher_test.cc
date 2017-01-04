// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/deterministic_dispatcher.h"

#include <memory>
#include <string>
#include <vector>

#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "headless/public/util/testing/fake_managed_dispatch_url_request_job.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::ElementsAre;

namespace headless {

class DeterministicDispatcherTest : public ::testing::Test {
 protected:
  DeterministicDispatcherTest() {}
  ~DeterministicDispatcherTest() override {}

  void SetUp() override {
    deterministic_dispatcher_.reset(
        new DeterministicDispatcher(loop_.task_runner()));
  }

  base::MessageLoop loop_;
  std::unique_ptr<DeterministicDispatcher> deterministic_dispatcher_;
};

TEST_F(DeterministicDispatcherTest, DispatchDataReadyInReverseOrder) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 1,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 4,
                                           &notifications));
  job4->DispatchHeadersComplete();
  job3->DispatchHeadersComplete();
  job2->DispatchHeadersComplete();
  job1->DispatchHeadersComplete();

  EXPECT_TRUE(notifications.empty());

  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 1 OK"));

  job1.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 1 OK", "id: 2 OK"));

  job2.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 1 OK", "id: 2 OK", "id: 3 OK"));

  job3.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications,
              ElementsAre("id: 1 OK", "id: 2 OK", "id: 3 OK", "id: 4 OK"));
}

TEST_F(DeterministicDispatcherTest,
       ErrorsAndDataReadyDispatchedInCreationOrder) {
  std::vector<std::string> notifications;
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job1(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 1,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job2(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 2,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job3(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 3,
                                           &notifications));
  std::unique_ptr<FakeManagedDispatchURLRequestJob> job4(
      new FakeManagedDispatchURLRequestJob(deterministic_dispatcher_.get(), 4,
                                           &notifications));
  job4->DispatchHeadersComplete();
  job3->DispatchStartError(static_cast<net::Error>(-123));
  job2->DispatchHeadersComplete();
  job1->DispatchHeadersComplete();

  EXPECT_TRUE(notifications.empty());

  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 1 OK"));

  job1.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 1 OK", "id: 2 OK"));

  job2.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications,
              ElementsAre("id: 1 OK", "id: 2 OK", "id: 3 err: -123"));

  job3.reset();
  base::RunLoop().RunUntilIdle();
  EXPECT_THAT(notifications, ElementsAre("id: 1 OK", "id: 2 OK",
                                         "id: 3 err: -123", "id: 4 OK"));
}

}  // namespace headless
