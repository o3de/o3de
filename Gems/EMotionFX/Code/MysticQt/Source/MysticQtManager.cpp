/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mMainWindow = nullptr;
    }


    // destructor
    MysticQtManager::~MysticQtManager()
    {
        // get the number of icons and destroy them
        const uint32 numIcons = mIcons.GetLength();
        for (uint32 i = 0; i < numIcons; ++i)
        {
            delete mIcons[i];
        }
        mIcons.Clear();
    }



    // constructor
    MysticQtManager::IconData::IconData(const char* filename)
    {
        mFileName = filename;
        mIcon = new QIcon(QDir{ QString(GetMysticQt()->GetDataDir().c_str()) }.filePath(filename));
    }


    // destructor
    MysticQtManager::IconData::~IconData()
    {
        delete mIcon;
    }


    const QIcon& MysticQtManager::FindIcon(const char* filename)
    {
        // get the number of icons and iterate through them
        const uint32 numIcons = mIcons.GetLength();
        for (uint32 i = 0; i < numIcons; ++i)
        {
            if (AzFramework::StringFunc::Equal(mIcons[i]->mFileName.c_str(), filename, false /* no case */))
            {
                return *(mIcons[i]->mIcon);
            }
        }

        // we haven't found it
        IconData* iconData = new IconData(filename);
        mIcons.Add(iconData);
        return *(iconData->mIcon);
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
