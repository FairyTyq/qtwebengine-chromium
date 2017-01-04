/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DXGI_TEXTURE_MAPPING_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DXGI_TEXTURE_MAPPING_H_

#include <D3D11.h>
#include <DXGI1_2.h>

#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/modules/desktop_capture/win/dxgi_texture.h"

namespace webrtc {

// A DxgiTexture which directly maps bitmap from IDXGIResource. This class is
// used when DXGI_OUTDUPL_DESC.DesktopImageInSystemMemory is true. (This usually
// means the video card shares main memory with CPU, instead of having its own
// individual memory.)
class DxgiTextureMapping : public DxgiTexture {
 public:
  // Creates a DxgiTextureMapping instance. Caller must maintain the lifetime
  // of input duplication to make sure it outlives this instance.
  DxgiTextureMapping(const DesktopRect& desktop_rect,
                     IDXGIOutputDuplication* duplication);

  ~DxgiTextureMapping() override;

  bool CopyFrom(const DXGI_OUTDUPL_FRAME_INFO& frame_info,
                IDXGIResource* resource,
                const DesktopRegion& region) override;

  bool DoRelease() override;

 private:
  IDXGIOutputDuplication* const duplication_;
};

}  // namespace webrtc

#endif  // WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DXGI_TEXTURE_MAPPING_H_
