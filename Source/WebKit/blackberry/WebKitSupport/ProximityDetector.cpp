/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 */

#include "config.h"
#include "ProximityDetector.h"

#include "BlackBerryPlatformPrimitives.h"
#include "Document.h"
#include "Element.h"
#include "ExceptionCode.h"
#include "HTMLNames.h"
#include "HitTestResult.h"
#include "RenderLayer.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "WebPage_p.h"
#include <list>

#define SHOWDEBUG_PROXIMITYDETECTOR 0
using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

#if SHOWDEBUG_PROXIMITYDETECTOR
#define ProximityLog(format, ...) Platform::logAlways(Platform::LogLevelCritical, format, ## __VA_ARGS__)
#else
#define ProximityLog(format, ...)
#endif // SHOWDEBUG_PROXIMITYDETECTOR

static int getPriorityLevel(Node* node)
{
    // Priority level is ascending with zero being the lowest.
    if (node->isTextNode())
        return 1;

    if (!node->isElementNode())
        return 0;

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

class SmartBlock {
public:
    SmartBlock(IntRect& rect, int score)
        : m_rect(rect)
        , m_score(score)
        , m_containedNodes(0)
    { }

    Platform::IntRect m_rect;
    int m_score;
    int m_containedNodes;
};

ProximityDetector::ProximityDetector(WebPagePrivate* webPage)
    : m_webPage(webPage)
{
}

ProximityDetector::~ProximityDetector()
{
}

IntPoint ProximityDetector::findBestPoint(const IntPoint& documentPos, const IntRect& documentPaddingRect, ProximityDetectionStrategy strategy)
{
    ASSERT(m_webPage);

    if (!m_webPage->focusedOrMainFrame())
        return documentPos;

    Document* document = m_webPage->focusedOrMainFrame()->document();

    if (!document || !document->view())
        return documentPos;

    unsigned left = -documentPaddingRect.x();
    unsigned top = -documentPaddingRect.y();
    unsigned right = documentPaddingRect.maxX();
    unsigned bottom = documentPaddingRect.maxY();

    // Adjust hit point to frame
    IntPoint frameContentPos(document->view()->windowToContents(m_webPage->focusedOrMainFrame()->view()->contentsToWindow(documentPos)));
    HitTestRequest request(HitTestRequest::ReadOnly);
    HitTestResult result(frameContentPos, top, right, bottom, left);
    document->renderView()->layer()->hitTest(request, result);

    IntPoint bestPoint = documentPos;
    int bestScore = -1;
    int bestArea = 0;

    std::list<SmartBlock> smartBlocks;
    // Iterate over the list of nodes checking both priority and location
    ListHashSet<RefPtr<Node> > intersectedNodes = result.rectBasedTestResult();

    for (ListHashSet<RefPtr<Node> >::const_iterator it = intersectedNodes.begin(); it != intersectedNodes.end(); ++it) {
        Node* curNode = (*it).get();
        RenderObject* renderer = curNode ? curNode->renderer() : 0;
        if (!renderer || !(renderer->isRenderBlock() || renderer->isInlineBlockOrInlineTable()
            || renderer->isReplaced() || (renderer->isInline() && renderer->isText())))
            continue;

        // FIXME: is this accurate?
        IntRect curRect = renderer->absoluteBoundingBoxRect(true /*use transforms*/);
        IntRect hitTestRect = HitTestLocation::rectForPoint(documentPos, top, right, bottom, left);

        // Check that top corner does not exceed padding
        if (!hitTestRect.contains(curRect.location()))
            continue;

        SmartBlock curBlock = SmartBlock(curRect, getPriorityLevel(curNode));
        bool isCurBlockContained = false;

        for (std::list<SmartBlock>::iterator itr = smartBlocks.begin(); itr != smartBlocks.end();) {
            SmartBlock& block = *itr;
            if (curRect.contains(block.m_rect)) {
                // can multiple blocks contain the current one?
                curBlock.m_containedNodes += block.m_containedNodes + 1;
                curBlock.m_score += block.m_score;
                itr = smartBlocks.erase(itr);
                continue;
            }
            if (block.m_rect.contains(curRect)) {
                isCurBlockContained = true;
                block.m_containedNodes++;
                block.m_score += curBlock.m_score;
            }
            ++itr;
        }

        if (!isCurBlockContained)
            smartBlocks.push_back(curBlock);
    }
    ProximityLog("Projected Scroll Position %d %d blocks %d SmartBlocks %d", documentPos.x(), documentPos.y(), intersectedNodes.size(), smartBlocks.size());

    // Determine the best smartblock based on the strategy
    for (std::list<SmartBlock>::iterator itr = smartBlocks.begin(); itr != smartBlocks.end(); ++itr) {
        SmartBlock block = *itr;
        ProximityLog("rect %s contained %d score %d", block.m_rect.toString().c_str(), block.m_containedNodes, block.m_score);
        switch (strategy) {
        case Score:
            if (block.m_score > bestScore) {
                bestPoint = block.m_rect.location(); // use top left
                bestScore = block.m_score;
            }
            break;
        case Closest:
            if (documentPos.distanceSquaredToPoint(bestPoint) > documentPos.distanceSquaredToPoint(block.m_rect.location()) || itr == smartBlocks.begin())
                bestPoint = block.m_rect.location(); // use top left
            break;
        case Area:
            {
                int area =  block.m_rect.area();
                if (area > bestArea) {
                    bestPoint = block.m_rect.location(); // use top left
                    bestArea = area;
                }
            }
            break;
        default:
            break;
        }
    }

    ProximityLog("BestPoint %d %d", bestPoint.x(), bestPoint.y());
    return bestPoint;
}


} // namespace Webkit
} // namespace BlackBery
