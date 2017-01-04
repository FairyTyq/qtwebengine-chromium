// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/gpu/display_compositor/compositor_frame_sink_factory_impl.h"

#include "base/memory/ptr_util.h"
#include "cc/surfaces/surface_id.h"
#include "services/ui/gpu/display_compositor/compositor_frame_sink_impl.h"

namespace ui {
namespace gpu {

CompositorFrameSinkFactoryImpl::CompositorFrameSinkFactoryImpl(
    uint32_t client_id,
    const scoped_refptr<DisplayCompositor>& display_compositor)
    : client_id_(client_id),
      display_compositor_(display_compositor),
      allocator_(client_id) {}

CompositorFrameSinkFactoryImpl::~CompositorFrameSinkFactoryImpl() {}

void CompositorFrameSinkFactoryImpl::CompositorFrameSinkConnectionLost(
    int local_id) {
  sinks_.erase(local_id);
}

cc::SurfaceId CompositorFrameSinkFactoryImpl::GenerateSurfaceId() {
  return allocator_.GenerateId();
}

void CompositorFrameSinkFactoryImpl::CreateCompositorFrameSink(
    uint32_t local_id,
    uint64_t nonce,
    mojo::InterfaceRequest<mojom::CompositorFrameSink> sink,
    mojom::CompositorFrameSinkClientPtr client) {
  // TODO(fsamuel): Use nonce once patch lands:
  // https://codereview.chromium.org/1996783002/
  sinks_[local_id] = base::MakeUnique<CompositorFrameSinkImpl>(
      this, local_id, display_compositor_, std::move(sink), std::move(client));
}

}  // namespace gpu
}  // namespace ui
