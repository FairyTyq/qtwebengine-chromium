// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_UI_RESOURCE_MANAGER_H_
#define CC_RESOURCES_UI_RESOURCE_MANAGER_H_

#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "cc/base/cc_export.h"
#include "cc/resources/scoped_ui_resource.h"
#include "cc/resources/ui_resource_request.h"

namespace cc {
class UIResourceRequest;

class CC_EXPORT UIResourceManager {
 public:
  UIResourceManager();
  virtual ~UIResourceManager();

  // CreateUIResource creates a resource given a bitmap.  The bitmap is
  // generated via an interface function, which is called when initializing the
  // resource and when the resource has been lost (due to lost context).  The
  // parameter of the interface is a single boolean, which indicates whether the
  // resource has been lost or not.  CreateUIResource returns an Id of the
  // resource, which is always positive.
  virtual UIResourceId CreateUIResource(UIResourceClient* client);

  // Deletes a UI resource.  May safely be called more than once.
  virtual void DeleteUIResource(UIResourceId id);

  virtual gfx::Size GetUIResourceSize(UIResourceId id) const;

  // Methods meant to be used only internally in cc ------------

  // The current UIResourceRequestQueue is moved to the caller.
  std::vector<UIResourceRequest> TakeUIResourcesRequests();

  // Put the recreation of all UI resources into the resource queue after they
  // were evicted on the impl thread.
  void RecreateUIResources();

 private:
  struct UIResourceClientData {
    UIResourceClient* client;
    gfx::Size size;
  };

  using UIResourceClientMap =
      std::unordered_map<UIResourceId, UIResourceClientData>;
  UIResourceClientMap ui_resource_client_map_;
  int next_ui_resource_id_;

  using UIResourceRequestQueue = std::vector<UIResourceRequest>;
  UIResourceRequestQueue ui_resource_request_queue_;

  DISALLOW_COPY_AND_ASSIGN(UIResourceManager);
};

}  // namespace cc

#endif  // CC_RESOURCES_UI_RESOURCE_MANAGER_H_
