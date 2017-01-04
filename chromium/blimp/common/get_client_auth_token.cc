// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "blimp/common/get_client_auth_token.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/string_util.h"
#include "blimp/common/switches.h"

namespace blimp {

std::string GetClientAuthToken(const base::CommandLine& cmd_line) {
  std::string file_contents;
  const base::FilePath path = cmd_line.GetSwitchValuePath(kClientAuthTokenPath);
  if (!base::ReadFileToString(path, &file_contents)) {
    LOG(ERROR) << "Could not read client auth token file at "
               << (path.empty() ? "(not provided)" : path.AsUTF8Unsafe());
  }
  return base::CollapseWhitespaceASCII(file_contents, true);
}

}  // namespace blimp
