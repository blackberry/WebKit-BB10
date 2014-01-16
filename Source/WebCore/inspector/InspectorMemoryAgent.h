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

#ifndef InspectorMemoryAgent_h
#define InspectorMemoryAgent_h

#if ENABLE(INSPECTOR)

#include "InspectorBaseAgent.h"
#include "InspectorFrontend.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class InspectorClient;
class InspectorDOMStorageAgent;
class InspectorState;
class InstrumentingAgents;
class Page;

typedef String ErrorString;

class InspectorMemoryAgent : public InspectorBaseAgent<InspectorMemoryAgent>, public InspectorBackendDispatcher::MemoryCommandHandler {
    WTF_MAKE_NONCOPYABLE(InspectorMemoryAgent);
public:
    typedef Vector<OwnPtr<InspectorBaseAgentInterface> > InspectorAgents;

    static PassOwnPtr<InspectorMemoryAgent> create(InstrumentingAgents* instrumentingAgents, InspectorClient* client, InspectorCompositeState* state, Page* page)
    {
        return adoptPtr(new InspectorMemoryAgent(instrumentingAgents, client, state, page));
    }
    virtual ~InspectorMemoryAgent();

    virtual void getDOMCounters(ErrorString*, int* documents, int* nodes, int* jsEventListeners);
    virtual void getProcessMemoryDistribution(ErrorString*, const bool* reportGraph, RefPtr<TypeBuilder::Memory::MemoryBlock>& out_processMemory, RefPtr<InspectorObject>& graphMetaInformation);

    virtual void reportMemoryUsage(MemoryObjectInfo*) const;

    void getProcessMemoryDistributionMap(HashMap<String, size_t>* memoryInfo);

    virtual void setFrontend(InspectorFrontend*);
    virtual void clearFrontend();

private:
    InspectorMemoryAgent(InstrumentingAgents*, InspectorClient*, InspectorCompositeState*, Page*);

    PassRefPtr<InspectorObject> getProcessMemoryDistributionImpl(bool reportGraph, HashMap<String, size_t>* memoryInfo);

    InspectorClient* m_inspectorClient;
    Page* m_page;
    InspectorFrontend::Memory* m_frontend;
};

} // namespace WebCore

#endif // !ENABLE(INSPECTOR)
#endif // !defined(InspectorMemoryAgent_h)
