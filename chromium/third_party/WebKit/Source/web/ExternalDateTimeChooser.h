/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef ExternalDateTimeChooser_h
#define ExternalDateTimeChooser_h

#if !ENABLE(INPUT_MULTIPLE_FIELDS_UI)
#include "platform/DateTimeChooser.h"

namespace WebCore {
class DateTimeChooserClient;
}

namespace blink {

class ChromeClientImpl;
class WebString;
class WebViewClient;

class ExternalDateTimeChooser FINAL : public WebCore::DateTimeChooser {
public:
    static PassRefPtr<ExternalDateTimeChooser> create(ChromeClientImpl*, WebViewClient*, WebCore::DateTimeChooserClient*, const WebCore::DateTimeChooserParameters&);
    virtual ~ExternalDateTimeChooser();

    // The following functions are for DateTimeChooserCompletion.
    void didChooseValue(const WebString&);
    void didChooseValue(double);
    void didCancelChooser();

private:
    ExternalDateTimeChooser(WebCore::DateTimeChooserClient*);
    bool openDateTimeChooser(ChromeClientImpl*, WebViewClient*, const WebCore::DateTimeChooserParameters&);

    // DateTimeChooser function:
    virtual void endChooser() OVERRIDE;

    WebCore::DateTimeChooserClient* m_client;
};

}
#endif

#endif
