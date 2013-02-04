/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 */


#include "config.h"
#include "ProximityDetector.h"

#include "Document.h"
#include "Element.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "WebPage_p.h"

using WTF::RefPtr;
using WTF::String;

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

static int getPriorityLevel(Node* node)
{
    // Priority level is ascending with zero being the lowest.
    if (node->isTextNode())
        return 1;

    if (!node->isElementNode())
        return 0;

    // FIXME: handle Frames, subframes, divs etc
    Element* element = toElement(node);
    ASSERT(element);
    ExceptionCode ec = 0;

    if (element->webkitMatchesSelector("img,obj", ec))
        return 1;

    if (element->hasTagName(HTMLNames::pTag))
        return 2;

    if (element->webkitMatchesSelector("h1,h2,h3,h4,h5,h6", ec))
        return 3;

    return 0;
}

ProximityDetector::ProximityDetector(WebPagePrivate* webPage)
    : m_webPage(webPage)
{
}

ProximityDetector::~ProximityDetector()
{
}

// NOTE: both parameter and the return values of this method are in *contents coordinates*.
IntPoint ProximityDetector::findBestPoint(const IntPoint& contentPos, const IntRect& paddingRect)
{
    ASSERT(m_webPage);

    if (!m_webPage->m_mainFrame)
        return contentPos;

    Document* document = m_webPage->m_mainFrame->document();

    if (!document || !document->frame()->view())
        return contentPos;

    unsigned left = -paddingRect.x();
    unsigned top = -paddingRect.y();
    unsigned right = paddingRect.maxX();
    unsigned bottom = paddingRect.maxY();

    // Adjust hit point to frame
    IntPoint frameContentPos(document->frame()->view()->windowToContents(m_webPage->m_mainFrame->view()->contentsToWindow(contentPos)));
    HitTestRequest request(HitTestRequest::ReadOnly | HitTestRequest::Active | HitTestRequest::IgnoreClipping);
    HitTestResult result(frameContentPos, top, right, bottom, left);
    document->renderView()->layer()->hitTest(request, result);

    IntPoint bestPoint = contentPos;
    int bestPriority = 0;

    // Iterate over the list of nodes checking both priority and location
    ListHashSet<RefPtr<Node> > intersectedNodes = result.rectBasedTestResult();
    ListHashSet<RefPtr<Node> >::const_iterator it = intersectedNodes.begin();
    ListHashSet<RefPtr<Node> >::const_iterator end = intersectedNodes.end();
    for ( ; it != end; ++it) {

        Node* curNode = (*it).get();
        if (!curNode || !curNode->renderer())
            continue;

        IntRect curRect = curNode->renderer()->absoluteBoundingBoxRect(true /*use transforms*/);
        IntRect hitTestRect = HitTestResult::rectForPoint(contentPos, top, right, bottom, left);

        // Check that top corner does not exceed padding
        if (!hitTestRect.contains(curRect.location()))
            continue;

       int priority = getPriorityLevel(curNode);
       if (!priority)
           continue;

       // FIXME: consider checking size of rects and pos on page to compare.
       if (priority > bestPriority
               || (priority == bestPriority && sqrt((double)contentPos.distanceSquaredToPoint(bestPoint)) > sqrt((double)contentPos.distanceSquaredToPoint(curRect.location())))) {
           bestPoint = curRect.location(); // use top left for now
           bestPriority = priority;
       }
    }

    return bestPoint;
}

}
}
