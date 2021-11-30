/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required files
#include "MysticQtManager.h"
#include <MCore/Source/StringConversions.h>
#include <QtGui/QIcon>
#include <QDir>

namespace MysticQt
{
    // the global MysticQt manager
    MysticQtManager* gMysticQtManager = nullptr;

    //--------------------------------------------------

    // constructor
    MysticQtManager::MysticQtManager()
    {
        m_mainWindow = nullptr;
    }


    // destructor
    MysticQtManager::~MysticQtManager()
    {
        // get the number of icons and destroy them
        for (IconData* icon : m_icons)
        {
            delete icon;
        }
        m_icons.clear();
    }



    // constructor
    MysticQtManager::IconData::IconData(const char* filename)
    {
        m_fileName = filename;
        m_icon = new QIcon(QDir{ QString(GetMysticQt()->GetDataDir().c_str()) }.filePath(filename));
    }


    // destructor
    MysticQtManager::IconData::~IconData()
    {
        delete m_icon;
    }


    const QIcon& MysticQtManager::FindIcon(const char* filename)
    {
        // get the number of icons and iterate through them
        for (IconData* icon : m_icons)
        {
            if (AzFramework::StringFunc::Equal(icon->m_fileName.c_str(), filename, false /* no case */))
            {
                return *(icon->m_icon);
            }
        }

        // we haven't found it
        IconData* iconData = new IconData(filename);
        m_icons.emplace_back(iconData);
        return *(iconData->m_icon);
    }


    //--------------------------------------------------

    // initialize the MysticQt manager
    bool Initializer::Init(const char* appDir, const char* dataDir)
    {
        // create the gMCore object
        if (gMysticQtManager)
        {
            return true;
        }

        gMysticQtManager = new MysticQtManager();
        gMysticQtManager->SetAppDir(appDir);
        gMysticQtManager->SetDataDir(dataDir);
        return true;
    }


    // shutdown the MysticQt manager
    void Initializer::Shutdown()
    {
        // delete the manager
        delete gMysticQtManager;
        gMysticQtManager = nullptr;
    }
}   // namespace MysticQt
