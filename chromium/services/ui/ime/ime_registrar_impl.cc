// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ime/ime_registrar_impl.h"

namespace ui {

IMERegistrarImpl::IMERegistrarImpl(IMEServerImpl* ime_server)
    : ime_server_(ime_server) {}

IMERegistrarImpl::~IMERegistrarImpl() {}

void IMERegistrarImpl::AddBinding(mojom::IMERegistrarRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void IMERegistrarImpl::RegisterDriver(mojom::IMEDriverPtr driver) {
  // TODO(moshayedi): crbug.com/634441. IMERegistrarImpl currently identifies
  // the last registered driver as the current driver. Rethink this once we
  // have more usecases.
  ime_server_->OnDriverChanged(std::move(driver));
}
}  // namespace ui
