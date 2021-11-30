/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include "LogPanel_Panel.h" // for the BaseLogPanel
#include "LogControl.h" // for BaseLogView
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/Component/TickBus.h>
#endif

namespace AzToolsFramework
{
    namespace LogPanel
    {
        //! TracePrintFLogPanel - an implementation of BaseLogPanel which shows recent traceprintfs
        //! You'd plug this into a UI of your choice and let it do its thing.
        //! You might want to also connect to the signal BaseLogPanel::TabsReset() which will get fired when the user says
        //! reset to default.
        class TracePrintFLogPanel
            : public BaseLogPanel
        {
            Q_OBJECT;
        public:
            // class allocator intentionally removed so that QT can make us in their auto-gen code
            //AZ_CLASS_ALLOCATOR(TracePrintFLogPanel, AZ::SystemAllocator, 0);

            TracePrintFLogPanel(QWidget* pParent = nullptr);

            void InsertLogLine(Logging::LogLine::LogType type, const char* window, const char* message, void* data);

        Q_SIGNALS:
            void LogLineSelected(const Logging::LogLine& logLine);

        private:
            QWidget* CreateTab(const TabSettings& settings) override;
        };


        //! AZTracePrintFLogTab - a Log View listening on AZ Traceprintfs and puts them in a ring buffer
        //! of particular interest is perhaps how it adds a "clear" option to the context menu in its constructor.
        //! it uses the RingBufferLogDataModel, below.
        class AZTracePrintFLogTab
            : public BaseLogView
            , protected AZ::Debug::TraceMessageBus::Handler
            , protected AZ::SystemTickBus::Handler
        {
            Q_OBJECT;
        public:
            AZ_CLASS_ALLOCATOR(AZTracePrintFLogTab, AZ::SystemAllocator, 0);
            AZTracePrintFLogTab(QWidget* pParent, const TabSettings& in_settings);
            virtual ~AZTracePrintFLogTab();

            //////////////////////////////////////////////////////////////////////////
            // TraceMessagesBus
            bool OnAssert(const char* message) override;
            bool OnException(const char* message) override;
            bool OnError(const char* window, const char* message) override;
            bool OnWarning(const char* window, const char* message) override;
            bool OnPrintf(const char* window, const char* message) override;
            //////////////////////////////////////////////////////////////////////////

            /// Log a message received from the TraceMessageBus
            void LogTraceMessage(Logging::LogLine::LogType type, const char* window, const char* message, void* data = nullptr);

            QTableView* GetLogView() { return m_ptrLogView; }

        Q_SIGNALS:
            void SelectionChanged(const Logging::LogLine& logLine);

        protected:
            void OnSystemTick() override;

            TabSettings m_settings;

            // note that we actually buffer the lines up since we could receive them at any time from this bus, even on another thread
            // so we dont push them to the GUI immediately.  Instead we connect to the tickbus and drain the queue on tick.

            AZStd::queue<Logging::LogLine> m_bufferedLines;
            AZStd::atomic_bool m_alreadyQueuedDrainMessage;  // we also only drain the queue at the end so that we do bulk inserts instead of one at a time.
            AZStd::mutex m_bufferedLinesMutex; // protects m_bufferedLines from draining and adding entries into it from different threads at the same time 

            void CurrentItemChanged(const QModelIndex& current, const QModelIndex& previous);

        private Q_SLOTS:
            void    DrainMessages();

        public slots:
            virtual void Clear();
        };
    } // namespace LogPanel
} // namespace AzToolsFramework
