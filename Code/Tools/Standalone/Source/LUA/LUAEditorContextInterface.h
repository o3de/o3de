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

#ifndef LUAEDITORCONTEXTINTERFACE_H
#define LUAEDITORCONTEXTINTERFACE_H

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorContextBus.h>
#include <AzCore/IO/SystemFile.h>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <windows.h>
#endif

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
