/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "LogPanel_Panel.h"
#include "LogControl.h" // for BaseLogView
#endif

namespace AzToolsFramework
{
    namespace LogPanel
    {
        //! A log panel that is used for generic data.
        //! Used for generic data sources such as Log Files, or special case logging and forensic data that isn't Expected to constantly stream in over time.
        //! General usage:  Create the panel, put it in your GUI, add as many tabs as you want, using AddLogTab, then call FillFromFile(...) or AddLine(...)
        //! unlike the traceprintf view, this does not expect you to be filling it from many threads, so it has no thread safety on FillFromFile or AddLine - call those from the GUI thread.
        //! The tabs created will be separate views on the same log data.
        //! Note that if you want to have your own log file format you can derive from this base class and override the parseData function
        class GenericLogPanel
            : public BaseLogPanel
        {
            Q_OBJECT
        public:
            // class allocator intentionally removed so that QT can make us in their auto-gen code
            //AZ_CLASS_ALLOCATOR(StaticLogPanel, AZ::SystemAllocator);
            GenericLogPanel(QWidget* pParent = nullptr);

            //! Fill From File - append log data from a file
            //! You can call this repeatedly if you'd like.
            void FillFromFile(const AZStd::string& fileName);

            //! ParseData - override to read whatever format(s) you want.
            //! entireLog will be a null terminated buffer (extra null at the end).
            //! If your source data is binary then there will be an extra null.
            //! The length does not include that extra null, its there for convenience.
            virtual void ParseData(const char* entireLog, AZ::u64 logLength);

            //! calling AddLogLine consumes the given line (move operation) and updates all tabs
            void AddLogLine(Logging::LogLine& target);

            //! Whether tabs will expand the row height of their current item to show the full message text
            void SetCurrentItemsExpandToFit(bool expandToFit);

        protected:
            QWidget* CreateTab(const TabSettings& settings) override;

            ListLogDataModel m_dataModel;

            bool m_alreadyQueuedCommit = false;
        private Q_SLOTS:
            // we commit any added lines after a short delay so that data that is flooding in from a file does not cause constant refreshes.
            void CommitAddedLines();

        private:
            bool m_currentItemsExpandToFit = false;
        };

        class GenericLogTab
            : public BaseLogView
        {
            Q_OBJECT;
        public:
            AZ_CLASS_ALLOCATOR(GenericLogTab, AZ::SystemAllocator);
            GenericLogTab(QWidget* pParent, const TabSettings& in_settings);

            void SetDataSource(QAbstractItemModel* source);
        private:

            FilteredLogDataModel m_filteredModel;
        };
    } // namespace LogPanel
} // namespace AzToolsFramework

