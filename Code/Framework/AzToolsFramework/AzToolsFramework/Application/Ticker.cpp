/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Ticker.h"

namespace AzToolsFramework
{
    Ticker::Ticker(QObject* parent /*= nullptr*/, float timeoutMS)
        : QObject(parent)
        , m_thread(nullptr)
        , m_cancelled(false)
        , m_timeoutMS(timeoutMS)
    {
        m_thread = azcreate(QThread, (parent));
    }

    Ticker::~Ticker()
    {
        Cancel();
    }

    void Ticker::Start()
    {
        moveToThread(m_thread);
        QMetaObject::invokeMethod(this, "Loop", Qt::QueuedConnection);
        m_thread->start();
        m_cancelled = false;
    }

    void Ticker::Cancel()
    {
        if (!m_cancelled)
        {
            m_cancelled = true;
            m_thread->quit();
            m_thread->wait();
            azdestroy(m_thread);
        }
    }

    void Ticker::Loop()
    {
        if (!m_cancelled)
        {
            Q_EMIT Tick();
            QTimer::singleShot(static_cast<int>(m_timeoutMS), Qt::PreciseTimer, this, &Ticker::Loop);
        }
    }
}

#include "Application/moc_Ticker.cpp"
