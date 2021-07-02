/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
