// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SSL_SSL_ERROR_HANDLER_H_
#define CONTENT_BROWSER_SSL_SSL_ERROR_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/global_request_id.h"
#include "content/public/common/resource_type.h"
#include "net/ssl/ssl_info.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {

class ResourceDispatcherHostImpl;
class SSLManager;
class WebContents;

// SSLErrorHandler is the UI-thread class for handling SSL certificate
// errors. Users of this class can call CancelRequest(),
// ContinueRequest(), or DenyRequest() when a decision about how to
// handle the error has been made. Users of this class must
// call exactly one of those methods exactly once.
class SSLErrorHandler {
 public:
  // SSLErrorHandler's delegate lives on the IO thread, and thus these
  // delegate methods must be called on the IO thread only.
  class CONTENT_EXPORT Delegate {
   public:
    // Called when SSLErrorHandler decides to cancel the request because of
    // the SSL error.
    virtual void CancelSSLRequest(int error, const net::SSLInfo* ssl_info) = 0;

    // Called when SSLErrorHandler decides to continue the request despite the
    // SSL error.
    virtual void ContinueSSLRequest() = 0;

   protected:
    virtual ~Delegate() {}
  };

  SSLErrorHandler(WebContents* web_contents,
                  const base::WeakPtr<Delegate>& delegate,
                  ResourceType resource_type,
                  const GURL& url,
                  const net::SSLInfo& ssl_info,
                  bool fatal);

  virtual ~SSLErrorHandler();

  const net::SSLInfo& ssl_info() const { return ssl_info_; }

  const GURL& request_url() const { return request_url_; }

  ResourceType resource_type() const { return resource_type_; }

  WebContents* web_contents() const { return web_contents_; }

  int cert_error() const { return cert_error_; }

  bool fatal() const { return fatal_; }

  // Cancels the associated net::URLRequest.
  CONTENT_EXPORT void CancelRequest();

  // Continue the net::URLRequest ignoring any previous errors.  Note that some
  // errors cannot be ignored, in which case this will result in the request
  // being canceled.
  void ContinueRequest();

  // Cancels the associated net::URLRequest and mark it as denied.  The renderer
  // processes such request in a special manner, optionally replacing them
  // with alternate content (typically frames content is replaced with a
  // warning message).
  void DenyRequest();

 private:
  // This must not be dereferenced on the UI thread. SSLErrorHandler
  // simply holds on to the reference to be passed back to the IO thread
  // to enact a decision about the error once one has been made.
  base::WeakPtr<Delegate> delegate_;

  // The URL for the request that generated the error.
  const GURL request_url_;

  // What kind of resource is associated with the request that generated
  // the error.
  const ResourceType resource_type_;

  // The net::SSLInfo associated with the request that generated the error.
  const net::SSLInfo ssl_info_;

  // A net error code describing the error that occurred.
  const int cert_error_;

  // True if the error is from a host requiring certificate errors to be fatal.
  const bool fatal_;

  // The WebContents associated with the request that generated the error.
  WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(SSLErrorHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SSL_SSL_ERROR_HANDLER_H_
