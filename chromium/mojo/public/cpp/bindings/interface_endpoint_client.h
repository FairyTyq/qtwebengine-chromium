// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_ENDPOINT_CLIENT_H_
#define MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_ENDPOINT_CLIENT_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "mojo/public/cpp/bindings/bindings_export.h"
#include "mojo/public/cpp/bindings/connection_error_callback.h"
#include "mojo/public/cpp/bindings/filter_chain.h"
#include "mojo/public/cpp/bindings/lib/control_message_handler.h"
#include "mojo/public/cpp/bindings/lib/control_message_proxy.h"
#include "mojo/public/cpp/bindings/message.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"

namespace mojo {

class AssociatedGroup;
class AssociatedGroupController;
class InterfaceEndpointController;

// InterfaceEndpointClient handles message sending and receiving of an interface
// endpoint, either the implementation side or the client side.
// It should only be accessed and destructed on the creating thread.
class MOJO_CPP_BINDINGS_EXPORT InterfaceEndpointClient
    : NON_EXPORTED_BASE(public MessageReceiverWithResponder),
      public base::MessageLoop::DestructionObserver {
 public:
  // |receiver| is okay to be null. If it is not null, it must outlive this
  // object.
  InterfaceEndpointClient(ScopedInterfaceEndpointHandle handle,
                          MessageReceiverWithResponderStatus* receiver,
                          std::unique_ptr<MessageReceiver> payload_validator,
                          bool expect_sync_requests,
                          scoped_refptr<base::SingleThreadTaskRunner> runner,
                          uint32_t interface_version);
  ~InterfaceEndpointClient() override;

  // Sets the error handler to receive notifications when an error is
  // encountered.
  void set_connection_error_handler(const base::Closure& error_handler) {
    DCHECK(thread_checker_.CalledOnValidThread());
    error_handler_ = error_handler;
    error_with_reason_handler_.Reset();
  }

  void set_connection_error_with_reason_handler(
      const ConnectionErrorWithReasonCallback& error_handler) {
    DCHECK(thread_checker_.CalledOnValidThread());
    error_with_reason_handler_ = error_handler;
    error_handler_.Reset();
  }

  // Returns true if an error was encountered.
  bool encountered_error() const {
    DCHECK(thread_checker_.CalledOnValidThread());
    return encountered_error_;
  }

  // Returns true if this endpoint has any pending callbacks.
  bool has_pending_responders() const {
    DCHECK(thread_checker_.CalledOnValidThread());
    return !async_responders_.empty() || !sync_responses_.empty();
  }

  AssociatedGroupController* group_controller() const {
    return handle_.group_controller();
  }
  AssociatedGroup* associated_group();
  uint32_t interface_id() const;

  // Adds a MessageReceiver which can filter a message after validation but
  // before dispatch.
  void AddFilter(std::unique_ptr<MessageReceiver> filter);

  // After this call the object is in an invalid state and shouldn't be reused.
  ScopedInterfaceEndpointHandle PassHandle();

  // Raises an error on the underlying message pipe. It disconnects the pipe
  // and notifies all interfaces running on this pipe.
  void RaiseError();

  // MessageReceiverWithResponder implementation:
  bool Accept(Message* message) override;
  bool AcceptWithResponder(Message* message,
                           MessageReceiver* responder) override;

  // The following methods are called by the router. They must be called
  // outside of the router's lock.

  // NOTE: |message| must have passed message header validation.
  bool HandleIncomingMessage(Message* message);
  void NotifyError();

  internal::ControlMessageProxy* control_message_proxy() {
    return &control_message_proxy_;
  }

 private:
  // Maps from the id of a response to the MessageReceiver that handles the
  // response.
  using AsyncResponderMap =
      std::map<uint64_t, std::unique_ptr<MessageReceiver>>;

  struct SyncResponseInfo {
   public:
    explicit SyncResponseInfo(bool* in_response_received);
    ~SyncResponseInfo();

    Message response;

    // Points to a stack-allocated variable.
    bool* response_received;

   private:
    DISALLOW_COPY_AND_ASSIGN(SyncResponseInfo);
  };

  using SyncResponseMap = std::map<uint64_t, std::unique_ptr<SyncResponseInfo>>;

  // Used as the sink for |payload_validator_| and forwards messages to
  // HandleValidatedMessage().
  class HandleIncomingMessageThunk : public MessageReceiver {
   public:
    explicit HandleIncomingMessageThunk(InterfaceEndpointClient* owner);
    ~HandleIncomingMessageThunk() override;

    // MessageReceiver implementation:
    bool Accept(Message* message) override;

   private:
    InterfaceEndpointClient* const owner_;

    DISALLOW_COPY_AND_ASSIGN(HandleIncomingMessageThunk);
  };

  bool HandleValidatedMessage(Message* message);
  void StopObservingIfNecessary();

  // base::MessageLoop::DestructionObserver:
  void WillDestroyCurrentMessageLoop() override;

  ScopedInterfaceEndpointHandle handle_;
  std::unique_ptr<AssociatedGroup> associated_group_;
  InterfaceEndpointController* controller_;

  MessageReceiverWithResponderStatus* const incoming_receiver_;
  HandleIncomingMessageThunk thunk_;
  FilterChain filters_;

  AsyncResponderMap async_responders_;
  SyncResponseMap sync_responses_;

  uint64_t next_request_id_;

  base::Closure error_handler_;
  ConnectionErrorWithReasonCallback error_with_reason_handler_;
  bool encountered_error_;

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  internal::ControlMessageProxy control_message_proxy_;
  internal::ControlMessageHandler control_message_handler_;

  bool observing_message_loop_destruction_ = true;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<InterfaceEndpointClient> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InterfaceEndpointClient);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_CPP_BINDINGS_INTERFACE_ENDPOINT_CLIENT_H_
