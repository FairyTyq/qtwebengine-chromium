// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/metrics/child_call_stack_profile_collector.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "services/shell/public/cpp/interface_provider.h"

namespace metrics {

ChildCallStackProfileCollector::ProfilesState::ProfilesState() = default;
ChildCallStackProfileCollector::ProfilesState::ProfilesState(
    const ProfilesState&) = default;

ChildCallStackProfileCollector::ProfilesState::ProfilesState(
    const CallStackProfileParams& params,
    base::TimeTicks start_timestamp,
    const base::StackSamplingProfiler::CallStackProfiles& profiles)
    : params(params), start_timestamp(start_timestamp), profiles(profiles) {}

ChildCallStackProfileCollector::ProfilesState::~ProfilesState() = default;

ChildCallStackProfileCollector::ChildCallStackProfileCollector() {}

ChildCallStackProfileCollector::~ChildCallStackProfileCollector() {}

base::StackSamplingProfiler::CompletedCallback
ChildCallStackProfileCollector::GetProfilerCallback(
    const CallStackProfileParams& params) {
  return base::Bind(&ChildCallStackProfileCollector::Collect,
                    // This class has lazy instance lifetime.
                    base::Unretained(this), params,
                    base::TimeTicks::Now());
}

void ChildCallStackProfileCollector::SetParentProfileCollector(
    metrics::mojom::CallStackProfileCollectorPtr parent_collector) {
  base::AutoLock alock(lock_);
  // This function should only invoked once, during the mode of operation when
  // retaining profiles after construction.
  DCHECK(retain_profiles_);
  retain_profiles_ = false;
  task_runner_ = base::ThreadTaskRunnerHandle::Get();
  parent_collector_ = std::move(parent_collector);
  if (parent_collector_) {
    for (const ProfilesState& state : profiles_) {
      parent_collector_->Collect(state.params, state.start_timestamp,
                                  state.profiles);
    }
  }
  profiles_.clear();
}

void ChildCallStackProfileCollector::Collect(
    const CallStackProfileParams& params,
    base::TimeTicks start_timestamp,
    const std::vector<CallStackProfile>& profiles) {
  base::AutoLock alock(lock_);
  if (task_runner_ &&
      // The profiler thread does not have a task runner. Attempting to
      // invoke Get() on it results in a DCHECK.
      (!base::ThreadTaskRunnerHandle::IsSet() ||
       base::ThreadTaskRunnerHandle::Get() != task_runner_)) {
    // Post back to the thread that owns the the parent interface.
    task_runner_->PostTask(FROM_HERE, base::Bind(
        &ChildCallStackProfileCollector::Collect,
        // This class has lazy instance lifetime.
        base::Unretained(this),
        params,
        start_timestamp,
        profiles));
    return;
  }

  if (parent_collector_) {
    parent_collector_->Collect(params, start_timestamp, profiles);
  } else if (retain_profiles_) {
    profiles_.push_back(ProfilesState(params, start_timestamp, profiles));
  }
}

}  // namespace metrics
