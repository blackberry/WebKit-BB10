/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebLoaderClient.h"

#include "ImmutableArray.h"
#include "ImmutableDictionary.h"
#include "WKAPICast.h"
#include "WebBackForwardListItem.h"
#include "WebPageProxy.h"
#include <string.h>

using namespace WebCore;

namespace WebKit {

void WebLoaderClient::didStartProvisionalLoadForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didStartProvisionalLoadForFrame)
        return;

    m_client.didStartProvisionalLoadForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didReceiveServerRedirectForProvisionalLoadForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didReceiveServerRedirectForProvisionalLoadForFrame)
        return;

    m_client.didReceiveServerRedirectForProvisionalLoadForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didFailProvisionalLoadWithErrorForFrame(WebPageProxy* page, WebFrameProxy* frame, const ResourceError& error, APIObject* userData)
{
    if (!m_client.didFailProvisionalLoadWithErrorForFrame)
        return;

    m_client.didFailProvisionalLoadWithErrorForFrame(toAPI(page), toAPI(frame), toAPI(error), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didCommitLoadForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didCommitLoadForFrame)
        return;

    m_client.didCommitLoadForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didFinishDocumentLoadForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didFinishDocumentLoadForFrame)
        return;

    m_client.didFinishDocumentLoadForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didFinishLoadForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didFinishLoadForFrame)
        return;

    m_client.didFinishLoadForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didFailLoadWithErrorForFrame(WebPageProxy* page, WebFrameProxy* frame, const ResourceError& error, APIObject* userData)
{
    if (!m_client.didFailLoadWithErrorForFrame)
        return;

    m_client.didFailLoadWithErrorForFrame(toAPI(page), toAPI(frame), toAPI(error), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didSameDocumentNavigationForFrame(WebPageProxy* page, WebFrameProxy* frame, SameDocumentNavigationType type, APIObject* userData)
{
    if (!m_client.didSameDocumentNavigationForFrame)
        return;

    m_client.didSameDocumentNavigationForFrame(toAPI(page), toAPI(frame), toAPI(type), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didReceiveTitleForFrame(WebPageProxy* page, const String& title, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didReceiveTitleForFrame)
        return;

    m_client.didReceiveTitleForFrame(toAPI(page), toAPI(title.impl()), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didFirstLayoutForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didFirstLayoutForFrame)
        return;

    m_client.didFirstLayoutForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didFirstVisuallyNonEmptyLayoutForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didFirstVisuallyNonEmptyLayoutForFrame)
        return;

    m_client.didFirstVisuallyNonEmptyLayoutForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didNewFirstVisuallyNonEmptyLayout(WebPageProxy* page, APIObject* userData)
{
    if (!m_client.didNewFirstVisuallyNonEmptyLayout)
        return;

    m_client.didNewFirstVisuallyNonEmptyLayout(toAPI(page), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didLayout(WebPageProxy* page, LayoutMilestones milestones, APIObject* userData)
{
    if (!m_client.didLayout)
        return;

    m_client.didLayout(toAPI(page), toWKLayoutMilestones(milestones), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didRemoveFrameFromHierarchy(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didRemoveFrameFromHierarchy)
        return;

    m_client.didRemoveFrameFromHierarchy(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didDisplayInsecureContentForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didDisplayInsecureContentForFrame)
        return;

    m_client.didDisplayInsecureContentForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didRunInsecureContentForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didRunInsecureContentForFrame)
        return;

    m_client.didRunInsecureContentForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didDetectXSSForFrame(WebPageProxy* page, WebFrameProxy* frame, APIObject* userData)
{
    if (!m_client.didDetectXSSForFrame)
        return;

    m_client.didDetectXSSForFrame(toAPI(page), toAPI(frame), toAPI(userData), m_client.clientInfo);
}

bool WebLoaderClient::canAuthenticateAgainstProtectionSpaceInFrame(WebPageProxy* page, WebFrameProxy* frame, WebProtectionSpace* protectionSpace)
{
    if (!m_client.canAuthenticateAgainstProtectionSpaceInFrame)
        return false;

    return m_client.canAuthenticateAgainstProtectionSpaceInFrame(toAPI(page), toAPI(frame), toAPI(protectionSpace), m_client.clientInfo);
}

void WebLoaderClient::didReceiveAuthenticationChallengeInFrame(WebPageProxy* page, WebFrameProxy* frame, AuthenticationChallengeProxy* authenticationChallenge)
{
    if (!m_client.didReceiveAuthenticationChallengeInFrame)
        return;

    m_client.didReceiveAuthenticationChallengeInFrame(toAPI(page), toAPI(frame), toAPI(authenticationChallenge), m_client.clientInfo);
}

void WebLoaderClient::didStartProgress(WebPageProxy* page)
{
    if (!m_client.didStartProgress)
        return;

    m_client.didStartProgress(toAPI(page), m_client.clientInfo);
}

void WebLoaderClient::didChangeProgress(WebPageProxy* page)
{
    if (!m_client.didChangeProgress)
        return;

    m_client.didChangeProgress(toAPI(page), m_client.clientInfo);
}

void WebLoaderClient::didFinishProgress(WebPageProxy* page)
{
    if (!m_client.didFinishProgress)
        return;

    m_client.didFinishProgress(toAPI(page), m_client.clientInfo);
}

void WebLoaderClient::processDidBecomeUnresponsive(WebPageProxy* page)
{
    if (!m_client.processDidBecomeUnresponsive)
        return;

    m_client.processDidBecomeUnresponsive(toAPI(page), m_client.clientInfo);
}

void WebLoaderClient::interactionOccurredWhileProcessUnresponsive(WebPageProxy* page)
{
    if (!m_client.interactionOccurredWhileProcessUnresponsive)
        return;

    m_client.interactionOccurredWhileProcessUnresponsive(toAPI(page), m_client.clientInfo);
}

void WebLoaderClient::processDidBecomeResponsive(WebPageProxy* page)
{
    if (!m_client.processDidBecomeResponsive)
        return;

    m_client.processDidBecomeResponsive(toAPI(page), m_client.clientInfo);
}

void WebLoaderClient::processDidCrash(WebPageProxy* page)
{
    if (!m_client.processDidCrash)
        return;

    m_client.processDidCrash(toAPI(page), m_client.clientInfo);
}

void WebLoaderClient::didChangeBackForwardList(WebPageProxy* page, WebBackForwardListItem* addedItem, Vector<RefPtr<APIObject> >* removedItems)
{
    if (!m_client.didChangeBackForwardList)
        return;

    RefPtr<ImmutableArray> removedItemsArray;
    if (removedItems && !removedItems->isEmpty())
        removedItemsArray = ImmutableArray::adopt(*removedItems);

    m_client.didChangeBackForwardList(toAPI(page), toAPI(addedItem), toAPI(removedItemsArray.get()), m_client.clientInfo);
}

bool WebLoaderClient::shouldGoToBackForwardListItem(WebPageProxy* page, WebBackForwardListItem* item)
{
    // We should only even considering sending the shouldGoToBackForwardListItem() client callback
    // for version 0 clients. Later versioned clients should get willGoToBackForwardListItem() instead,
    // but due to XPC race conditions this one might have been called instead.
    if (m_client.version > 0 || !m_client.shouldGoToBackForwardListItem)
        return true;

    return m_client.shouldGoToBackForwardListItem(toAPI(page), toAPI(item), m_client.clientInfo);
}

void WebLoaderClient::willGoToBackForwardListItem(WebPageProxy* page, WebBackForwardListItem* item, APIObject* userData)
{
    if (m_client.willGoToBackForwardListItem)
        m_client.willGoToBackForwardListItem(toAPI(page), toAPI(item), toAPI(userData), m_client.clientInfo);
}

void WebLoaderClient::didFailToInitializePlugin(WebPageProxy* page, const String& mimeType, const String& frameURLString, const String& pageURLString)
{
    if (m_client.didFailToInitializePlugin_deprecatedForUseWithV0)
        m_client.didFailToInitializePlugin_deprecatedForUseWithV0(toAPI(page), toAPI(mimeType.impl()), m_client.clientInfo);

    if (m_client.pluginDidFail_deprecatedForUseWithV1)
        m_client.pluginDidFail_deprecatedForUseWithV1(toAPI(page), kWKErrorCodeCannotLoadPlugIn, toAPI(mimeType.impl()), 0, 0, m_client.clientInfo);

    if (m_client.pluginDidFail) {
        RefPtr<ImmutableDictionary> pluginInformation = WebPageProxy::pluginInformationDictionary(String(), String(), String(), frameURLString, mimeType, pageURLString, String(), String());
        m_client.pluginDidFail(toAPI(page), kWKErrorCodeCannotLoadPlugIn, toAPI(pluginInformation.get()), m_client.clientInfo);
    }
}

void WebLoaderClient::didBlockInsecurePluginVersion(WebPageProxy* page, const String& mimeType, const String& pluginBundleIdentifier, const String& pluginBundleVersion, const String& frameURLString, const String& pageURLString)
{
    if (m_client.pluginDidFail_deprecatedForUseWithV1)
        m_client.pluginDidFail_deprecatedForUseWithV1(toAPI(page), kWKErrorCodeInsecurePlugInVersion, toAPI(mimeType.impl()), toAPI(pluginBundleIdentifier.impl()), toAPI(pluginBundleVersion.impl()), m_client.clientInfo);

    if (m_client.pluginDidFail) {
        RefPtr<ImmutableDictionary> pluginInformation = WebPageProxy::pluginInformationDictionary(pluginBundleIdentifier, pluginBundleVersion, String(), frameURLString, mimeType, pageURLString, String(), String());
        m_client.pluginDidFail(toAPI(page), kWKErrorCodeInsecurePlugInVersion, toAPI(pluginInformation.get()), m_client.clientInfo);
    }
}

static inline WKPluginLoadPolicy toWKPluginLoadPolicy(PluginModuleLoadPolicy pluginModuleLoadPolicy)
{
    switch (pluginModuleLoadPolicy) {
    case PluginModuleLoadNormally:
        return kWKPluginLoadPolicyLoadNormally;
    case PluginModuleBlocked:
        return kWKPluginLoadPolicyBlocked;
    case PluginModuleInactive:
        return kWKPluginLoadPolicyInactive;
    }

    ASSERT_NOT_REACHED();
    return kWKPluginLoadPolicyBlocked;
}

static inline PluginModuleLoadPolicy toPluginModuleLoadPolicy(WKPluginLoadPolicy pluginLoadPolicy)
{
    switch (pluginLoadPolicy) {
    case kWKPluginLoadPolicyLoadNormally:
        return PluginModuleLoadNormally;
    case kWKPluginLoadPolicyBlocked:
        return PluginModuleBlocked;
    case kWKPluginLoadPolicyInactive:
        return PluginModuleInactive;
    }

    ASSERT_NOT_REACHED();
    return PluginModuleBlocked;
}

PluginModuleLoadPolicy WebLoaderClient::pluginLoadPolicy(WebPageProxy* page, const String& pluginBundleIdentifier, const String& pluginBundleVersion, const String& displayName, const String& frameURLString, const String& pageURLString, PluginModuleLoadPolicy currentPluginLoadPolicy)
{
    if (!m_client.pluginLoadPolicy)
        return currentPluginLoadPolicy;

    RefPtr<ImmutableDictionary> pluginInformation = WebPageProxy::pluginInformationDictionary(pluginBundleIdentifier, pluginBundleVersion, displayName, frameURLString, String(), pageURLString, String(), String());
    return toPluginModuleLoadPolicy(m_client.pluginLoadPolicy(toAPI(page), toWKPluginLoadPolicy(currentPluginLoadPolicy), toAPI(pluginInformation.get()), m_client.clientInfo));
}

} // namespace WebKit
