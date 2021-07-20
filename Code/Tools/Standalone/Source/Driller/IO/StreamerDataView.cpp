/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Woodpecker_precompiled.h"

#include "StreamerDataView.hxx"
#include <Woodpecker/Driller/IO/moc_StreamerDataView.cpp>

#include <QTimer>
#include <QScrollBar>

namespace Driller
{
    StreamerDrillerTableView::StreamerDrillerTableView(QWidget* pParent)
        : QTableView(pParent)
        , m_isScrollAfterInsert(true)
    {
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_scheduledMaxScroll = false;
    }

    StreamerDrillerTableView::~StreamerDrillerTableView()
    {
    }

    void StreamerDrillerTableView::rowsAboutToBeInserted()
    {
        m_isScrollAfterInsert = IsAtMaxScroll();
    }

    void StreamerDrillerTableView::rowsInserted()
    {
        if ((m_isScrollAfterInsert) && (!m_scheduledMaxScroll))
        {
            m_scheduledMaxScroll = true;
            QTimer::singleShot(0, this, SLOT(doScrollToBottom()));
        }
    }

    void StreamerDrillerTableView::doScrollToBottom()
    {
        m_scheduledMaxScroll = false;
        scrollToBottom();
        m_isScrollAfterInsert = true;
    }

    bool StreamerDrillerTableView::IsAtMaxScroll() const
    {
        return (verticalScrollBar()->value() == verticalScrollBar()->maximum());
    }
}
