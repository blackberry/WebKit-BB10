/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 */

#ifndef ProximityDetector_h
#define ProximityDetector_h

namespace WebCore {
class IntPoint;
class IntRect;
}

namespace BlackBerry {
namespace WebKit {

class WebPagePrivate;

class ProximityDetector {
public:
    ProximityDetector(WebPagePrivate* webpage);
    ~ProximityDetector();

    enum ProximityDetectionStrategy { Closest = 0, Area, Score };

    WebCore::IntPoint findBestPoint(const WebCore::IntPoint& documentPos, const WebCore::IntRect& documentPaddingRect, ProximityDetectionStrategy = Area);

private:
    WebPagePrivate* m_webPage;
};

}
}

#endif // ProximityDetector_h
