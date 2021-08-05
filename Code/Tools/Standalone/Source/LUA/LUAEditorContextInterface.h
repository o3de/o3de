/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef LUAEDITORCONTEXTINTERFACE_H
#define LUAEDITORCONTEXTINTERFACE_H

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorContextBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzCore/PlatformIncl.h>
#include <CryCommon/platform.h>

namespace LUAEditor
{
    struct DocumentInfo
    {
        AZStd::string m_assetId; // the assetId

        AZ::IO::SystemFile m_scriptFile;
        //AZ::Data::Asset<AZ::ScriptAsset> m_scriptAsset; //this is the documents data
        AZStd::string m_scriptAsset;

        AZStd::string m_assetName; // relative asset name
        AZStd::string m_displayName; // for friendly display only

        FILETIME m_lastKnownModTime;
        AzToolsFramework::SourceControlFileInfo m_sourceControlInfo;

        bool m_bSourceControl_Ready; // a result (or failure) came back from SCC.  Until this is true, you cannot write to it.
        bool m_bSourceControl_BusyGettingStats; // a perforce stat operation is pending
        bool m_bSourceControl_BusyRequestingEdit; // a perforce edit request operation is pending
        bool m_bSourceControl_CanWrite; // you are allowed to edit and save this file
        bool m_bSourceControl_CanCheckOut; // you may be able to check this file out (Actually attempting to may still fail due to changes on the net)

        bool m_bDataIsLoaded;
        bool m_bDataIsWritten;
        bool m_bCloseAfterSave;
        bool m_bUntitledDocument;

        bool m_bIsModified;

        bool m_bIsBeingSaved;

        int m_PresetLineAtOpen; // auto-position to this offset when data is loaded

        DocumentInfo()
            : m_bSourceControl_Ready(false)
            , m_bSourceControl_BusyGettingStats(false)
            , m_bSourceControl_BusyRequestingEdit(false)
            , m_bSourceControl_CanWrite(false)
            , m_bSourceControl_CanCheckOut(false)
            //, m_assetId(AZ::Data::AssetId::CreateNull())
            , m_bDataIsLoaded(false)
            , m_bDataIsWritten(true)
            , m_bCloseAfterSave(false)
            , m_bUntitledDocument(false)
            , m_bIsModified(false)
            , m_bIsBeingSaved(false)
            , m_PresetLineAtOpen(1){}

        // Copy constructor does not copy over open file handle
        DocumentInfo(const DocumentInfo& other)
            : m_assetId(other.m_assetId)
            , m_scriptAsset(other.m_scriptAsset)
            , m_assetName(other.m_assetName)
            , m_displayName(other.m_displayName)
            , m_lastKnownModTime(other.m_lastKnownModTime)
            , m_sourceControlInfo(other.m_sourceControlInfo)
            , m_bSourceControl_Ready(other.m_bSourceControl_Ready)
            , m_bSourceControl_BusyGettingStats(other.m_bSourceControl_BusyGettingStats)
            , m_bSourceControl_BusyRequestingEdit(other.m_bSourceControl_BusyRequestingEdit)
            , m_bSourceControl_CanWrite(other.m_bSourceControl_CanWrite)
            , m_bSourceControl_CanCheckOut(other.m_bSourceControl_CanCheckOut)
            , m_bDataIsLoaded(other.m_bDataIsLoaded)
            , m_bDataIsWritten(other.m_bDataIsWritten)
            , m_bCloseAfterSave(other.m_bCloseAfterSave)
            , m_bUntitledDocument(other.m_bUntitledDocument)
            , m_bIsModified(other.m_bIsModified)
            , m_bIsBeingSaved(other.m_bIsBeingSaved)
            , m_PresetLineAtOpen(other.m_PresetLineAtOpen)
        {}

        DocumentInfo& operator=(const DocumentInfo& other)
        {
            m_assetId = other.m_assetId;
            m_scriptAsset = other.m_scriptAsset;
            m_assetName = other.m_assetName;
            m_displayName = other.m_displayName;
            m_lastKnownModTime = other.m_lastKnownModTime;
            m_sourceControlInfo = other.m_sourceControlInfo;
            m_bSourceControl_Ready = other.m_bSourceControl_Ready;
            m_bSourceControl_BusyGettingStats = other.m_bSourceControl_BusyGettingStats;
            m_bSourceControl_BusyRequestingEdit = other.m_bSourceControl_BusyRequestingEdit;
            m_bSourceControl_CanWrite = other.m_bSourceControl_CanWrite;
            m_bSourceControl_CanCheckOut = other.m_bSourceControl_CanCheckOut;
            m_bDataIsLoaded = other.m_bDataIsLoaded;
            m_bDataIsWritten = other.m_bDataIsWritten;
            m_bCloseAfterSave = other.m_bCloseAfterSave;
            m_bUntitledDocument = other.m_bUntitledDocument;
            m_bIsModified = other.m_bIsModified;
            m_bIsBeingSaved = other.m_bIsBeingSaved;
            m_PresetLineAtOpen = other.m_PresetLineAtOpen;

            return *this;
        }
    };

    class ContextInterface
        : public LegacyFramework::EditorContextMessages
    {
    public:
        typedef AZ::EBus<ContextInterface> Bus;
        typedef Bus::Handler Handler;

        virtual void ShowLUAEditorView() = 0;
    };
}

#endif
