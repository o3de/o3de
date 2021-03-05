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
#if !defined(Q_MOC_RUN)
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include <AzCore/Memory/SystemAllocator.h>
#endif

#pragma once

namespace AzToolsFramework
{
    //! Qt suppresses all timer events during modal dialogs, this object allows us to 
    //! emit a Tick event from which we can broadcast a SystemTick event.
    class Ticker
        : public QObject
    {
    public:
        Q_OBJECT

    public:

        explicit Ticker(QObject* parent = nullptr, float timeoutMS = 10.f);
        virtual ~Ticker();

        //! Starts the ticking on a thread
        void Start();

        //! Cancels and destroys the ticking thread
        void Cancel();

    Q_SIGNALS:
        //! Connect to this signal to handle the tick
        void Tick();

    private Q_SLOTS:
        //! Single shot event that emits the Tick signal
        void Loop();

    private:

        QThread* m_thread;
        bool m_cancelled;
        float m_timeoutMS;
    };

}