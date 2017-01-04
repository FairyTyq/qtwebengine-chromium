/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/network/WebSocketHandshakeRequest.h"

namespace blink {

WebSocketHandshakeRequest::WebSocketHandshakeRequest(const KURL& url)
    : m_url(url) {}

WebSocketHandshakeRequest::WebSocketHandshakeRequest() {}

WebSocketHandshakeRequest::WebSocketHandshakeRequest(
    const WebSocketHandshakeRequest& request)
    : m_url(request.m_url),
      m_headerFields(request.m_headerFields),
      m_headersText(request.m_headersText) {}

WebSocketHandshakeRequest::~WebSocketHandshakeRequest() {}

void WebSocketHandshakeRequest::addAndMergeHeader(HTTPHeaderMap* map,
                                                  const AtomicString& name,
                                                  const AtomicString& value) {
  HTTPHeaderMap::AddResult result = map->add(name, value);
  if (!result.isNewEntry) {
    // Inspector expects the "\n" separated format.
    result.storedValue->value =
        result.storedValue->value + "\n" + String(value);
  }
}

}  // namespace blink
