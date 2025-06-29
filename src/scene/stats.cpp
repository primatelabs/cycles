/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "scene/stats.h"
#include "scene/object.h"
#include "util/algorithm.h"

#include "util/string.h"

namespace ccl {

static int kIndentNumSpaces = 2;

/* Named size entry. */

namespace {

bool namedSizeEntryComparator(const NamedSizeEntry &a, const NamedSizeEntry &b)
{
  /* We sort in descending order. */
  return a.size > b.size;
}

bool namedTimeEntryComparator(const NamedTimeEntry &a, const NamedTimeEntry &b)
{
  /* We sort in descending order. */
  return a.time > b.time;
}

bool namedTimeSampleEntryComparator(const NamedNestedSampleStats &a,
                                    const NamedNestedSampleStats &b)
{
  return a.sum_samples > b.sum_samples;
}

bool namedSampleCountPairComparator(const NamedSampleCountPair &a, const NamedSampleCountPair &b)
{
  return a.samples > b.samples;
}

}  // namespace

NamedSizeEntry::NamedSizeEntry() : size(0) {}

NamedSizeEntry::NamedSizeEntry(const string &name, const size_t size) : name(name), size(size) {}

NamedTimeEntry::NamedTimeEntry() : time(0) {}

NamedTimeEntry::NamedTimeEntry(const string &name, const double time) : name(name), time(time) {}

/* Named size statistics. */

NamedSizeStats::NamedSizeStats() : total_size(0) {}

void NamedSizeStats::add_entry(const NamedSizeEntry &entry)
{
  total_size += entry.size;
  entries.push_back(entry);
}

string NamedSizeStats::full_report(const int indent_level)
{
  const string indent(indent_level * kIndentNumSpaces, ' ');
  const string double_indent = indent + indent;
  string result;
  result += string_printf("%sTotal memory: %s (%s)\n",
                          indent.c_str(),
                          string_human_readable_size(total_size).c_str(),
                          string_human_readable_number(total_size).c_str());
  sort(entries.begin(), entries.end(), namedSizeEntryComparator);
  for (const NamedSizeEntry &entry : entries) {
    result += string_printf("%s%-32s %s (%s)\n",
                            double_indent.c_str(),
                            entry.name.c_str(),
                            string_human_readable_size(entry.size).c_str(),
                            string_human_readable_number(entry.size).c_str());
  }
  return result;
}

string NamedTimeStats::full_report(const int indent_level)
{
  const string indent(indent_level * kIndentNumSpaces, ' ');
  const string double_indent = indent + indent;
  string result;
  result += string_printf("%sTotal time: %fs\n", indent.c_str(), total_time);
  sort(entries.begin(), entries.end(), namedTimeEntryComparator);
  for (const NamedTimeEntry &entry : entries) {
    result += string_printf(
        "%s%-40s %fs\n", double_indent.c_str(), entry.name.c_str(), entry.time);
  }
  return result;
}

/* Named time sample statistics. */

NamedNestedSampleStats::NamedNestedSampleStats() : self_samples(0), sum_samples(0) {}

NamedNestedSampleStats::NamedNestedSampleStats(const string &name, const uint64_t samples)
    : name(name), self_samples(samples), sum_samples(samples)
{
}

NamedNestedSampleStats &NamedNestedSampleStats::add_entry(const string &name_, uint64_t samples_)
{
  entries.push_back(NamedNestedSampleStats(name_, samples_));
  return entries[entries.size() - 1];
}

void NamedNestedSampleStats::update_sum()
{
  sum_samples = self_samples;
  for (NamedNestedSampleStats &entry : entries) {
    entry.update_sum();
    sum_samples += entry.sum_samples;
  }
}

string NamedNestedSampleStats::full_report(const int indent_level, uint64_t total_samples)
{
  update_sum();

  if (total_samples == 0) {
    total_samples = sum_samples;
  }

  const string indent(indent_level * kIndentNumSpaces, ' ');

  const double sum_percent = 100 * ((double)sum_samples) / total_samples;
  const double sum_seconds = sum_samples * 0.001;
  const double self_percent = 100 * ((double)self_samples) / total_samples;
  const double self_seconds = self_samples * 0.001;
  const string info = string_printf("%-32s: Total %3.2f%% (%.2fs), Self %3.2f%% (%.2fs)\n",
                                    name.c_str(),
                                    sum_percent,
                                    sum_seconds,
                                    self_percent,
                                    self_seconds);
  string result = indent + info;

  sort(entries.begin(), entries.end(), namedTimeSampleEntryComparator);
  for (NamedNestedSampleStats &entry : entries) {
    result += entry.full_report(indent_level + 1, total_samples);
  }
  return result;
}

/* Named sample count pairs. */

NamedSampleCountPair::NamedSampleCountPair(const string &name,
                                           const uint64_t samples,
                                           const uint64_t hits)
    : name(name), samples(samples), hits(hits)
{
}

NamedSampleCountStats::NamedSampleCountStats() = default;

void NamedSampleCountStats::add(const string &name, const uint64_t samples, const uint64_t hits)
{
  const entry_map::iterator entry = entries.find(name);
  if (entry != entries.end()) {
    entry->second.samples += samples;
    entry->second.hits += hits;
    return;
  }
  entries.emplace(name, NamedSampleCountPair(name, samples, hits));
}

string NamedSampleCountStats::full_report(const int indent_level)
{
  const string indent(indent_level * kIndentNumSpaces, ' ');

  vector<NamedSampleCountPair> sorted_entries;
  sorted_entries.reserve(entries.size());

  uint64_t total_hits = 0;
  uint64_t total_samples = 0;
  for (entry_map::const_reference entry : entries) {
    const NamedSampleCountPair &pair = entry.second;

    total_hits += pair.hits;
    total_samples += pair.samples;

    sorted_entries.push_back(pair);
  }
  const double avg_samples_per_hit = ((double)total_samples) / total_hits;

  sort(sorted_entries.begin(), sorted_entries.end(), namedSampleCountPairComparator);

  string result;
  for (const NamedSampleCountPair &entry : sorted_entries) {
    const double seconds = entry.samples * 0.001;
    const double relative = ((double)entry.samples) / (entry.hits * avg_samples_per_hit);

    result += indent +
              string_printf(
                  "%-32s: %.2fs (Relative cost: %.2f)\n", entry.name.c_str(), seconds, relative);
  }
  return result;
}

/* Mesh statistics. */

MeshStats::MeshStats() = default;

string MeshStats::full_report(const int indent_level)
{
  const string indent(indent_level * kIndentNumSpaces, ' ');
  string result;
  result += indent + "Geometry:\n" + geometry.full_report(indent_level + 1);
  return result;
}

/* Image statistics. */

ImageStats::ImageStats() = default;

string ImageStats::full_report(const int indent_level)
{
  const string indent(indent_level * kIndentNumSpaces, ' ');
  string result;
  result += indent + "Textures:\n" + textures.full_report(indent_level + 1);
  return result;
}

/* Overall statistics. */

RenderStats::RenderStats()
{
  has_profiling = false;
}

void RenderStats::collect_profiling(Scene *scene, Profiler &prof)
{
  has_profiling = true;

  kernel = NamedNestedSampleStats("Total render time", prof.get_event(PROFILING_UNKNOWN));
  kernel.add_entry("Ray setup", prof.get_event(PROFILING_RAY_SETUP));
  kernel.add_entry("Intersect Closest", prof.get_event(PROFILING_INTERSECT_CLOSEST));
  kernel.add_entry("Intersect Shadow", prof.get_event(PROFILING_INTERSECT_SHADOW));
  kernel.add_entry("Intersect Subsurface", prof.get_event(PROFILING_INTERSECT_SUBSURFACE));
  kernel.add_entry("Intersect Volume Stack", prof.get_event(PROFILING_INTERSECT_VOLUME_STACK));
  kernel.add_entry("Intersect Blocked Light", prof.get_event(PROFILING_INTERSECT_DEDICATED_LIGHT));

  NamedNestedSampleStats &surface = kernel.add_entry("Shade Surface", 0);
  surface.add_entry("Setup", prof.get_event(PROFILING_SHADE_SURFACE_SETUP));
  surface.add_entry("Shader Evaluation", prof.get_event(PROFILING_SHADE_SURFACE_EVAL));
  surface.add_entry("Render Passes", prof.get_event(PROFILING_SHADE_SURFACE_PASSES));
  surface.add_entry("Direct Light", prof.get_event(PROFILING_SHADE_SURFACE_DIRECT_LIGHT));
  surface.add_entry("Indirect Light", prof.get_event(PROFILING_SHADE_SURFACE_INDIRECT_LIGHT));
  surface.add_entry("Ambient Occlusion", prof.get_event(PROFILING_SHADE_SURFACE_AO));

  NamedNestedSampleStats &volume = kernel.add_entry("Shade Volume", 0);
  volume.add_entry("Setup", prof.get_event(PROFILING_SHADE_VOLUME_SETUP));
  volume.add_entry("Integrate", prof.get_event(PROFILING_SHADE_VOLUME_INTEGRATE));
  volume.add_entry("Direct Light", prof.get_event(PROFILING_SHADE_VOLUME_DIRECT_LIGHT));
  volume.add_entry("Indirect Light", prof.get_event(PROFILING_SHADE_VOLUME_INDIRECT_LIGHT));

  NamedNestedSampleStats &shadow = kernel.add_entry("Shade Shadow", 0);
  shadow.add_entry("Setup", prof.get_event(PROFILING_SHADE_SHADOW_SETUP));
  shadow.add_entry("Surface", prof.get_event(PROFILING_SHADE_SHADOW_SURFACE));
  shadow.add_entry("Volume", prof.get_event(PROFILING_SHADE_SHADOW_VOLUME));
  shadow.add_entry("Blocked Light", prof.get_event(PROFILING_SHADE_DEDICATED_LIGHT));

  NamedNestedSampleStats &light = kernel.add_entry("Shade Light", 0);
  light.add_entry("Setup", prof.get_event(PROFILING_SHADE_LIGHT_SETUP));
  light.add_entry("Shader Evaluation", prof.get_event(PROFILING_SHADE_LIGHT_EVAL));

  shaders.entries.clear();
  for (Shader *shader : scene->shaders) {
    uint64_t samples;
    uint64_t hits;
    if (prof.get_shader(shader->id, samples, hits)) {
      shaders.add(shader->name, samples, hits);
    }
  }

  objects.entries.clear();
  for (Object *object : scene->objects) {
    uint64_t samples;
    uint64_t hits;
    if (prof.get_object(object->get_device_index(), samples, hits)) {
      objects.add(object->name, samples, hits);
    }
  }
}

string RenderStats::full_report()
{
  string result;
  result += "Mesh statistics:\n" + mesh.full_report(1);
  result += "Image statistics:\n" + image.full_report(1);
  if (has_profiling) {
    result += "Kernel statistics:\n" + kernel.full_report(1);
    result += "Shader statistics:\n" + shaders.full_report(1);
    result += "Object statistics:\n" + objects.full_report(1);
  }
  else {
    result += "Profiling information not available (only works with CPU rendering)";
  }
  return result;
}

NamedTimeStats::NamedTimeStats() : total_time(0.0) {}

string UpdateTimeStats::full_report(const int indent_level)
{
  return times.full_report(indent_level + 1);
}

SceneUpdateStats::SceneUpdateStats() = default;

string SceneUpdateStats::full_report()
{
  string result;
  result += "Scene:\n" + scene.full_report(1);
  result += "Geometry:\n" + geometry.full_report(1);
  result += "Light:\n" + light.full_report(1);
  result += "Object:\n" + object.full_report(1);
  result += "Image:\n" + image.full_report(1);
  result += "Background:\n" + background.full_report(1);
  result += "Bake:\n" + bake.full_report(1);
  result += "Camera:\n" + camera.full_report(1);
  result += "Film:\n" + film.full_report(1);
  result += "Integrator:\n" + integrator.full_report(1);
  result += "OSL:\n" + osl.full_report(1);
  result += "Particles:\n" + particles.full_report(1);
  result += "SVM:\n" + svm.full_report(1);
  result += "Tables:\n" + tables.full_report(1);
  result += "Procedurals:\n" + procedurals.full_report(1);
  return result;
}

void SceneUpdateStats::clear()
{
  geometry.times.clear();
  image.times.clear();
  light.times.clear();
  object.times.clear();
  background.times.clear();
  bake.times.clear();
  camera.times.clear();
  film.times.clear();
  integrator.times.clear();
  osl.times.clear();
  particles.times.clear();
  scene.times.clear();
  svm.times.clear();
  tables.times.clear();
  procedurals.times.clear();
}

}
