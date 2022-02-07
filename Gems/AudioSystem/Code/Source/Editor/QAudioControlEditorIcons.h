/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <QIcon>
#include <AudioControl.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    inline QIcon GetControlTypeIcon(EACEControlType type)
    {
        QString iconFile;

        switch (type)
        {
        case AudioControls::eACET_TRIGGER:
            iconFile = ":/Icons/Trigger_Icon.svg";
            break;
        case AudioControls::eACET_RTPC:
            iconFile = ":/Icons/RTPC_Icon.svg";
            break;
        case AudioControls::eACET_SWITCH:
            iconFile = ":/Icons/Switch_Icon.svg";
            break;
        case AudioControls::eACET_SWITCH_STATE:
            iconFile = ":/Icons/Property_Icon.png";
            break;
        case AudioControls::eACET_ENVIRONMENT:
            iconFile = ":/Icons/Environment_Icon.svg";
            break;
        case AudioControls::eACET_PRELOAD:
            iconFile = ":/Icons/Preload_Icon.svg";
            break;
        default:
            // should make a "default"/empty icon...
            iconFile = ":/Icons/Unassigned.svg";
        }

        QIcon icon(iconFile);
        icon.addFile(iconFile, QSize(), QIcon::Selected);
        return icon;
    }

    //-------------------------------------------------------------------------------------------//
    inline QIcon GetFolderIcon()
    {
        QIcon icon = QIcon(":/Icons/Folder_Icon.svg");
        icon.addFile(":/Icons/Folder_Icon_Selected.svg", QSize(), QIcon::Selected);
        return icon;
    }

    //-------------------------------------------------------------------------------------------//
    inline QIcon GetSoundBankIcon()
    {
        QIcon icon = QIcon(":/Icons/Preload_Icon.svg");
        icon.addFile(":/Icons/Preload_Icon.svg", QSize(), QIcon::Selected);
        return icon;
    }

    //-------------------------------------------------------------------------------------------//
    inline QIcon GetGroupIcon(int group)
    {
        const int numberOfGroups = 5;
        group = group % numberOfGroups;
        QString path;
        switch (group)
        {
        case 0:
            path = ":/Icons/folder purple.svg";
            break;
        case 1:
            path = ":/Icons/folder blue.svg";
            break;
        case 2:
            path = ":/Icons/folder green.svg";
            break;
        case 3:
            path = ":/Icons/folder red.svg";
            break;
        case 4:
            path = ":/Icons/folder yellow.svg";
            break;
        default:
            path = "Icons/folder red.svg";
            break;
        }

        QIcon icon;
        icon.addFile(path);
        icon.addFile(path, QSize(), QIcon::Selected);
        return icon;
    }
} // namespace AudioControls
