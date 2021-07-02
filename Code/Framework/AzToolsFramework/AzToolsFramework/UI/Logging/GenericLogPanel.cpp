/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"

#include "GenericLogPanel.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/IO/LocalFileIO.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QDateTime::d': class 'QSharedDataPointer<QDateTimePrivate>' needs to have dll-interface to be used by clients of class 'QDateTime'
#include <QDateTime>
AZ_POP_DISABLE_WARNING
#include <QTimer>

#include <AzToolsFramework/UI/Logging/LogLine.h>
#include <AzFramework/Logging/LogFile.h>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        GenericLogPanel::GenericLogPanel(QWidget* pParent /* = nullptr*/)
            : BaseLogPanel(pParent)
        {
        }

        QWidget* GenericLogPanel::CreateTab(const TabSettings& settings)
        {
            GenericLogTab* newTab = new GenericLogTab(this, settings);
            newTab->SetDataSource(&m_dataModel);
            newTab->SetCurrentItemExpandsToFit(m_currentItemsExpandToFit);
            return newTab;
        }

        //! Fill From File - append log data from a file
        //! You can call this repeatedly if you'd like.
        void GenericLogPanel::FillFromFile(const AZStd::string& fileName)
        {
            using namespace AZ::IO;

            FileIOBase* mainInstance = FileIOBase::GetInstance();

            // delete localInstance if it goes out of scope for any reason:
            AZStd::unique_ptr<FileIOBase> localInstance;
            if (!mainInstance)
            {
                // create one locally
                localInstance.reset(new LocalFileIO());
                mainInstance = localInstance.get();
            }

            HandleType openedFile = InvalidHandle;
            if (!mainInstance->Open(fileName.c_str(), OpenMode::ModeRead, openedFile))
            {
                Logging::LogLine line(AZStd::string::format("unable to open the log file at %s", fileName.c_str()).c_str(), "LOGGING", Logging::LogLine::TYPE_WARNING, QDateTime::currentMSecsSinceEpoch());
                AddLogLine(line);
                CommitAddedLines();
                return;
            }

            AZ::u64 logSize = 0;
            if (!mainInstance->Size(openedFile, logSize))
            {
                Logging::LogLine line(AZStd::string::format("unable to read the size of the log file at %s", fileName.c_str()).c_str(), "LOGGING", Logging::LogLine::TYPE_WARNING, QDateTime::currentMSecsSinceEpoch());
                AddLogLine(line);
                mainInstance->Close(openedFile);
                CommitAddedLines();
                return;
            }

            AZStd::vector<char> tempStorage;
            tempStorage.resize_no_construct(logSize + 1);
            if (!mainInstance->Read(openedFile, tempStorage.data(), logSize, true))
            {
                Logging::LogLine line(AZStd::string::format("Unable to read %llu bytes from log file %s", logSize, fileName.c_str()).c_str(), "LOGGING", Logging::LogLine::TYPE_WARNING, QDateTime::currentMSecsSinceEpoch());
                AddLogLine(line);
                mainInstance->Close(openedFile);
                CommitAddedLines();
                return;
            }

            tempStorage[logSize] = 0;
            ParseData(tempStorage.data(), logSize);

            mainInstance->Close(openedFile);
        }

        void GenericLogPanel::ParseData(const char* entireLog, AZ::u64 logLength)
        {
            Logging::LogLine::ParseLog(entireLog, logLength, 
                AZStd::bind(&GenericLogPanel::AddLogLine, this, AZStd::placeholders::_1));

            CommitAddedLines();
        }

        //! calling AddLogLine consumes the given line (move operation) and updates all tabs
        void GenericLogPanel::AddLogLine(Logging::LogLine& target)
        {
            m_dataModel.AppendLine(target);
            if (!m_alreadyQueuedCommit)
            {
                m_alreadyQueuedCommit = true;
                QMetaObject::invokeMethod(this, "CommitAddedLines", Qt::QueuedConnection);
            }
        }

        void GenericLogPanel::SetCurrentItemsExpandToFit(bool expandToFit)
        {
            m_currentItemsExpandToFit = expandToFit;
            for (auto logTab: findChildren<GenericLogTab*>())
                logTab->SetCurrentItemExpandsToFit(expandToFit);
        }

        void GenericLogPanel::CommitAddedLines()
        {
            m_alreadyQueuedCommit = false;
            m_dataModel.CommitAdd();
        }

        GenericLogTab::GenericLogTab(QWidget* pParent, const TabSettings& in_settings)
            : BaseLogView(pParent)
        {
            m_filteredModel.SetTabSettings(in_settings);
        }

        void GenericLogTab::SetDataSource(QAbstractItemModel* source)
        {
            m_filteredModel.setSourceModel(source);
            ConnectModelToView(&m_filteredModel);
        }

    } // namespace LogPanel
} // namespace AzToolsFramework

#include "UI/Logging/moc_GenericLogPanel.cpp"
