/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MYSTICQT_MANAGER_H
#define __MYSTICQT_MANAGER_H

// include required files
#if !defined(Q_MOC_RUN)
#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/containers/vector.h>
#include "MysticQtConfig.h"
#include <QWidget>
#endif


namespace MysticQt
{
    // the initializer
    class MYSTICQT_API Initializer
    {
    public:
        static bool MCORE_CDECL Init(const char* appDir, const char* dataDir = "");
        static void MCORE_CDECL Shutdown();
    };


    // the MysticQt manager class
    class MYSTICQT_API MysticQtManager
    {
        friend class Initializer;
        MCORE_MEMORYOBJECTCATEGORY(MysticQtManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT);

    public:
        MCORE_INLINE QWidget* GetMainWindow() const                         { return m_mainWindow; }
        MCORE_INLINE void SetMainWindow(QWidget* mainWindow)                { m_mainWindow = mainWindow; }

        MCORE_INLINE void SetAppDir(const char* appDir)
        {
            m_appDir = appDir;
            if (m_dataDir.size() == 0)
            {
                m_dataDir = appDir;
            }
        }
        MCORE_INLINE const AZStd::string& GetAppDir() const                 { return m_appDir; }

        MCORE_INLINE void SetDataDir(const char* dataDir)
        {
            m_dataDir = dataDir;
            if (m_appDir.size() == 0)
            {
                m_appDir = dataDir;
            }
        }
        MCORE_INLINE const AZStd::string& GetDataDir() const                { return m_dataDir; }

        const QIcon& FindIcon(const char* filename);

    private:
        struct MYSTICQT_API IconData
        {
            MCORE_MEMORYOBJECTCATEGORY(MysticQtManager::IconData, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MYSTICQT)

            IconData(const char* filename);
            ~IconData();

            QIcon*          m_icon;
            AZStd::string   m_fileName;
        };

        QWidget*                            m_mainWindow;
        AZStd::vector<IconData*>             m_icons;
        AZStd::string                       m_appDir;
        AZStd::string                       m_dataDir;

        MysticQtManager();
        ~MysticQtManager();
    };


    // the global
    extern MYSTICQT_API MysticQtManager* gMysticQtManager;

    // shortcuts
    MCORE_INLINE MysticQtManager*           GetMysticQt()                   { return MysticQt::gMysticQtManager; }
    MCORE_INLINE const AZStd::string&       GetAppDir()                     { return gMysticQtManager->GetAppDir(); }
    MCORE_INLINE const AZStd::string&       GetDataDir()                    { return gMysticQtManager->GetDataDir(); }
}   // namespace MysticQt

#endif
