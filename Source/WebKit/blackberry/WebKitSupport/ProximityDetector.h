/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

    WebCore::IntPoint findBestPoint(const WebCore::IntPoint& contentPos, const WebCore::IntRect& paddingRect);

private:
    WebPagePrivate* m_webPage;
};

}
}

#endif // ProximityDetector_h
