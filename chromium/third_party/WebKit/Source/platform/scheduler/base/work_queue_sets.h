// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_WORK_QUEUE_SETS_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_WORK_QUEUE_SETS_H_

#include <stddef.h>

#include <map>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/trace_event/trace_event_argument.h"
#include "platform/scheduler/base/task_queue_impl.h"
#include "public/platform/WebCommon.h"

namespace blink {
namespace scheduler {
namespace internal {
class TaskQueueImpl;

// There is a WorkQueueSet for each scheduler priority and each WorkQueueSet
// uses a EnqueueOrderToWorkQueueMap to keep track of which queue in the set has
// the oldest task (i.e. the one that should be run next if the
// TaskQueueSelector chooses to run a task a given priority).  The reason this
// works is because std::map is a tree based associative container and all the
// values are kept in sorted order.
class BLINK_PLATFORM_EXPORT WorkQueueSets {
 public:
  WorkQueueSets(size_t num_sets, const char* name);
  ~WorkQueueSets();

  // O(log num queues)
  void AddQueue(WorkQueue* queue, size_t set_index);

  // O(log num queues)
  void RemoveQueue(WorkQueue* work_queue);

  // O(log num queues)
  void ChangeSetIndex(WorkQueue* queue, size_t set_index);

  // O(log num queues)
  void OnPushQueue(WorkQueue* work_queue);

  // If empty it's O(1) amortized, otherwise it's O(log num queues)
  // Assumes |work_queue| contains the lowest enqueue order in the set.
  void OnPopQueue(WorkQueue* work_queue);

  // O(1)
  bool GetOldestQueueInSet(size_t set_index, WorkQueue** out_work_queue) const;

  // O(1)
  bool IsSetEmpty(size_t set_index) const;

#if DCHECK_IS_ON() || !defined(NDEBUG)
  // Note this iterates over everything in |enqueue_order_to_work_queue_maps_|.
  // It's intended for use with DCHECKS and for testing
  bool ContainsWorkQueueForTest(const WorkQueue* queue) const;
#endif

  const char* name() const { return name_; }

 private:
  typedef std::map<EnqueueOrder, WorkQueue*> EnqueueOrderToWorkQueueMap;
  std::vector<EnqueueOrderToWorkQueueMap> enqueue_order_to_work_queue_maps_;
  const char* name_;

  DISALLOW_COPY_AND_ASSIGN(WorkQueueSets);
};

}  // namespace internal
}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_BASE_WORK_QUEUE_SETS_H_
