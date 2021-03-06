/*
 * Copyright (C) 2012 Samsung Electronics. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetworkInfoConnection.h"

#if ENABLE(NETWORK_INFO)
#include "Event.h"
#include "Frame.h"
#include "NetworkInfoClient.h"

namespace WebCore {

PassRefPtr<NetworkInfoConnection> NetworkInfoConnection::create(Navigator* navigator)
{
    RefPtr<NetworkInfoConnection> networkInfoConnection(adoptRef(new NetworkInfoConnection(navigator)));
    networkInfoConnection->suspendIfNeeded();
    return networkInfoConnection.release();
}

NetworkInfoConnection::NetworkInfoConnection(Navigator* navigator)
    : ActiveDOMObject(navigator->frame()->document(), this)
    , FrameDestructionObserver(navigator->frame())
    , m_networkInfo(0)
{
    if (NetworkInfoController* controller = networkInfoController())
        controller->addListener(this);
}

NetworkInfoConnection::~NetworkInfoConnection()
{
}

double NetworkInfoConnection::bandwidth() const
{
    if (NetworkInfoController* controller = networkInfoController())
        return controller->client()->bandwidth();

    ASSERT_NOT_REACHED();
    return 0;
}

bool NetworkInfoConnection::metered() const
{
    if (NetworkInfoController* controller = networkInfoController())
        return controller->client()->metered();

    ASSERT_NOT_REACHED();
    return false;
}

void NetworkInfoConnection::didChangeNetworkInformation(PassRefPtr<Event> event, PassRefPtr<NetworkInfo> networkInfo)
{
    m_networkInfo = networkInfo;
    dispatchEvent(event);
}

EventTargetData* NetworkInfoConnection::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* NetworkInfoConnection::ensureEventTargetData()
{
    return &m_eventTargetData;
}

const AtomicString& NetworkInfoConnection::interfaceName() const
{
    return eventNames().interfaceForNetworkInfoConnection;
}

void NetworkInfoConnection::suspend(ReasonForSuspension)
{
    if (NetworkInfoController* controller = networkInfoController())
        controller->removeListener(this);
}

void NetworkInfoConnection::resume()
{
    if (NetworkInfoController* controller = networkInfoController())
        controller->addListener(this);
}

void NetworkInfoConnection::stop()
{
    if (NetworkInfoController* controller = networkInfoController())
        controller->removeListener(this);
}

NetworkInfoController* NetworkInfoConnection::networkInfoController() const
{
    NetworkInfoController* controller = 0;
    if (m_frame && m_frame->page())
        controller = NetworkInfoController::from(m_frame->page());
    return controller;
}

} // namespace WebCore
#endif
