/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <thread>

#include "util/profiling.h"

namespace ccl {

Profiler::Profiler() : do_stop_worker(true) {}

Profiler::~Profiler()
{
  assert(worker == nullptr);
}

void Profiler::run()
{
  uint64_t updates = 0;
  auto start_time = std::chrono::system_clock::now();
  while (!do_stop_worker) {
    thread_scoped_lock lock(mutex);
    for (ProfilingState *state : states) {
      const uint32_t cur_event = state->event;
      const int32_t cur_shader = state->shader;
      const int32_t cur_object = state->object;

      /* The state reads/writes should be atomic, but just to be sure
       * check the values for validity anyways. */
      if (cur_event < PROFILING_NUM_EVENTS) {
        event_samples[cur_event]++;
      }

      if (cur_shader >= 0 && cur_shader < shader_samples.size()) {
        shader_samples[cur_shader]++;
      }

      if (cur_object >= 0 && cur_object < object_samples.size()) {
        object_samples[cur_object]++;
      }
    }
    lock.unlock();

    /* Relative waits always overshoot a bit, so just waiting 1ms every
     * time would cause the sampling to drift over time.
     * By keeping track of the absolute time, the wait times correct themselves -
     * if one wait overshoots a lot, the next one will be shorter to compensate. */
    updates++;
    std::this_thread::sleep_until(start_time + updates * std::chrono::milliseconds(1));
  }
}

void Profiler::reset(const int num_shaders, const int num_objects)
{
  const bool running = (worker != nullptr);
  if (running) {
    stop();
  }

  /* Resize and clear the accumulation vectors. */
  shader_hits.assign(num_shaders, 0);
  object_hits.assign(num_objects, 0);

  event_samples.assign(PROFILING_NUM_EVENTS, 0);
  shader_samples.assign(num_shaders, 0);
  object_samples.assign(num_objects, 0);

  if (running) {
    start();
  }
}

void Profiler::start()
{
  assert(worker == nullptr);
  do_stop_worker = false;
  worker = make_unique<thread>([this] { run(); });
}

void Profiler::stop()
{
  if (worker != nullptr) {
    do_stop_worker = true;

    worker->join();
    worker.reset();
  }
}

void Profiler::add_state(ProfilingState *state)
{
  const thread_scoped_lock lock(mutex);

  /* Add the ProfilingState from the list of sampled states. */
  assert(std::find(states.begin(), states.end(), state) == states.end());
  states.push_back(state);

  /* Resize thread-local hit counters. */
  state->shader_hits.assign(shader_hits.size(), 0);
  state->object_hits.assign(object_hits.size(), 0);

  /* Initialize the state. */
  state->event = PROFILING_UNKNOWN;
  state->shader = -1;
  state->object = -1;
  state->active = true;
}

void Profiler::remove_state(ProfilingState *state)
{
  const thread_scoped_lock lock(mutex);

  /* Remove the ProfilingState from the list of sampled states. */
  states.erase(std::remove(states.begin(), states.end(), state), states.end());
  state->active = false;

  /* Merge thread-local hit counters. */
  assert(shader_hits.size() == state->shader_hits.size());
  for (int i = 0; i < shader_hits.size(); i++) {
    shader_hits[i] += state->shader_hits[i];
  }

  assert(object_hits.size() == state->object_hits.size());
  for (int i = 0; i < object_hits.size(); i++) {
    object_hits[i] += state->object_hits[i];
  }
}

uint64_t Profiler::get_event(ProfilingEvent event)
{
  assert(worker == nullptr);
  return event_samples[event];
}

bool Profiler::get_shader(const int shader, uint64_t &samples, uint64_t &hits)
{
  assert(worker == nullptr);
  if (shader_samples[shader] == 0) {
    return false;
  }
  samples = shader_samples[shader];
  hits = shader_hits[shader];
  return true;
}

bool Profiler::get_object(const int object, uint64_t &samples, uint64_t &hits)
{
  assert(worker == nullptr);
  if (object_samples[object] == 0) {
    return false;
  }
  samples = object_samples[object];
  hits = object_hits[object];
  return true;
}

bool Profiler::active() const
{
  return (worker != nullptr);
}

}
