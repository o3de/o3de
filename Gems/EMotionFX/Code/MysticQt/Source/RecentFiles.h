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

#pragma once

#if !defined(Q_MOC_RUN)
#include "MysticQtConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include <QMenu>
#include <QObject>
#endif


namespace MysticQt
{
    class MYSTICQT_API RecentFiles
        : public QObject
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(RecentFiles, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT)

    public:
        RecentFiles();
        ~RecentFiles() = default;

        void Init(QMenu* parentMenu, size_t numRecentFiles, const char* subMenuName, const char* configStringName);
        void SetMaxRecentFiles(size_t numRecentFiles);
        void AddRecentFile(AZStd::string filename);
        AZStd::string GetLastRecentFileName() const;

    signals:
        void OnRecentFile(QAction* action);

    private slots:
        void OnClearRecentFiles();
        void OnRecentFileSlot();

    private:
        void UpdateMenu();
        void RemoveDuplicates();

        void Save();
        void Load();

        QStringList     m_recentFiles;
        size_t          m_maxNumRecentFiles;
        QMenu*          m_recentFilesMenu;
        QAction*        m_resetRecentFilesAction;
        QString         m_configStringName;
    };
} // namespace MysticQt
