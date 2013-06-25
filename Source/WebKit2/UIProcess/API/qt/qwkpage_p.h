/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef qwkpage_p_h
#define qwkpage_p_h

#include "DrawingAreaProxy.h"
#include "PageClient.h"
#include "qwkpage.h"
#include "WebPageNamespace.h"
#include "WebPageProxy.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>
#include <QBasicTimer>
#include <QGraphicsView>
#include <QKeyEvent>

class QWKPreferences;

class QWKPagePrivate : WebKit::PageClient {
public:
    QWKPagePrivate(QWKPage*, WKPageNamespaceRef);
    ~QWKPagePrivate();

    static QWKPagePrivate* get(QWKPage* page) { return page->d; }

    void init(const QSize& viewportSize, WTF::PassOwnPtr<WebKit::DrawingAreaProxy>);

    // PageClient
#if USE(ACCELERATED_COMPOSITING)
    void pageDidEnterAcceleratedCompositing() { }
    void pageDidLeaveAcceleratedCompositing() { }
#endif // USE(ACCELERATED_COMPOSITING)
    virtual void processDidExit() { }
    virtual void processDidRevive() { }
    virtual void setCursor(const WebCore::Cursor&);
    virtual void setViewportArguments(const WebCore::ViewportArguments&);
    virtual void takeFocus(bool direction) { }
    virtual void toolTipChanged(const WTF::String&, const WTF::String&);
    virtual void registerEditCommand(PassRefPtr<WebKit::WebEditCommandProxy>, WebKit::WebPageProxy::UndoOrRedo);
    virtual void clearAllEditCommands();
    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&);
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&);
    virtual void didNotHandleKeyEvent(const WebKit::NativeWebKeyboardEvent&);
    virtual PassRefPtr<WebKit::WebPopupMenuProxy> createPopupMenuProxy();

    virtual void setFindIndicator(PassRefPtr<WebKit::FindIndicator>, bool fadeOut);

    void paint(QPainter* painter, QRect);

    void keyPressEvent(QKeyEvent*);
    void keyReleaseEvent(QKeyEvent*);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*);
    void mousePressEvent(QGraphicsSceneMouseEvent*);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*);
    void wheelEvent(QGraphicsSceneWheelEvent*);

    void updateAction(QWKPage::WebAction action);
    void updateNavigationActions();
    void updateEditorActions();
    void setEditCommandState(const WTF::String&, bool, int);

    void _q_webActionTriggered(bool checked);

    void touchEvent(QTouchEvent*);

    QWKPage* q;

    QAction* actions[QWKPage::WebActionCount];
    QWKPreferences* preferences;

    RefPtr<WebKit::WebPageProxy> page;
    WKPageNamespaceRef pageNamespaceRef;
    WebCore::ViewportArguments viewportArguments;

    QWKPage::CreateNewPageFn createNewPageFn;

    QPoint tripleClick;
    QBasicTimer tripleClickTimer;
};

class QtViewportAttributesPrivate : public QSharedData {
public:
    QtViewportAttributesPrivate(QWKPage::ViewportAttributes* qq)
        : q(qq)
    { }

    QWKPage::ViewportAttributes* q;
};


#endif /* qkpage_p_h */
