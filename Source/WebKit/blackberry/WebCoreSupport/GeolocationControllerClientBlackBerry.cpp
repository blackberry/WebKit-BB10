/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GeolocationControllerClientBlackBerry.h"

#include "Chrome.h"
#include "Frame.h"
#include "Geolocation.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "Page.h"
#include "SecurityOrigin.h"
#include "WebPage_p.h"

using namespace WebCore;

static CString frameOrigin(Frame* frame)
{
    DOMWindow* window = frame->domWindow();
    SecurityOrigin* origin = window->securityOrigin();
    CString latinOrigin = origin->toString().latin1();
    return latinOrigin;
}

GeolocationControllerClientBlackBerry::GeolocationControllerClientBlackBerry(BlackBerry::WebKit::WebPagePrivate* webPagePrivate)
    : m_webPagePrivate(webPagePrivate)
    , m_tracker(0)
    , m_accuracy(false)
{
}

void GeolocationControllerClientBlackBerry::geolocationDestroyed()
{
    if (m_tracker)
        m_tracker->destroy();
    delete this;
}

void GeolocationControllerClientBlackBerry::startUpdating()
{
    if (m_tracker)
        m_tracker->resume();
    else
        m_tracker = BlackBerry::Platform::GeoTracker::create(this, m_accuracy);
}

void GeolocationControllerClientBlackBerry::stopUpdating()
{
    if (m_tracker)
        m_tracker->suspend();
}

GeolocationPosition* GeolocationControllerClientBlackBerry::lastPosition()
{
    return m_lastPosition.get();
}

void GeolocationControllerClientBlackBerry::requestPermission(Geolocation* location)
{
    Frame* frame = location->frame();
    if (!frame)
        return;
    if (!m_webPagePrivate->m_webSettings->isGeolocationEnabled()) {
        location->setIsAllowed(false);
        return;
    }
    DOMWindow* window = frame->domWindow();
    if (!window)
    return;

    CString latinOrigin = frameOrigin(frame);

    m_webPagePrivate->m_client->requestGeolocationPermission(this, location, latinOrigin.data(), latinOrigin.length());
}

void GeolocationControllerClientBlackBerry::cancelPermissionRequest(Geolocation* location)
{
    Frame* frame = location->frame();
    if (!frame)
        return;
    m_webPagePrivate->m_client->cancelGeolocationPermission(this, location);
}

void GeolocationControllerClientBlackBerry::onLocationUpdate(double timestamp, double latitude, double longitude, double accuracy, double altitude, bool altitudeValid,
                                                             double altitudeAccuracy, bool altitudeAccuracyValid, double speed, bool speedValid, double heading, bool headingValid)
{
    m_lastPosition = GeolocationPosition::create(timestamp, latitude, longitude, accuracy, altitudeValid, altitude, altitudeAccuracyValid,
                                                 altitudeAccuracy, headingValid, heading, speedValid, speed);
    GeolocationController::from(m_webPagePrivate->m_page)->positionChanged(m_lastPosition.get());
}

void GeolocationControllerClientBlackBerry::onLocationError(const char* errorStr)
{
    RefPtr<GeolocationError> error = GeolocationError::create(GeolocationError::PositionUnavailable, String::fromUTF8(errorStr));
    GeolocationController::from(m_webPagePrivate->m_page)->errorOccurred(error.get());
}

void GeolocationControllerClientBlackBerry::onPermission(void* context, bool isAllowed)
{
    Geolocation* position = static_cast<Geolocation*>(context);
    position->setIsAllowed(isAllowed);
}

void GeolocationControllerClientBlackBerry::setEnableHighAccuracy(bool newAccuracy)
{
    if (m_accuracy == newAccuracy)
        return;

    m_accuracy = newAccuracy;

    if (m_tracker)
        m_tracker->setRequiresHighAccuracy(m_accuracy);
}

