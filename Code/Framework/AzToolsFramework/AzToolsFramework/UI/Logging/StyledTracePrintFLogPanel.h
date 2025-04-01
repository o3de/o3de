/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "StyledLogPanel.h"
#include "LogLine.h"

#include <AzCore/std/containers/queue.h>
#include <AzCore/Debug/TraceMessageBus.h>
#endif

namespace AzToolsFramework
{
    namespace LogPanel
    {
        class StyledTracePrintFLogPanel
            : public StyledLogPanel
        {
            Q_OBJECT;
        public:
            AZ_CLASS_ALLOCATOR(StyledTracePrintFLogPanel, AZ::SystemAllocator);

            StyledTracePrintFLogPanel(QWidget* pParent = nullptr);

        private:
            QWidget* CreateTab(const TabSettings& settings) override;
        };

        class StyledTracePrintFLogTab
            : public StyledLogTab
            , protected AZ::Debug::TraceMessageBus::Handler
        {
            Q_OBJECT;
        public:
            AZ_CLASS_ALLOCATOR(StyledTracePrintFLogTab, AZ::SystemAllocator);
            StyledTracePrintFLogTab(const TabSettings& in_settings, QWidget* parent = nullptr);
            virtual ~StyledTracePrintFLogTab();

            //////////////////////////////////////////////////////////////////////////
            // TraceMessagesBus
            virtual bool OnAssert(const char* message);
            virtual bool OnException(const char* message);
            virtual bool OnError(const char* window, const char* message);
            virtual bool OnWarning(const char* window, const char* message);
            virtual bool OnPrintf(const char* window, const char* message);
            //////////////////////////////////////////////////////////////////////////

        protected:
            // Log a message received from the TraceMessageBus
            void LogTraceMessage(Logging::LogLine::LogType type, const char* window, const char* message, bool alwaysShowMessage = false);

            // note that we actually buffer the lines up since we could receive them at any time from this bus, even on another thread
            // so we dont push them to the GUI immediately.  Instead we connect to the tickbus and drain the queue on tick.

            AZStd::queue<Logging::LogLine> m_bufferedLines;
            AZStd::atomic_bool m_alreadyQueuedDrainMessage;  // we also only drain the queue at the end so that we do bulk inserts instead of one at a time.
            AZStd::mutex m_bufferedLinesMutex; // protects m_bufferedLines from draining and adding entries into it from different threads at the same time 

        private Q_SLOTS:
            void DrainMessages();
            void ScheduleDrainMessages();

        public slots:
            virtual void Clear();
        };

    } // namespace LogPanel
} // namespace AzToolsFramework
