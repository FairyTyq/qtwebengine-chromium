// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/media_metadata.h"

#include <algorithm>
#include <iterator>

namespace content {

MediaMetadata::MediaMetadata() = default;

MediaMetadata::~MediaMetadata() = default;

MediaMetadata::MediaMetadata(const MediaMetadata& other) = default;

bool MediaMetadata::operator==(const MediaMetadata& other) const {
  return title == other.title && artist == other.artist &&
      album == other.album && artwork == other.artwork;
}

bool MediaMetadata::operator!=(const MediaMetadata& other) const {
  return !this->operator==(other);
}

}  // namespace content
