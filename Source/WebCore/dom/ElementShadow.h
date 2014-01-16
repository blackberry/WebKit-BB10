/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
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

#ifndef ElementShadow_h
#define ElementShadow_h

#include "ContentDistributor.h"
#include "ExceptionCode.h"
#include "ShadowRoot.h"
#include <wtf/DoublyLinkedList.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class ElementShadow {
   WTF_MAKE_NONCOPYABLE(ElementShadow); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<ElementShadow> create()
    {
        return adoptPtr(new ElementShadow());
    }

    ~ElementShadow()
    {
        removeAllShadowRoots();
    }

    Element* host() const;
    ShadowRoot* youngestShadowRoot() const { return m_shadowRoots.head(); }
    ShadowRoot* oldestShadowRoot() const { return m_shadowRoots.tail(); }
    ElementShadow* containingShadow() const;

    ShadowRoot* addShadowRoot(Element* shadowHost, ShadowRoot::ShadowRootType);

    void attach();
    void detach();

    bool childNeedsStyleRecalc() const;
    bool needsStyleRecalc() const;
    void recalcStyle(Node::StyleChange);

    void invalidateDistribution() { m_distributor.invalidateDistribution(host()); }
    void didAffectSelector(AffectedSelectorMask mask) { m_distributor.didAffectSelector(host(), mask); }
    void willAffectSelector() { m_distributor.willAffectSelector(host()); }

    ContentDistributor& distributor() { return m_distributor; }
    const ContentDistributor& distributor() const { return m_distributor; }

    void reportMemoryUsage(MemoryObjectInfo*) const;

private:
    ElementShadow() { }

    void removeAllShadowRoots();

    DoublyLinkedList<ShadowRoot> m_shadowRoots;
    ContentDistributor m_distributor;
};

inline Element* ElementShadow::host() const
{
    ASSERT(!m_shadowRoots.isEmpty());
    return youngestShadowRoot()->host();
}

inline ShadowRoot* Node::youngestShadowRoot() const
{
    if (!this->isElementNode())
        return 0;
    if (ElementShadow* shadow = toElement(this)->shadow())
        return shadow->youngestShadowRoot();
    return 0;
}

inline ElementShadow* ElementShadow::containingShadow() const
{
    if (ShadowRoot* parentRoot = host()->containingShadowRoot())
        return parentRoot->owner();
    return 0;
}

class ShadowRootVector : public Vector<RefPtr<ShadowRoot> > {
public:
    explicit ShadowRootVector(ElementShadow* tree)
    {
        for (ShadowRoot* root = tree->youngestShadowRoot(); root; root = root->olderShadowRoot())
            append(root);
    }
};

inline ElementShadow* shadowOfParent(const Node* node)
{
    if (!node)
        return 0;
    if (Node* parent = node->parentNode())
        if (parent->isElementNode())
            return toElement(parent)->shadow();
    return 0;
}


} // namespace

#endif
