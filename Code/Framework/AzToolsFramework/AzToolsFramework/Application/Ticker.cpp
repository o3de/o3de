/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
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