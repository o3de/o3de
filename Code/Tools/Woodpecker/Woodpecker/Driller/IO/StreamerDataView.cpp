/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
