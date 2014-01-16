/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"
#include "WebPermissions.h"

#include "TestCommon.h"
#include "WebTestDelegate.h"
#include <public/WebCString.h>
#include <public/WebURL.h>

using namespace std;

namespace WebTestRunner {

WebPermissions::WebPermissions()
    : m_delegate(0)
{
    reset();
}

WebPermissions::~WebPermissions()
{
}

bool WebPermissions::allowImage(WebKit::WebFrame*, bool enabledPerSettings, const WebKit::WebURL& imageURL)
{
    bool allowed = enabledPerSettings && m_imagesAllowed;
    if (m_dumpCallbacks && m_delegate)
        m_delegate->printMessage(std::string("PERMISSION CLIENT: allowImage(") + normalizeLayoutTestURL(imageURL.spec()) + "): " + (allowed ? "true" : "false") + "\n");
    return allowed;
}

bool WebPermissions::allowScriptFromSource(WebKit::WebFrame*, bool enabledPerSettings, const WebKit::WebURL& scriptURL)
{
    bool allowed = enabledPerSettings && m_scriptsAllowed;
    if (m_dumpCallbacks && m_delegate)
        m_delegate->printMessage(std::string("PERMISSION CLIENT: allowScriptFromSource(") + normalizeLayoutTestURL(scriptURL.spec()) + "): " + (allowed ? "true" : "false") + "\n");
    return allowed;
}

bool WebPermissions::allowStorage(WebKit::WebFrame*, bool)
{
    return m_storageAllowed;
}

bool WebPermissions::allowPlugins(WebKit::WebFrame*, bool enabledPerSettings)
{
    return enabledPerSettings && m_pluginsAllowed;
}

bool WebPermissions::allowDisplayingInsecureContent(WebKit::WebFrame*, bool enabledPerSettings, const WebKit::WebSecurityOrigin&, const WebKit::WebURL&)
{
    return enabledPerSettings || m_displayingInsecureContentAllowed;
}

bool WebPermissions::allowRunningInsecureContent(WebKit::WebFrame*, bool enabledPerSettings, const WebKit::WebSecurityOrigin&, const WebKit::WebURL&)
{
    return enabledPerSettings || m_runningInsecureContentAllowed;
}

void WebPermissions::setImagesAllowed(bool imagesAllowed)
{
    m_imagesAllowed = imagesAllowed;
}

void WebPermissions::setScriptsAllowed(bool scriptsAllowed)
{
    m_scriptsAllowed = scriptsAllowed;
}

void WebPermissions::setStorageAllowed(bool storageAllowed)
{
    m_storageAllowed = storageAllowed;
}

void WebPermissions::setPluginsAllowed(bool pluginsAllowed)
{
    m_pluginsAllowed = pluginsAllowed;
}

void WebPermissions::setDisplayingInsecureContentAllowed(bool allowed)
{
    m_displayingInsecureContentAllowed = allowed;
}

void WebPermissions::setRunningInsecureContentAllowed(bool allowed)
{
    m_runningInsecureContentAllowed = allowed;
}

void WebPermissions::setDelegate(WebTestDelegate* delegate)
{
    m_delegate = delegate;
}

void WebPermissions::setDumpCallbacks(bool dumpCallbacks)
{
    m_dumpCallbacks = dumpCallbacks;
}

void WebPermissions::reset()
{
    m_dumpCallbacks = false;
    m_imagesAllowed = true;
    m_scriptsAllowed = true;
    m_storageAllowed = true;
    m_pluginsAllowed = true;
    m_displayingInsecureContentAllowed = false;
    m_runningInsecureContentAllowed = false;
}

}
