/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#pragma once

#include "util/thread.h"

namespace ccl {

/* Counting Semaphore
 *
 * To restrict concurrent access to a resource to a specified number
 * of threads. Similar to std::counting_semaphore from C++20. */

class thread_counting_semaphore {
 public:
  explicit thread_counting_semaphore(const int count) : count(count) {}

  thread_counting_semaphore(const thread_counting_semaphore &) = delete;

  void acquire()
  {
    thread_scoped_lock lock(mutex);
    while (count == 0) {
      condition.wait(lock);
    }
    count--;
  }

  void release()
  {
    const thread_scoped_lock lock(mutex);
    count++;
    condition.notify_one();
  }

 protected:
  thread_mutex mutex;
  thread_condition_variable condition;
  int count;
};

}
