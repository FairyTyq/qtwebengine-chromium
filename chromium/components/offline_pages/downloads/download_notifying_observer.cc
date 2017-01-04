// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/downloads/download_notifying_observer.h"

#include "components/offline_pages/background/request_coordinator.h"
#include "components/offline_pages/background/save_page_request.h"
#include "components/offline_pages/client_policy_controller.h"
#include "components/offline_pages/downloads/download_ui_adapter.h"
#include "components/offline_pages/downloads/offline_page_download_notifier.h"

namespace offline_pages {
namespace {
int kUserDataKey;  // Only address is used.
}  // namespace

DownloadNotifyingObserver::DownloadNotifyingObserver(
    std::unique_ptr<OfflinePageDownloadNotifier> notifier,
    ClientPolicyController* policy_controller)
    : notifier_(std::move(notifier)), policy_controller_(policy_controller) {}

DownloadNotifyingObserver::~DownloadNotifyingObserver() {}

// static
DownloadNotifyingObserver* DownloadNotifyingObserver::GetFromRequestCoordinator(
    RequestCoordinator* request_coordinator) {
  DCHECK(request_coordinator);
  return static_cast<DownloadNotifyingObserver*>(
      request_coordinator->GetUserData(&kUserDataKey));
}

// static
void DownloadNotifyingObserver::CreateAndStartObserving(
    RequestCoordinator* request_coordinator,
    std::unique_ptr<OfflinePageDownloadNotifier> notifier) {
  DCHECK(request_coordinator);
  DCHECK(notifier.get());
  DownloadNotifyingObserver* observer = new DownloadNotifyingObserver(
      std::move(notifier), request_coordinator->GetPolicyController());
  request_coordinator->AddObserver(observer);
  // |request_coordinator| takes ownership of observer here.
  request_coordinator->SetUserData(&kUserDataKey, observer);
}

void DownloadNotifyingObserver::OnAdded(const SavePageRequest& request) {
  DCHECK(notifier_.get());
  if (!IsVisibleInUI(request.client_id()))
    return;
  notifier_->NotifyDownloadProgress(DownloadUIItem(request));
}

void DownloadNotifyingObserver::OnChanged(const SavePageRequest& request) {
  DCHECK(notifier_.get());
  if (!IsVisibleInUI(request.client_id()))
    return;
  if (request.request_state() == SavePageRequest::RequestState::PAUSED)
    notifier_->NotifyDownloadPaused(DownloadUIItem(request));
  else
    notifier_->NotifyDownloadProgress(DownloadUIItem(request));
}

void DownloadNotifyingObserver::OnCompleted(
    const SavePageRequest& request,
    RequestCoordinator::BackgroundSavePageResult status) {
  DCHECK(notifier_.get());
  if (!IsVisibleInUI(request.client_id()))
    return;
  if (status == RequestCoordinator::BackgroundSavePageResult::SUCCESS)
    notifier_->NotifyDownloadSuccessful(DownloadUIItem(request));
  else if (status == RequestCoordinator::BackgroundSavePageResult::REMOVED)
    notifier_->NotifyDownloadCanceled(DownloadUIItem(request));
  else
    notifier_->NotifyDownloadFailed(DownloadUIItem(request));
}

bool DownloadNotifyingObserver::IsVisibleInUI(const ClientId& page) {
  return policy_controller_->IsSupportedByDownload(page.name_space) &&
         base::IsValidGUID(page.id);
}

}  // namespace offline_pages
