/* SPDX-FileCopyrightText: 2011-2022 Blender Foundation
 *
 * SPDX-License-Identifier: Apache-2.0 */

#include "bvh/bvh.h"

#include "device/device.h"

#include "scene/attribute.h"
#include "scene/camera.h"
#include "scene/geometry.h"
#include "scene/hair.h"
#include "scene/light.h"
#include "scene/mesh.h"
#include "scene/object.h"
#include "scene/osl.h"
#include "scene/pointcloud.h"
#include "scene/scene.h"
#include "scene/shader.h"
#include "scene/shader_nodes.h"

#include "util/progress.h"

namespace ccl {

void GeometryManager::device_update_mesh(Device * /*unused*/,
                                         DeviceScene *dscene,
                                         Scene *scene,
                                         Progress &progress)
{
  /* Count. */
  size_t vert_size = 0;
  size_t tri_size = 0;

  size_t curve_key_size = 0;
  size_t curve_size = 0;
  size_t curve_segment_size = 0;

  size_t point_size = 0;

  for (Geometry *geom : scene->geometry) {
    if (geom->is_mesh() || geom->is_volume()) {
      Mesh *mesh = static_cast<Mesh *>(geom);

      vert_size += mesh->verts.size();
      tri_size += mesh->num_triangles();
    }
    else if (geom->is_hair()) {
      Hair *hair = static_cast<Hair *>(geom);

      curve_key_size += hair->get_curve_keys().size();
      curve_size += hair->num_curves();
      curve_segment_size += hair->num_segments();
    }
    else if (geom->is_pointcloud()) {
      PointCloud *pointcloud = static_cast<PointCloud *>(geom);
      point_size += pointcloud->num_points();
    }
  }

  /* Fill in all the arrays. */
  if (tri_size != 0) {
    /* normals */
    progress.set_status("Updating Mesh", "Computing normals");

    packed_float3 *tri_verts = dscene->tri_verts.alloc(vert_size);
    uint *tri_shader = dscene->tri_shader.alloc(tri_size);
    packed_float3 *vnormal = dscene->tri_vnormal.alloc(vert_size);
    packed_uint3 *tri_vindex = dscene->tri_vindex.alloc(tri_size);

    const bool copy_all_data = dscene->tri_shader.need_realloc() ||
                               dscene->tri_vindex.need_realloc() ||
                               dscene->tri_vnormal.need_realloc();

    for (Geometry *geom : scene->geometry) {
      if (geom->is_mesh() || geom->is_volume()) {
        Mesh *mesh = static_cast<Mesh *>(geom);

        if (mesh->shader_is_modified() || mesh->smooth_is_modified() ||
            mesh->triangles_is_modified() || copy_all_data)
        {
          mesh->pack_shaders(scene, &tri_shader[mesh->prim_offset]);
        }

        if (mesh->verts_is_modified() || copy_all_data) {
          mesh->pack_normals(&vnormal[mesh->vert_offset]);
        }

        if (mesh->verts_is_modified() || mesh->triangles_is_modified() || copy_all_data) {
          mesh->pack_verts(&tri_verts[mesh->vert_offset], &tri_vindex[mesh->prim_offset]);
        }

        if (progress.get_cancel()) {
          return;
        }
      }
    }

    /* vertex coordinates */
    progress.set_status("Updating Mesh", "Copying Mesh to device");

    dscene->tri_verts.copy_to_device_if_modified();
    dscene->tri_shader.copy_to_device_if_modified();
    dscene->tri_vnormal.copy_to_device_if_modified();
    dscene->tri_vindex.copy_to_device_if_modified();
  }

  if (curve_segment_size != 0) {
    progress.set_status("Updating Mesh", "Copying Curves to device");

    float4 *curve_keys = dscene->curve_keys.alloc(curve_key_size);
    KernelCurve *curves = dscene->curves.alloc(curve_size);
    KernelCurveSegment *curve_segments = dscene->curve_segments.alloc(curve_segment_size);

    const bool copy_all_data = dscene->curve_keys.need_realloc() ||
                               dscene->curves.need_realloc() ||
                               dscene->curve_segments.need_realloc();

    for (Geometry *geom : scene->geometry) {
      if (geom->is_hair()) {
        Hair *hair = static_cast<Hair *>(geom);

        const bool curve_keys_co_modified = hair->curve_radius_is_modified() ||
                                            hair->curve_keys_is_modified();
        const bool curve_data_modified = hair->curve_shader_is_modified() ||
                                         hair->curve_first_key_is_modified();

        if (!curve_keys_co_modified && !curve_data_modified && !copy_all_data) {
          continue;
        }

        hair->pack_curves(scene,
                          &curve_keys[hair->curve_key_offset],
                          &curves[hair->prim_offset],
                          &curve_segments[hair->curve_segment_offset]);
        if (progress.get_cancel()) {
          return;
        }
      }
    }

    dscene->curve_keys.copy_to_device_if_modified();
    dscene->curves.copy_to_device_if_modified();
    dscene->curve_segments.copy_to_device_if_modified();
  }

  if (point_size != 0) {
    progress.set_status("Updating Mesh", "Copying Point clouds to device");

    float4 *points = dscene->points.alloc(point_size);
    uint *points_shader = dscene->points_shader.alloc(point_size);

    for (Geometry *geom : scene->geometry) {
      if (geom->is_pointcloud()) {
        PointCloud *pointcloud = static_cast<PointCloud *>(geom);
        pointcloud->pack(
            scene, &points[pointcloud->prim_offset], &points_shader[pointcloud->prim_offset]);
        if (progress.get_cancel()) {
          return;
        }
      }
    }

    dscene->points.copy_to_device();
    dscene->points_shader.copy_to_device();
  }
}

}
