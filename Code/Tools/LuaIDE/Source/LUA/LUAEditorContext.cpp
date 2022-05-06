/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include "LUAEditorContext.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContextAttributes.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Asset/AssetProcessorMessages.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/UI/UICore/SaveChangesDialog.hxx>
#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <AzToolsFramework/UI/Logging/LogPanel_Panel.h>
#include <AzToolsFramework/UI/UICore/QTreeViewStateSaver.hxx>
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>

#include <Source/LUA/LUALocalsTrackerMessages.h>

#include <QMessageBox>
#include <regex>

#include "LUAEditorContextInterface.h"
#include "LUAEditorDebuggerMessages.h"
#include "LUAWatchesDebuggerMessages.h"
#include "LUAEditorViewMessages.h"
#include "LUAContextControlMessages.h"

namespace LUAEditor
{
    const char* LUAEditorDebugName = "Lua Debug";
    const char* LUAEditorInfoName = "Lua Editor";

    LUAEditor::Context* s_pLUAEditorScriptPtr = nullptr; // for script access

    class BreakpointSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(BreakpointSavedState, "{EB3E0061-75AC-41F7-8631-6072F6C018EB}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(BreakpointSavedState, AZ::SystemAllocator, 0);
        BreakpointSavedState() {}
        BreakpointMap m_Breakpoints;
        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<BreakpointSavedState>()
                    ->Field("m_Breakpoints", &BreakpointSavedState::m_Breakpoints)
                    ->Version(2);
            }
        }
    };

    AZ::Uuid ContextID("37BA0B6A-CFCF-42CA-91A5-E794BB17AD6D");


    //////////////////////////////////////////////////////////////////////////
    //Context
    Context::Context()
        : m_pLUAEditorMainWindow(nullptr)
        , m_connectedState(false)
        , m_fileIO(nullptr)
    {
        m_referenceModel = new QStandardItemModel();
        m_numOutstandingOperations = 0;
        s_pLUAEditorScriptPtr = this;
        m_bProcessingActivate = false;
        m_bReloadCheckQueued = false;
        mostRecentlyOpenedDocumentView.clear();
        m_queuedOpenRecent = false;
        m_bShuttingDown = false;

        AddDefaultLUAKeywords();
        AddDefaultLUALibraryFunctions();
    }

    Context::~Context()
    {
        s_pLUAEditorScriptPtr = nullptr;
        delete m_referenceModel;

        for (auto errorData : m_errorData)
        {
            delete errorData;
        }

        m_errorData.clear();

        AZ_Assert(m_numOutstandingOperations == 0, "Save still pending when shut down.");
        AZ_Assert(!m_pLUAEditorMainWindow, "You must deactivate this Context");
    }

    Context::Context(const Context&)
     {
         AZ_Assert(false, "You can't copy this type!");
     }

    //////////////////////////////////////////////////////////////////////////
    // AZ::Component
    void Context::Init()
    {
        m_fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(m_fileIO, "FileIO system is not present, make sure a FileIO instance is set by the application.");
    }

    void Context::Activate()
    {
        ResetTargetContexts();

        LegacyFramework::CoreMessageBus::Handler::BusConnect();
        ContextInterface::Handler::BusConnect(ContextID);
        Context_DocumentManagement::Handler::BusConnect();
        Context_DebuggerManagement::Handler::BusConnect();
        LUABreakpointRequestMessages::Handler::BusConnect();
        LUAStackRequestMessages::Handler::BusConnect();
        AzFramework::TargetManagerClient::Bus::Handler::BusConnect();
        LUAWatchesRequestMessages::Handler::BusConnect();
        LUATargetContextRequestMessages::Handler::BusConnect();
        HighlightedWords::Handler::BusConnect();
        AzFramework::AssetSystemInfoBus::Handler::BusConnect();

        // Connect to source control
        using SCConnectionBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCConnectionBus::Broadcast(&SCConnectionBus::Events::EnableSourceControl, true);

        //AzToolsFramework::RegisterAssetType(AzToolsFramework::RegisteredAssetType(ContextID, AZ::ScriptAsset::StaticAssetType(), "Script", ".lua", true, 0));

        m_pBreakpointSavedState = AZ::UserSettings::CreateFind<BreakpointSavedState>(AZ_CRC("BreakpointSavedState", 0xbb65be3a), AZ::UserSettings::CT_LOCAL);

        AzToolsFramework::MainWindowDescription desc;
        desc.name = "LUA Editor";
        desc.ContextID = ContextID;
        desc.hotkeyDesc = AzToolsFramework::HotkeyDescription(AZ_CRC("LUAOpenEditor", 0x5870cf6d), "Ctrl+Shift+L", "Open LUA Editor", "General", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW);
        EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, AddComponentInfo, desc);

        EBUS_EVENT_RESULT(m_ipcOpenFilesHandle, LegacyFramework::IPCCommandBus, RegisterIPCHandler, "open_files", AZStd::bind(&Context::OnIPCOpenFiles, this, AZStd::placeholders::_1));

        bool connectedToAssetProcessor = false;

        // When the AssetProcessor is already launched it should take less than a second to perform a connection
        // but when the AssetProcessor needs to be launch it could take up to 15 seconds to have the AssetProcessor initialize
        // and able to negotiate a connection when running a debug build
        // and to negotiate a connection

        AzFramework::AssetSystem::ConnectionSettings connectionSettings;
        AzFramework::AssetSystem::ReadConnectionSettingsFromSettingsRegistry(connectionSettings);
        connectionSettings.m_connectionDirection = AzFramework::AssetSystem::ConnectionSettings::ConnectionDirection::ConnectToAssetProcessor;
        connectionSettings.m_connectionIdentifier = desc.name;
        AzFramework::AssetSystemRequestBus::BroadcastResult(connectedToAssetProcessor,
            &AzFramework::AssetSystemRequestBus::Events::EstablishAssetProcessorConnection, connectionSettings);
        if (!connectedToAssetProcessor)
        {
            AZ_TracePrintf(desc.name.c_str(), "%s was not able to connect to the Asset Processor. Please ensure that the Asset Processor is running.", desc.name.c_str());
        }

    }

    void Context::Deactivate()
    {
        EBUS_EVENT(LegacyFramework::IPCCommandBus, UnregisterIPCHandler, m_ipcOpenFilesHandle);

        //AzToolsFramework::UnRegisterAssetType(AZ::ScriptAsset::StaticAssetType());
        LUATargetContextRequestMessages::Handler::BusDisconnect();
        LUAWatchesRequestMessages::Handler::BusDisconnect();
        LegacyFramework::CoreMessageBus::Handler::BusDisconnect();
        //AzToolsFramework::AssetManagementMessages::Handler::BusDisconnect(ContextID);
        ContextInterface::Handler::BusDisconnect(ContextID);
        Context_DocumentManagement::Handler::BusDisconnect();
        Context_DebuggerManagement::Handler::BusDisconnect();
        LUAStackRequestMessages::Handler::BusDisconnect();
        LUABreakpointRequestMessages::Handler::BusDisconnect();
        AzFramework::TargetManagerClient::Bus::Handler::BusDisconnect();
        HighlightedWords::Handler::BusDisconnect();
        AzFramework::AssetSystemInfoBus::Handler::BusDisconnect();

    }

    void Context::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetProcessorToolsConnection", 0x734669bc));
    }

    void Context::ApplicationDeactivated()
    {
    }

    void Context::ApplicationActivated()
    {
        if (m_bShuttingDown)
        {
            return;
        }
        if (m_bProcessingActivate)
        {
            return;
        }

        RefreshAllDocumentPerforceStat();

        // Open any files we specified in the command line.
        if (!m_filesToOpen.empty())
        {
            for (const auto& file : m_filesToOpen)
            {
                OpenAssetByPhysicalPath(file.c_str());
            }

            m_filesToOpen.clear();
        }

        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->SetupLuaFilesPanel();
        }
    }

    bool Context::OnIPCOpenFiles(const AZStd::string& parameters)
    {
        if (parameters.empty())
        {
            return false;
        }

        AZStd::vector<AZStd::string> files;
        AZ::StringFunc::Tokenize(parameters.c_str(), files, ";");
        if (!files.empty())
        {
            for (const auto& file : files)
            {
                OpenAssetByPhysicalPath(file.c_str());
            }

            if (m_pLUAEditorMainWindow)
            {
                if (m_pLUAEditorMainWindow->isMinimized())
                {
                    m_pLUAEditorMainWindow->showNormal();
                }
                else
                {
                    m_pLUAEditorMainWindow->show();
                }

                m_pLUAEditorMainWindow->raise();
                m_pLUAEditorMainWindow->activateWindow();
                m_pLUAEditorMainWindow->setFocus();


                EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
            }

            return true;
        }

        return false;
    }

    void Context::ApplicationShow(AZ::Uuid id)
    {
        if (ContextID == id)
        {
            ProvisionalShowAndFocus(true);
        }
    }
    void Context::ApplicationHide(AZ::Uuid id)
    {
        if (ContextID == id)
        {
            bool allowed = OnGetPermissionToShutDown(); // everyone must agree to hide before we can

            if (allowed)
            {
                m_pLUAEditorMainWindow->hide();
                auto newState = AZ::UserSettings::CreateFind<LUAEditorContextSavedState>(AZ_CRC("LUA EDITOR CONTEXT STATE", 0xc20c7427), AZ::UserSettings::CT_LOCAL);
                newState->m_MainEditorWindowIsVisible = false;
            }
        }
    }
    void Context::ApplicationCensus()
    {
        auto newState = AZ::UserSettings::CreateFind<LUAEditorContextSavedState>(AZ_CRC("LUA EDITOR CONTEXT STATE", 0xc20c7427), AZ::UserSettings::CT_LOCAL);
        EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, ApplicationCensusReply, newState->m_MainEditorWindowIsVisible);
    }

    void Context::AddDefaultLUAKeywords()
    {
        AZStd::string keywords[] = {
            {"and"}, {"false"}, {"local"}, {"then"}, {"break"}, {"for"}, {"nil"}, {"true"}, {"do"},
            {"function"}, {"not"}, {"until"}, {"else"}, {"goto"}, {"or"}, {"while"}, {"elseif"}, {"if"}, {"repeat"},
            {"end"}, {"in"}, {"return"}
        };

        m_LUAKeywords.insert(std::begin(keywords), std::end(keywords));
    }

    void Context::AddDefaultLUALibraryFunctions()
    {
        AZStd::string librarys[] = {
            {"assert"}, {"collectgarbage"}, {"next"}, {"pairs"}, {"pcall"}, {"rawequal"}, {"rawget"}, {"rawlen"}, {"rawset"},
            {"select"}, {"setmetatable"}, {"tonumber"}, {"tostring"}, {"type"}, {"_VERSION"}, {"xpcall"}, {"coroutine.create"}, {"coroutine.resume"}, {"coroutine.running"},
            {"coroutine.status"}, {"coroutine.wrap"}, {"coroutine.yield"}, {"string.byte"}, {"string.char"}, {"string.dump"}, {"string.find"}, {"string.format"},
            {"string.gmatch"}, {"string.gsub"}, {"string.len"}, {"string.lower"}, {"string.match"}, {"string.rep"}, {"string.reverse"}, {"string.sub"}, {"string.upper"},
            {"table.concat"}, {"table.insert"}, {"table.pack"}, {"table.remove"}, {"table.sort"}, {"table.unpack"}, {"math.abs"}, {"math.acos"}, {"math.asin"},
            {"math.atan"}, {"math.atan2"}, {"math.ceil"}, {"math.cos"}, {"math.cosh"}, {"math.deg"}, {"math.exp"}, {"math.floor"}, {"math.fmod"}, {"math.frexp"},
            {"math.huge"}, {"math.ldexp"}, {"math.log"}, {"math.max"}, {"math.min"}, {"math.modf"}, {"math.pi"}, {"math.pow"}, {"math.rad"}, {"math.random"}, {"math.randomseed"},
            {"math.sin"}, {"math.sinh"}, {"math.sqrt"}, {"math.tan"}, {"math.tanh"}, {"bit32.arshift"}, {"bit32.band"}, {"bit32.bnot"}, {"bit32.bor"}, {"bit32.btest"},
            {"bit32.bxor"}, {"bit32.extract"}, {"bit32.replace"}, {"bit32.lrotate"}, {"bit32.lshift"}, {"bit32.rrotate"}, {"bit32.rshift"}
        };

        m_LUALibraryFunctions.insert(std::begin(librarys), std::end(librarys));
    }

    void Context::RefreshAllDocumentPerforceStat()
    {
        m_bProcessingActivate = true;
        AZ_TracePrintf(LUAEditorDebugName, "Entry refreshalldocumentperforceStat()\n");

        for (DocumentInfoMap::iterator it = m_documentInfoMap.begin(); it != m_documentInfoMap.end(); ++it)
        {
            DocumentInfo& info = it->second;
            {
                AZ_TracePrintf(LUAEditorDebugName, "Refreshing Perforce for assetId '%s' '%s'\n", info.m_assetId.c_str(), info.m_assetName.c_str());
            }

            if (m_fileIO && !info.m_bUntitledDocument)
            {
                // check for updated modtime:

                if (!m_fileIO->Exists(info.m_assetId.c_str()))
                {
                    AZ_TracePrintf(LUAEditorDebugName, "During Refresh, a document appears to have been removed from disk: \"%s\"\n", info.m_assetName.c_str());

                    // this can happen if they mark something for delete...
                    info.m_bIsModified = true;
                    info.m_bSourceControl_CanWrite = true;
                    if (m_pLUAEditorMainWindow)
                    {
                        m_pLUAEditorMainWindow->OnDocumentInfoUpdated(info);
                }
                }
                else
                {
                    uint64_t lastKnownModTime = (AZ::u64)info.m_lastKnownModTime.dwHighDateTime << 32 | info.m_lastKnownModTime.dwLowDateTime;
                    uint64_t modTime = m_fileIO->ModificationTime(info.m_assetId.c_str());

                    if (lastKnownModTime != modTime)
                    {
                        // ruh oh!  The file time of the asset changed - someone reverted, modified, etc.  What do we do?
                        // do we have unsaved changes?
                        info.m_lastKnownModTime.dwHighDateTime = static_cast<DWORD>(modTime >> 32);
                        info.m_lastKnownModTime.dwLowDateTime = static_cast<DWORD>(modTime);

                        {
                            AZ_TracePrintf(LUAEditorDebugName, "Document modtime has changed, queueing reload of '%s' '%s'\n", info.m_assetId.c_str(), info.m_assetName.c_str());
                        }

                        // async crossover to test files being written against asset changes
                        if (info.m_bDataIsWritten)
                        {
                            m_reloadCheckDocuments.insert(info.m_assetId);
                            if (!m_bReloadCheckQueued)
                            {
                                m_bReloadCheckQueued = true;
                                EBUS_QUEUE_FUNCTION(AZ::SystemTickBus, &Context::ProcessReloadCheck, this);
                            }
                        }
                    }

                    if (!info.m_bSourceControl_BusyGettingStats)
                    {
                        // it's OK to skip getting fresh file info from Perforce here
                        // because we've already given the go-ahead to exit the Application
                        if (!m_bShuttingDown)
                        {
                            {
                                AZ_TracePrintf(LUAEditorDebugName, "Queuing P4 Refresh of '%s' '%s'\n", info.m_assetId.c_str(), info.m_assetName.c_str());
                            }
                            info.m_bSourceControl_BusyGettingStats = true;
                            // while we're reading it, fetch the perforce information for it:
                            //AZ_TracePrintf("Debug","    ++m_numOutstandingOperations %d\n",__LINE__);
                            ++m_numOutstandingOperations;
                            using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
                            SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo,
                                info.m_assetId.c_str(),
                                AZStd::bind(&Context::PerforceStatResponseCallback,
                                this,
                                AZStd::placeholders::_1,
                                AZStd::placeholders::_2,
                                info.m_assetId)
                                );

                            // check for updated modtime, too...
                        }
                    }
                }
            }
        }

        m_bProcessingActivate = false;
        AZ_TracePrintf(LUAEditorDebugName, "Finished refreshalldocumentperforceStat()\n");
    }


    void Context::ProcessReloadCheck()
    {
        AZ_TracePrintf(LUAEditorDebugName, "ProcessReloadCheck ProcessReloadCheck()\n");
        m_bReloadCheckQueued = false;
        for (auto currentDoc = m_reloadCheckDocuments.begin(); currentDoc != m_reloadCheckDocuments.end(); ++currentDoc)
        {
            DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(*currentDoc);
            if (docInfoIter == m_documentInfoMap.end())
            {
                continue;
            }

            DocumentInfo& info = docInfoIter->second;
            {
                AZ_TracePrintf(LUAEditorDebugName, "ProcessReloadCheck inspecting assetId '%s' '%s'\n", info.m_assetId.c_str(), info.m_assetName.c_str());
            }

            auto newState = AZ::UserSettings::CreateFind<LUAEditorMainWindowSavedState>(AZ_CRC("LUA EDITOR MAIN WINDOW STATE", 0xa181bc4a), AZ::UserSettings::CT_LOCAL);

            // Check to see if it is unmodified and the setting is set to auto-reload unmodified files
            bool shouldAutoReload = newState->m_bAutoReloadUnmodifiedFiles && !info.m_bIsModified;
            bool shouldReload = false;

            if (!shouldAutoReload)
            {
                // we may have unsaved changes:
                QMessageBox msgBox(this->m_pLUAEditorMainWindow);
                msgBox.setText("A file has been modified by an outside program. Would you like to reload it from disk? If you do, you will lose any unsaved changes.");
                msgBox.setInformativeText(info.m_assetName.c_str());
                msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                msgBox.setButtonText(QMessageBox::Yes, "Reload From Disk");
                msgBox.setButtonText(QMessageBox::No, "Don't reload");
                msgBox.setDefaultButton(QMessageBox::No);
                msgBox.setIcon(QMessageBox::Question);
                shouldReload = (msgBox.exec() == QMessageBox::Yes);
            }

            if (shouldAutoReload || shouldReload)
            {
                // queue document reopen!
                {
                    AZ_TracePrintf(LUAEditorDebugName, "ProcessReloadCheck user queueing reload for assetId '%s' '%s'\n", info.m_assetId.c_str(), info.m_assetName.c_str());
                }
                EBUS_QUEUE_FUNCTION(AZ::SystemTickBus, &Context::OnReloadDocument, this, info.m_assetId);
            }
            else
            {
                // document remains open and modified, we don't overwrite or reload!
                // but also don't prompt them -update the modtime.
                if (!info.m_assetId.empty())
                {
                    if (m_fileIO)
                    {
                        uint64_t modTime = m_fileIO->ModificationTime(info.m_assetId.c_str());

                        info.m_lastKnownModTime.dwHighDateTime = static_cast<DWORD>(modTime >> 32);
                        info.m_lastKnownModTime.dwLowDateTime = static_cast<DWORD>(modTime);
                    }
                }
            }
        }
        AZ_TracePrintf(LUAEditorDebugName, "Exit ProcessReloadCheck()\n");
    }

    void Context::DesiredTargetConnected(bool connected)
    {
        AZ_TracePrintf(LUAEditorDebugName, "Context::DesiredTargetConnected( %d )\n", connected);

        if (connected)
        {
            EBUS_EVENT(LUAEditorDebuggerMessages::Bus, EnumerateContexts);
            EBUS_EVENT(LUAEditorMainWindowMessages::Bus, OnConnectedToTarget);
            EBUS_EVENT(LUAEditor::Context_ControlManagement::Bus, OnTargetConnected);
            m_connectedState = true;

        }
        else
        {
            EBUS_EVENT(LUAEditorMainWindowMessages::Bus, OnDisconnectedFromTarget);
            EBUS_EVENT(LUAEditor::Context_ControlManagement::Bus, OnTargetDisconnected);
            EBUS_EVENT(LUAEditor::Context_DebuggerManagement::Bus, OnDebuggerDetached);
            m_connectedState = false;

        }
    }

    void Context::DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID)
    {
        (void)oldTargetID;
        (void)newTargetID;

        AZ_TracePrintf(LUAEditorDebugName, "Context::RemoteTargetChanged()\n");

        RequestDetachDebugger();
    }

    //////////////////////////////////////////////////////////////////////////
    //BreakpointTracker Request Messages
    const BreakpointMap* Context::RequestBreakpoints()
    {
        return &m_pBreakpointSavedState->m_Breakpoints;
    }

    //////////////////////////////////////////////////////////////////////////
    //StackTracker Request Messages
    void Context::RequestStackClicked(const AZStd::string& stackString, int lineNumber)
    {
        // incoming display string looks like this:
        /*
        "[Lua] gameinfo\script\player\playercharacter_strider (587) : PreStateTick(37BA18D8, 0.033333)"
        */
        // outgoing string for the asset name should look like this:
        /*
        "gameinfo\script\player\playercharacter_strider"
        */

        QString script = stackString.c_str();
        script = script.right(script.length() - 6);
        script = script.left(script.indexOf(" "));

        RequestEditorFocus(script.toUtf8().data(), lineNumber);
    }

    //////////////////////////////////////////////////////////////////////////
    //TargetContextTracker Request Messages
    const AZStd::vector<AZStd::string> Context::RequestTargetContexts()
    {
        return m_targetContexts;
    }

    const AZStd::string Context::RequestCurrentTargetContext()
    {
        return m_currentTargetContext;
    }

    void Context::RequestEditorFocus(const AZStd::string& relativePath, int lineNumber)
    {
        AZStd::to_lower(const_cast<AZStd::string&>(relativePath).begin(), const_cast<AZStd::string&>(relativePath).end());

        bool foundAbsolutePath = false;
        AZStd::string absolutePath;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(foundAbsolutePath,
            &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, relativePath, absolutePath);

        bool fileFound = false;
        if (foundAbsolutePath)
        {
            AZStd::to_lower(const_cast<AZStd::string&>(absolutePath).begin(), const_cast<AZStd::string&>(absolutePath).end());
            auto docInfoIter = m_documentInfoMap.find(absolutePath);
            fileFound = docInfoIter != m_documentInfoMap.end();
        }

        if (!fileFound)
        {
            // Check if we have a relative path, we may still be able to open the file (this may happen when double clicking on a stack panel entry)
            for (const auto& doc : m_documentInfoMap)
            {
                const char* result = strstr(doc.first.c_str(), relativePath.c_str());
                if (result)
                {
                    absolutePath = doc.second.m_assetId;
                    fileFound = true;
                    break;
                }
            }

            if (!fileFound)
            {
                // the document was probably closed.
                if (foundAbsolutePath)
                { 
                    AssetOpenRequested(absolutePath, true);
                }
                else
                {
                    AssetOpenRequested(relativePath, true);
                }
                return;
            }
        }

        ProvisionalShowAndFocus();

        // tell the view that it needs to focus that document!
        m_pLUAEditorMainWindow->OnRequestFocusView(absolutePath);
        m_pLUAEditorMainWindow->MoveEditCursor(absolutePath, lineNumber, true);
    }

    void Context::RequestDeleteBreakpoint(const AZStd::string& assetIdString, int lineNumber)
    {
        for (BreakpointMap::iterator it = m_pBreakpointSavedState->m_Breakpoints.begin(); it != m_pBreakpointSavedState->m_Breakpoints.end(); ++it)
        {
            const Breakpoint& bp = it->second;

            if (bp.m_assetName == assetIdString)
            {
                if (bp.m_documentLine == lineNumber - 1) // -1 offset is counter to the +1 line numbering scheme used by the LUA editor
                {
                    DeleteBreakpoint(bp.m_breakpointId);
                    EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
                    break;
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // AzToolsFramework CoreMessages and support
    void Context::OnRestoreState()
    {
        const AZStd::string k_launchString = "launch";
        const AZStd::string k_luaEditorString = "lua";
        const AZStd::string k_luaScriptFileString = "files";

        // the world editor considers itself a default window, so it always makes one

        typedef AzToolsFramework::FrameworkMessages::Bus HotkeyBus;
        // register our hotkeys so that they exist in the preferences panel even if we're not open:

        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAFind", 0xc62d8078),                    "Ctrl+F",           "Find",                                 "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAQuickFindLocal", 0x115cbcda),          "Ctrl+F3",          "Quick Find Local",                     "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAQuickFindLocalReverse", 0xdd8a0c22),   "Ctrl+Shift+F3",    "Quick Find Local (Reverse)",           "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAFindInFiles", 0xdaebdfdd),             "Ctrl+Shift+F",     "Find In Files",                        "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAReplace", 0x1fd5510c),                 "Ctrl+R",           "Replace",                              "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAReplaceInFiles", 0x38b609e0),          "Ctrl+Shift+R",     "Replace In Files",                     "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAGoToLine", 0xb6603f27),                "Ctrl+G",           "Go to line number...",                 "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAFold", 0xf0969e48),                    "Alt+0",            "Fold Source Functions",                "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAUnfold", 0x36934ecd),                  "Alt+Shift+0",      "Unfold Source Functions",              "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUACloseAllExceptCurrent", 0x0076409a),   "Ctrl+Alt+F4",      "Close All Windows Except Current",     "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUACloseAll", 0xf732678f),                "Ctrl+Shift+F4",    "Close All Windows",                    "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAComment", 0x873c2725),                 "Ctrl+K",           "Comment Selected Block",               "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAUncomment", 0x9190cf18),               "Ctrl+Shift+K",     "Uncomment Selected Block",             "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));

        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUALinesUpTranspose", 0xafc899ef),        "Ctrl+Shift+Up",    "Transpose Lines Up",                   "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));
        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUALinesDnTranspose", 0xf9d733bf),        "Ctrl+Shift+Down",  "Transpose Lines Down",                 "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));

        EBUS_EVENT(HotkeyBus, RegisterHotkey, AzToolsFramework::HotkeyDescription(AZ_CRC("LUAResetZoom", 0xbe0787ad),               "Ctrl+0",           "Reset Default Zoom",                   "LUA Editor", 1, AzToolsFramework::HotkeyDescription::SCOPE_WINDOW));

        bool GUIMode = true;
        EBUS_EVENT_RESULT(GUIMode, LegacyFramework::FrameworkApplicationMessages::Bus, IsRunningInGUIMode);
        if (!GUIMode)
        {
            // do not auto create lua editor main window in batch mode.
            return;
        }

        const AZ::CommandLine* commandLine = nullptr;

        AZ::ComponentApplicationBus::Broadcast([&commandLine](AZ::ComponentApplicationRequests* requests)
            {
                commandLine = requests->GetAzCommandLine();
            });

        bool forceShow = false;
        bool forceHide = false;

        if (commandLine->HasSwitch(k_launchString))
        {
            forceHide = true;

            size_t numSwitchValues = commandLine->GetNumSwitchValues(k_launchString);

            for (size_t i = 0; i < numSwitchValues; ++i)
            {
                AZStd::string inputValue = commandLine->GetSwitchValue(k_launchString, i);

                if (inputValue.compare(k_luaEditorString) == 0)
                {
                    forceShow = true;
                    forceHide = false;
                }
            }
        }

        size_t numSwitchValues = commandLine->GetNumSwitchValues(k_luaScriptFileString);
        if (numSwitchValues >= 1)
        {
            m_filesToOpen.clear();
            for (size_t i = 0; i < numSwitchValues; ++i)
            {
                AZStd::string inputValue = commandLine->GetSwitchValue(k_luaScriptFileString, i);

                // Open the file(s)
                AZStd::to_lower(const_cast<AZStd::string&>(inputValue).begin(), const_cast<AZStd::string&>(inputValue).end());

                // Cache the files we want to open, we will open them when we activate the main window.
                m_filesToOpen.push_back(inputValue);
            }
        }

        ProvisionalShowAndFocus(forceShow, forceHide);
    }

    bool Context::OnGetPermissionToShutDown() // until everyone returns true, we can't shut down.
    {
        AZ_TracePrintf(LUAEditorDebugName, "Context::OnGetPermissionToShutDown()\n");

        if (m_pLUAEditorMainWindow)
        {
            if (!m_pLUAEditorMainWindow->OnGetPermissionToShutDown())
            {
                return false;
        }
        }

        m_bShuttingDown = true;
        return true;
    }

    // until everyone returns true, we can't shut down.
    bool Context::CheckOkayToShutDown()
    {
        if (m_pLUAEditorMainWindow)
        {
            // confirmation that we're quitting.
            if (m_pLUAEditorMainWindow->isVisible())
            {
                m_pLUAEditorMainWindow->setEnabled(false);
                m_pLUAEditorMainWindow->hide();
            }
        }
        if (m_numOutstandingOperations > 0)
        {
            AZ_TracePrintf(LUAEditorDebugName, "CheckOkayToShutDown() return FALSE with (%d) OutstandingOperations\n", static_cast<int>(m_numOutstandingOperations));
            return false;
        }

        return true;
    }

    void Context::OnSaveState()
    {
        // notify main view to persist?
        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->SaveWindowState();
        }
    }

    // Script interface:
    void Context::LoadLayout()
    {
        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->RestoreWindowState();
    }
    }

    void Context::SaveLayout()
    {
        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->SaveWindowState();
    }
    }

    void Context::OnDestroyState()
    {
        m_documentInfoMap.clear();
        if (m_pLUAEditorMainWindow)
        {
            delete m_pLUAEditorMainWindow;
        }
        m_pLUAEditorMainWindow = nullptr;
    }
    //////////////////////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////////////////////
    //ContextInterface

    //////////////////////////////////////////////////////////////////////////
    // Utility
    void Context::ProvisionalShowAndFocus(bool forcedShow, bool forcedHide)
    {
        // main view will auto persist (load)
        auto newState = AZ::UserSettings::CreateFind<LUAEditorContextSavedState>(AZ_CRC("LUA EDITOR CONTEXT STATE", 0xc20c7427), AZ::UserSettings::CT_LOCAL);

        if (forcedShow)
        {
            newState->m_MainEditorWindowIsOpen = true;
            newState->m_MainEditorWindowIsVisible = true;
        }
        else if (forcedHide)
        {
            newState->m_MainEditorWindowIsOpen = false;
            newState->m_MainEditorWindowIsVisible = false;
        }

        if (newState->m_MainEditorWindowIsOpen)
        {
            if (newState->m_MainEditorWindowIsVisible)
            {
                if (!m_pLUAEditorMainWindow)
                {
                    m_pLUAEditorMainWindow = aznew LUAEditorMainWindow(m_referenceModel, m_connectedState);
                }

                m_pLUAEditorMainWindow->show();
                m_pLUAEditorMainWindow->raise();
                m_pLUAEditorMainWindow->activateWindow();
                m_pLUAEditorMainWindow->setFocus();
            }
            else
            {
                if (m_pLUAEditorMainWindow)
                {
                    m_pLUAEditorMainWindow->hide();
                }
            }

            EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    // Context_DocumentManagement Messages
    void Context::OnNewDocument(const AZStd::string& assetId)
    {
        ShowLUAEditorView();

        AZStd::string normalizedAssetId = assetId;
        AZStd::to_lower(normalizedAssetId.begin(), normalizedAssetId.end());

        // make sure we have a name that is not already tracked
        DocumentInfoMap::pair_iter_bool infoEntry = m_documentInfoMap.insert_key(normalizedAssetId);
        DocumentInfo& info = infoEntry.first->second;
        info.m_assetId = normalizedAssetId;
        info.m_assetName = assetId;
        AZ::StringFunc::Path::GetFullFileName(assetId.c_str(), info.m_displayName);
        info.m_bSourceControl_Ready = true;
        info.m_bSourceControl_CanWrite = true;
        info.m_bUntitledDocument = false;
        info.m_bIsBeingSaved = false;
        info.m_scriptAsset = "";

        // now open a view that will end up with its info (the view will have a progress bar on it as it loads)
        m_pLUAEditorMainWindow->OnOpenLUAView(info);

        // now since there is no actual loading we just tell its done, since the document is untitled it wont try
        // retrieve the document data in the call
        info.m_bDataIsLoaded = true;
        m_pLUAEditorMainWindow->OnDocumentInfoUpdated(info);
    }

    void Context::OnLoadDocument(const AZStd::string& assetId, bool errorOnNotFound)
    {
        AssetOpenRequested(assetId, errorOnNotFound);
    }

    void Context::OnCloseDocument(const AZStd::string& id)
    {
        AZStd::string assetId = id; // as we might delete the reference

        AZ_TracePrintf(LUAEditorDebugName, AZStd::string::format("LUAEditor OnCloseDocument" "%s\n", assetId.c_str()).c_str());

        DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(assetId);
        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "LUAEditor OnCloseDocument() : Cant find Document Info.");

        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->OnCloseView(assetId);
        }

        m_documentInfoMap.erase(docInfoIter);

        docInfoIter = m_documentInfoMap.find(assetId);
        if (docInfoIter != m_documentInfoMap.end())
        {
            //AZ_Assert(docInfoIter != m_documentInfoMap.end(), "LUAEditor OnCloseDocument() : Cant find Document Data.");
            m_documentInfoMap.erase(docInfoIter);
        }

        CleanUpBreakpoints();
    }

    void Context::OnSaveDocument(const AZStd::string& assetId, bool bCloseAfterSave, bool bSaveAs)
    {
        AZ_TracePrintf(LUAEditorDebugName, AZStd::string::format("LUAEditor OnSaveDocument" "%s\n", assetId.c_str()).c_str());

        if (!m_pLUAEditorMainWindow)
        {
            return;
        }

        // Make a copy because it may be modified behind our backs by later bus calls
        AZStd::string originalAssetId = assetId;

        auto docInfoIter = m_documentInfoMap.find(originalAssetId);
        if (docInfoIter == m_documentInfoMap.end())
        {
            AZ_TracePrintf(LUAEditorDebugName, "Context::OnSaveDocument - Document with ID is already closed - ignoring.\n");
            return;
        }

        AZStd::string newAssetName(docInfoIter->second.m_assetName);

        bool newFileCreated = false;

        if (docInfoIter->second.m_bIsBeingSaved)
        {
            return;
        }

        bool trySaveAs = docInfoIter->second.m_bUntitledDocument || bSaveAs;

        while (trySaveAs)
        {
            if (!m_pLUAEditorMainWindow->OnFileSaveDialog(docInfoIter->second.m_assetName, newAssetName))
            {
                return;
            }

            // the file dialog lets us do silly things like choose the same name as the original
            // in which case we should treat it just like a regular save
            if (newAssetName == docInfoIter->second.m_assetName)
            {
                docInfoIter->second.m_bUntitledDocument = false;
                break;
            }

            // do not allow SaveAs onto an existing asset, even if it could be checked out and modified "safely."
            // end user must check out and modify contents directly if they want this

            if (AZ::StringFunc::Find(newAssetName.c_str(), ".lua") == AZStd::string::npos)
            {
                newAssetName += ".lua";
            }

            AZ::Data::AssetId catalogAssetId;
            EBUS_EVENT_RESULT(catalogAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, newAssetName.c_str(), AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid(), false);

            if (catalogAssetId.IsValid() || m_fileIO->Exists(newAssetName.c_str()))
            {
                QMessageBox::warning(m_pLUAEditorMainWindow, "Warning", "You Cannot SaveAs Over An Existing Asset\nPlease Check And Try A New Filename");
                continue;
            }

            trySaveAs = false;
            docInfoIter->second.m_bUntitledDocument = false;
            AZ::StringFunc::Path::GetFullFileName(newAssetName.c_str(), docInfoIter->second.m_displayName);

            // when you 'save as' you can write to it, even if it started out not that way.
            docInfoIter->second.m_bSourceControl_Ready = true;
            docInfoIter->second.m_bSourceControl_CanWrite = true;

            newFileCreated = true;
        }

        if (!docInfoIter->second.m_bSourceControl_CanWrite)
        {
            AZ_Warning("LUA Editor Error", false, "<div severity=\"warning\">Unable to save document - the document is read-only.</div>");
        }

        docInfoIter->second.m_bDataIsWritten = false;
        docInfoIter->second.m_bCloseAfterSave = bCloseAfterSave;
        docInfoIter->second.m_bIsBeingSaved = true;

        //AZ_TracePrintf("Debug","    ++m_numOutstandingOperations %d\n",__LINE__);
        ++m_numOutstandingOperations;

        //////////////////////////////////////////////////////////////////////////
        // Blocking test for now (use the streamer later)

        // insert with the proper ID (saved file)
        bool isSaved = false;
        AZ::IO::SystemFile luaFile;
        if (luaFile.Open(newAssetName.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            luaFile.Write(docInfoIter->second.m_scriptAsset.c_str(), docInfoIter->second.m_scriptAsset.size());
            isSaved = true;
            luaFile.Close();
        }

        // Open the newly saved file
        if (isSaved)
        {
            if (newFileCreated)
            {
                docInfoIter->second.m_bCloseAfterSave = true;

                AZStd::string normalizedAssetId = newAssetName;
                AZStd::to_lower(normalizedAssetId.begin(), normalizedAssetId.end());

                EBUS_EVENT(Context_DocumentManagement::Bus, OnLoadDocument, normalizedAssetId, true);
                DocumentCheckOutRequested(normalizedAssetId);
            }
        }

        DataSaveDoneCallback(isSaved, originalAssetId);
        //////////////////////////////////////////////////////////////////////////

        m_pLUAEditorMainWindow->OnDocumentInfoUpdated(docInfoIter->second);
    }

    bool Context::OnSaveDocumentAs(const AZStd::string& assetId, bool bCloseAfterSave)
    {
        AZ_TracePrintf(LUAEditorDebugName, AZStd::string::format("LUAEditor OnSaveDocumentAs" "%s\n", assetId.c_str()).c_str());

        [[maybe_unused]] DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(assetId);
        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "LUAEditor OnSaveDocumentAs() : Cant find Document Info.");

        OnSaveDocument(assetId, bCloseAfterSave, true);

        return true;
    }

    void Context::DocumentCheckOutRequested(const AZStd::string& assetId)
    {
        DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(assetId);
        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "Invalid document lookup.");

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        AZ_Assert(fileIO, "FileIO system is not present.");

        if (fileIO && !fileIO->Exists(assetId.c_str()))
        {
            AZ_Warning(LUAEditorInfoName, false, "%s : Unable to check out file from source control, it may need to be saved first.", assetId.c_str());
            return;
        }

        DocumentInfo& docInfo = docInfoIter->second;
        AZ_TracePrintf(LUAEditorDebugName, "LUAEditor DocumentCheckOutRequested: %s\n", docInfo.m_assetName.c_str());
        docInfo.m_bSourceControl_BusyRequestingEdit = true;

        AZStd::string sourceFile;
        bool fileFound = false;
        using AssetSysReqBus = AzToolsFramework::AssetSystemRequestBus;
        AssetSysReqBus::BroadcastResult(fileFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath, assetId.c_str(), sourceFile);

        if (!fileFound)
        {
            // This warning can be tripped if the LuaIDE loses connection with the asset processor.
            AZ_Warning(LUAEditorInfoName, false, "The Lua IDE source control integration requires an active connection to the Asset Processor. Make sure Asset Processor is running.");

            // Reset BusyRequestingEdit or we'll be stuck with the "checking out" loading bar for forever
            docInfo.m_bSourceControl_BusyRequestingEdit = false;
            return;
        }

        ////AZ_TracePrintf("Debug","    ++m_numOutstandingOperations %d\n",__LINE__);
        ++m_numOutstandingOperations;
        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        SCCommandBus::Broadcast(&SCCommandBus::Events::RequestEdit,
            sourceFile.c_str(),
            true,
            AZStd::bind(&Context::PerforceRequestEditCallback,
                        this,
                        AZStd::placeholders::_1,
                        AZStd::placeholders::_2,
                        assetId)
            );
    }

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //AssetManagementMessages
    //Our callback that tell us when the asset open request finishes
    void Context::DataLoadDoneCallback(bool success, const AZStd::string& assetId)
    {
        //AZ_TracePrintf("Debug","        --m_numOutstandingOperations %d\n",__LINE__);
        --m_numOutstandingOperations;
        {
            AZ_TracePrintf("Debug", "DataLoadDoneCallback() ENTRY: loaded data for assetId %s\n", assetId.c_str());
        }

        // update data?
        if (success)
        {
            DocumentInfoMap::iterator actualDocument = m_documentInfoMap.find(assetId);
            AZ_Assert(actualDocument != m_documentInfoMap.end(), "Invalid document lookup.");
            {
                AZ_TracePrintf(LUAEditorDebugName, "DataLoadDoneCallback() sending OnDocumentInfoUpdated data for assetId '%s' '%s'\n", assetId.c_str(), actualDocument->second.m_assetName.c_str());
            }

            actualDocument->second.m_bDataIsLoaded = true;
            actualDocument->second.m_bIsModified = false;

            if (m_pLUAEditorMainWindow)
            {
                m_pLUAEditorMainWindow->OnDocumentInfoUpdated(actualDocument->second);
            }
        }

        AZ_TracePrintf(LUAEditorDebugName, "DataLoadDoneCallback() EXIT\n");
    }

    void Context::DataSaveDoneCallback(bool success, const AZStd::string& assetId)
    {
        //AZ_TracePrintf("Debug","        --m_numOutstandingOperations %d\n",__LINE__);
        --m_numOutstandingOperations;

        {
            // forensic logging
            AZ_TracePrintf(LUAEditorDebugName, "DataSaveDoneCallback() ENTRY: data save returned for assetId %s (%s)\n", assetId.c_str(), success ? "TRUE" : "FALSE");
        }

        DocumentInfoMap::iterator actualDocument = m_documentInfoMap.find(assetId);
        AZ_Assert(actualDocument != m_documentInfoMap.end(), "Invalid assetId lookup.");
        if (actualDocument == m_documentInfoMap.end())
        {
            return;
        }

        actualDocument->second.m_bIsBeingSaved = false; // we are no longer saving - regardless of whether we succeeded or not!

        // update data:
        if (success)
        {
            actualDocument->second.m_bDataIsWritten = true;

            //update the mod time in the document info
            if (m_fileIO)
            {
                uint64_t modTime = m_fileIO->ModificationTime(assetId.c_str());

                actualDocument->second.m_lastKnownModTime.dwHighDateTime = static_cast<DWORD>(modTime >> 32);
                actualDocument->second.m_lastKnownModTime.dwLowDateTime = static_cast<DWORD>(modTime);

                actualDocument->second.m_bDataIsLoaded = true;
                actualDocument->second.m_bIsModified = false;
            }

            // refresh source info:
            if (m_pLUAEditorMainWindow)
            {
                m_pLUAEditorMainWindow->OnDocumentInfoUpdated(actualDocument->second);
            }

            if (actualDocument->second.m_bCloseAfterSave)
            {
                EBUS_EVENT(Context_DocumentManagement::Bus, OnCloseDocument, assetId);
            }
        }
    }

    void Context::NotifyDocumentModified(const AZStd::string& assetId, bool modified)
    {
        // the document was modified, note this down
        DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(assetId);
        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "Invalid document lookup.");
        docInfoIter->second.m_bIsModified = modified;
    }

    void Context::UpdateDocumentData(const AZStd::string& assetId, const char* dataPtr, const AZStd::size_t dataLength)
    {
        DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(assetId);
        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "Invalid document lookup.");

        DocumentInfo& docInfo = docInfoIter->second;
        AZ_Assert(docInfo.m_bDataIsLoaded, "You may not retrieve data until it is loaded.");

        //docInfo.m_scriptAsset.GetAssetData()->SetData(dataPtr, dataLength);
        docInfo.m_scriptAsset = AZStd::string(dataPtr, dataLength);
    }

    void Context::GetDocumentData(const AZStd::string& assetId, const char** dataPtr, AZStd::size_t& dataLength)
    {
        DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(assetId);
        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "Invalid document lookup.");

        DocumentInfo& docInfo = docInfoIter->second;
        AZ_Assert(docInfo.m_bDataIsLoaded, "You may not retrieve data until it is loaded.");

        *dataPtr = docInfo.m_scriptAsset.data();
        dataLength = docInfo.m_scriptAsset.size();
    }

    void Context::PerforceStatResponseCallback(bool success, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZStd::string& assetId)
    {
        {
            AZ_TracePrintf("Debug", "PerforceStatResponseCallback() ENTRY: loaded assetId %s\n", assetId.c_str());
        }

        //AZ_TracePrintf("Debug","        --m_numOutstandingOperations %d\n",__LINE__);
        --m_numOutstandingOperations;
        // you got a callback from the perforce API, this is guaranteed to be on the main thread.
        DocumentInfoMap::iterator actualDocument = m_documentInfoMap.find(assetId);

        // the document may have already been closed.  this is fine.
        if (actualDocument == m_documentInfoMap.end())
        {
            {
                AZ_TracePrintf("Debug", "PerforceStatResponseCallback() EXIT: no such assetId %s\n", assetId.c_str());
            }
            return;
        }

        DocumentInfo& doc = actualDocument->second;

        //only means stats has been retrieved at least once
        doc.m_bSourceControl_Ready = true;

        //this operation is now considered done
        doc.m_bSourceControl_BusyGettingStats = false;

        //check file info flags to see if we can write
        doc.m_bSourceControl_CanWrite = (fileInfo.m_flags & AzToolsFramework::SCF_Writeable) != 0;

        doc.m_sourceControlInfo = fileInfo;

        //if we can check out is slightly a little more complicated
            //if the stat operation failed then we cant check out
        //if the stat operation succeeded then we need to make sure that it is currently checked in and its not out of date
        if (success == false)
        {
            doc.m_bSourceControl_CanCheckOut = false;
        }
        else
        {
            doc.m_bSourceControl_CanCheckOut = (fileInfo.IsManaged() && !(fileInfo.m_flags & AzToolsFramework::SCF_OutOfDate));
            doc.m_bSourceControl_CanCheckOut = fileInfo.m_flags & AzToolsFramework::SCF_MultiCheckOut || doc.m_bSourceControl_CanCheckOut;
        }

        AZ_TracePrintf(LUAEditorDebugName, "PerforceStatResponseCallback() sending OnDocumentInfoUpdated\n");

        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->OnDocumentInfoUpdated(doc);
        }

        {
            AZ_TracePrintf("Debug", "PerforceStatResponseCallback() EXIT: OK %s\n", assetId.c_str());
        }
    }

    void Context::PerforceRequestEditCallback(bool success, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZStd::string& assetId)
    {
        //AZ_TracePrintf("Debug","        --m_numOutstandingOperations %d\n",__LINE__);
        --m_numOutstandingOperations;
        // you got a callback from the perforce API, this is guaranteed to be on the main thread.
        DocumentInfoMap::iterator actualDocument = m_documentInfoMap.find(assetId);

        if (actualDocument == m_documentInfoMap.end())
        {
            return;
        }
        
        DocumentInfo& doc = actualDocument->second;

        //this operation is considered done
        doc.m_bSourceControl_BusyRequestingEdit = false;

        //check file info flags to see if we can write
        doc.m_bSourceControl_CanWrite = !fileInfo.IsReadOnly();

        doc.m_sourceControlInfo = fileInfo;

        //if we can check out is slightly a little more complicated
        //if the stat operation failed then we cant check out
        //if the stat operation succeeded then we need to make sure that it is currently checked in and its not out of date
        if (success == false)
        {
            doc.m_bSourceControl_CanCheckOut = false;
        }
        else
        {
            doc.m_bSourceControl_CanCheckOut = fileInfo.IsManaged() && !fileInfo.HasFlag(AzToolsFramework::SCF_OutOfDate);

            doc.m_bSourceControl_CanCheckOut = fileInfo.HasFlag(AzToolsFramework::SCF_MultiCheckOut) || doc.m_bSourceControl_CanCheckOut;
        }

        if (!doc.m_bSourceControl_Ready)
        {
            QMessageBox::warning(m_pLUAEditorMainWindow, "Warning", "Perforce shows that it's not ready.");
        }
        if (!doc.m_bSourceControl_CanWrite)
        {
            if (!doc.m_sourceControlInfo.HasFlag(AzToolsFramework::SCF_OpenByUser))
            {
                QMessageBox::warning(m_pLUAEditorMainWindow, "Warning", "This file is ReadOnly you cannot write to this file.");
            }
        }
        else if (!doc.m_bSourceControl_CanCheckOut)
        {
            if (doc.m_sourceControlInfo.m_status == AzToolsFramework::SCS_ProviderIsDown)
            {
                QMessageBox::warning(m_pLUAEditorMainWindow, "Warning", "Perforce Is Down.\nFile will be saved.\nYou must reconcile with Perforce later!");
            }
            else if (doc.m_sourceControlInfo.m_status == AzToolsFramework::SCS_ProviderError)
            {
                QMessageBox::warning(m_pLUAEditorMainWindow, "Warning", "Perforce encountered an error.\nFile will be saved.\nYou must reconcile with Perforce later!");
            }
            else if (doc.m_sourceControlInfo.m_status == AzToolsFramework::SCS_CertificateInvalid)
            {
                QMessageBox::warning(m_pLUAEditorMainWindow, "Warning", "Perforce Connection is not trusted.\nFile will be saved.\nYou must reconcile with Perforce later!");
            }
            else if (!doc.m_sourceControlInfo.HasFlag(AzToolsFramework::SCF_OpenByUser))
            {
                QMessageBox::warning(m_pLUAEditorMainWindow, "Warning", "Perforce says that you cannot write to this file.");
            }
        }

        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->OnDocumentInfoUpdated(doc);
        }
    }


    void Context::OnReloadDocument(const AZStd::string assetId)
    {
        DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(assetId);
        {
            AZ_TracePrintf(LUAEditorDebugName, "OnReloadDocument() ENTRY user queing reload for assetId '%s'\n", assetId.c_str());
        }

        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "Invalid document lookup.");
        docInfoIter->second.m_bDataIsLoaded = false;
        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->OnDocumentInfoUpdated(docInfoIter->second);
        }

        AZ_TracePrintf(LUAEditorDebugName, "OnReloadDocument() Beginning asset load.\n");
        docInfoIter->second.m_assetId = assetId;

        // while we're reading it, fetch the perforce information for it:
        // AZ_TracePrintf("Debug","    ++m_numOutstandingOperations %d\n",__LINE__);
        ++m_numOutstandingOperations;
        using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
        SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo,
            assetId.c_str(),
            AZStd::bind(&Context::PerforceStatResponseCallback,
            this,
            AZStd::placeholders::_1,
            AZStd::placeholders::_2,
            assetId)
            );

        AZ_TracePrintf(LUAEditorDebugName, "OnReloadDocument() Queuing bind bus relay.\n");
        ++m_numOutstandingOperations;

        // Load the document
        bool isLoaded = false;
        AZ::IO::SystemFile luaFile;
        if (luaFile.Open(assetId.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            docInfoIter->second.m_scriptAsset.resize(luaFile.Length());
            luaFile.Read(docInfoIter->second.m_scriptAsset.size(), docInfoIter->second.m_scriptAsset.data());
            isLoaded = true;
            luaFile.Close();
        }

        Context::DataLoadDoneCallback(isLoaded, docInfoIter->second.m_assetId);
    }


    void Context::OpenMostRecentDocumentView()
    {
        m_queuedOpenRecent = false;

        if (mostRecentlyOpenedDocumentView.empty())
        {
            return;
        }

        if (m_pLUAEditorMainWindow)
        {
            m_bProcessingActivate = true;
            m_pLUAEditorMainWindow->IgnoreFocusEvents(false);
            m_pLUAEditorMainWindow->setAnimated(false);
            ProvisionalShowAndFocus();
            m_pLUAEditorMainWindow->OnRequestFocusView(mostRecentlyOpenedDocumentView);
            m_pLUAEditorMainWindow->setAnimated(true);
            m_bProcessingActivate = false;
        }
    }

    void Context::OpenAssetByPhysicalPath(const char* physicalPath)
    {
        //error check input
        if (!physicalPath)
        {
            AZ_Warning("LUAEditor::Context", false, AZStd::string::format("<span severity=\"err\">Path is null: '%s'</span>", physicalPath).c_str());
            return;
        }

        if (!strlen(physicalPath))
        {
            AZ_Warning("LUAEditor::Context", false, AZStd::string::format("<span severity=\"err\">Path is empty: '%s'</span>", physicalPath).c_str());
            return;
        }

        if (!m_fileIO->Exists(physicalPath))
        {
            AZ_Warning(LUAEditorInfoName, false, AZStd::string::format("<span severity=\"err\">Could not open the file, file not found: '%s'</span>", physicalPath).c_str());

            QMessageBox msgBox(m_pLUAEditorMainWindow);
            msgBox.setModal(true);
            msgBox.setText("File not found");
            msgBox.setInformativeText(physicalPath);
            msgBox.setStandardButtons(QMessageBox::Ok);
            msgBox.setDefaultButton(QMessageBox::Ok);
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.exec();

            CleanUpBreakpoints();
            return;
        }

        AssetOpenRequested(physicalPath, true);
    }

    void Context::AssetOpenRequested(const AZStd::string& assetId, bool errorOnNotFound)
    {
        if (!m_fileIO)
        {
            return;
        }

        AZStd::string assetIdLower(assetId);
        AZStd::to_lower(assetIdLower.begin(), assetIdLower.end());

        ShowLUAEditorView();

        // asset browser requests opening of a particular assets.
        // we need to do a whole bunch of things
        // * we need to start tracking and validate the document that is about to be opened.  It might already be open, for example.
        //  - documents may belong to another Context (?) like for example if entities have embedded blobs of lua.  In that case, the
        //    interface may be different in that the asset is treated as a different type and the other Context manages the docs.
        // * we need to create a new LUA panel for it
        // * we need to load that lua panel with the document's data, initializing it.

        // are we already tracking it?
        auto it = m_documentInfoMap.find(assetIdLower);
        if (it != m_documentInfoMap.end())
        {
            // tell the view that it needs to focus that document!
            mostRecentlyOpenedDocumentView = assetIdLower;
            if (m_queuedOpenRecent)
            {
                return;
            }

            EBUS_QUEUE_FUNCTION(AZ::SystemTickBus, &Context::OpenMostRecentDocumentView, this);
            return;
        }

        if (!m_fileIO->Exists(assetId.c_str()))
        {
            if (errorOnNotFound)
            {
                AZ_Warning(LUAEditorInfoName, false, AZStd::string::format("<span severity=\"err\">Could not open the file, file not found: '%s'</span>", assetId.c_str()).c_str());
                QMessageBox msgBox(m_pLUAEditorMainWindow);
                msgBox.setModal(true);
                msgBox.setText("File not found");
                msgBox.setStandardButtons(QMessageBox::Ok);
                msgBox.setDefaultButton(QMessageBox::Ok);
                msgBox.setIcon(QMessageBox::Critical);
                msgBox.exec();
            }

            CleanUpBreakpoints();

            return;
        }

        // Register the script into the asset catalog
        AZ::Data::AssetType assetType = AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid();
        AZ::Data::AssetId catalogAssetId;
        EBUS_EVENT_RESULT(catalogAssetId, AZ::Data::AssetCatalogRequestBus, GetAssetIdByPath, assetIdLower.c_str(), assetType, true);

        uint64_t modTime = m_fileIO->ModificationTime(assetId.c_str());

        DocumentInfo info;
        info.m_assetName = assetIdLower;
        AZ::StringFunc::Path::GetFullFileName(assetId.c_str(), info.m_displayName);
        info.m_assetId = assetIdLower;
        info.m_bSourceControl_BusyGettingStats = true;
        info.m_bSourceControl_BusyGettingStats = false;
        info.m_bSourceControl_CanWrite = true;
        info.m_lastKnownModTime.dwHighDateTime = static_cast<DWORD>(modTime >> 32);
        info.m_lastKnownModTime.dwLowDateTime = static_cast<DWORD>(modTime);
        info.m_bIsModified = false;

        // load the script source
        auto infoIter = m_documentInfoMap.insert(AZStd::make_pair(info.m_assetId, info)).first;

        // now open a view that will end up with its info (the view will have a progress bar on it as it loads)
        m_pLUAEditorMainWindow->OnOpenLUAView(info);
        {
            // while we're reading it, fetch the perforce information for it:
            //AZ_TracePrintf("Debug","    ++m_numOutstandingOperations %d\n",__LINE__);
            ++m_numOutstandingOperations;
            using SCCommandBus = AzToolsFramework::SourceControlCommandBus;
            SCCommandBus::Broadcast(&SCCommandBus::Events::GetFileInfo,
                assetId.c_str(),
                AZStd::bind(&Context::PerforceStatResponseCallback,
                this,
                AZStd::placeholders::_1,
                AZStd::placeholders::_2,
                assetId)
                );
        }

        //AZ_TracePrintf("Debug","    ++m_numOutstandingOperations %d\n",__LINE__);
        ++m_numOutstandingOperations;

        //////////////////////////////////////////////////////////////////////////
        // Load in places
        bool isLoaded = false;
        AZ::IO::SystemFile luaFile;
        if (luaFile.Open(assetId.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
        {
            infoIter->second.m_scriptAsset.resize(luaFile.Length());
            luaFile.Read(infoIter->second.m_scriptAsset.size(), infoIter->second.m_scriptAsset.data());
            isLoaded = true;
            luaFile.Close();
        }

        DataLoadDoneCallback(isLoaded, assetIdLower);
        //////////////////////////////////////////////////////////////////////////

        if (m_queuedOpenRecent)
        {
                return;
        }

        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->IgnoreFocusEvents(false);
        }

        mostRecentlyOpenedDocumentView = assetIdLower;
        EBUS_QUEUE_FUNCTION(AZ::SystemTickBus, &Context::OpenMostRecentDocumentView, this);
    }

    void Context::RunAsAnotherInstance()
    {
        const AZStd::string k_luaScriptFileString = "files";

        const AzFramework::CommandLine* commandLine = nullptr;
        AZ::ComponentApplicationBus::Broadcast([&commandLine](AZ::ComponentApplicationRequests* requests)
            {
                commandLine = requests->GetAzCommandLine();
            });

        AZStd::string parameters = "";
        size_t numSwitchValues = commandLine->GetNumSwitchValues(k_luaScriptFileString);
        if (numSwitchValues >= 1)
        {
            for (size_t i = 0; i < numSwitchValues; ++i)
            {
                AZStd::string inputValue = commandLine->GetSwitchValue(k_luaScriptFileString, i);
                AZStd::to_lower(const_cast<AZStd::string&>(inputValue).begin(), const_cast<AZStd::string&>(inputValue).end());

                // Cache the files we want to open, we will open them when we activate the main window.
                parameters.append(inputValue);
                parameters.append(";");
            }
        }

        // Send the list of files to open to the running instance.
        EBUS_EVENT(LegacyFramework::IPCCommandBus, SendIPCCommand, "open_files", parameters.c_str());
    }

    void Context::ShowLUAEditorView()
    {
        ProvisionalShowAndFocus(true);
    }

    //////////////////////////////////////////////////////////////////////////
    // Context_DebuggerManagement Messages
    //////////////////////////////////////////////////////////////////////////

    // ExecuteScriptBlob - execute a script blob.
    void Context::ExecuteScriptBlob(const AZStd::string& fromAssetId, bool executeLocally)
    {
        DocumentInfoMap::iterator docInfoIter = m_documentInfoMap.find(fromAssetId);
        AZ_Assert(docInfoIter != m_documentInfoMap.end(), "Could not find data");

        if (docInfoIter->second.m_scriptAsset.empty())
        {
            AZ_Warning(LUAEditorDebugName, false, "Could not execute empty script document.");
            return;
        }

        const char* scriptData = docInfoIter->second.m_scriptAsset.c_str();

        EBUS_EVENT(LUAEditor::LUAStackTrackerMessages::Bus, StackClear);

        DocumentInfoMap::iterator actualDocument = m_documentInfoMap.find(fromAssetId);
        AZ_Assert(actualDocument != m_documentInfoMap.end(), "Invalid document lookup.");

        SynchronizeBreakpoints();

        // the debug name is simply the name of the document.
        // if its unnamed, it's synthesized

        AZStd::string debugName = actualDocument->second.m_assetName;

        // if we're executing it locally, we'll just execute it locally - do not involve the debugger.

        if (executeLocally)
        {
            AZ::ScriptContext* sc = nullptr;
            EBUS_EVENT_RESULT(sc, AZ::ScriptSystemRequestBus, GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
            if (sc)
            {
                // we might want to bracket this with some sort of error or warning protection, to collect
                // the error / warning results in a neat way to show user.
                sc->Execute(scriptData, debugName.c_str());
            }
            return;
        }

        // otherwise we've been told to execute it on the debugger remotely which is presently unsupported
    }

    void Context::SynchronizeBreakpoints()
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::SynchronizeBreakpoints()\n");

        for (BreakpointMap::iterator it = m_pBreakpointSavedState->m_Breakpoints.begin(); it != m_pBreakpointSavedState->m_Breakpoints.end(); ++it)
        {
            Breakpoint& bp = it->second;
            EBUS_EVENT(LUAEditorDebuggerMessages::Bus, CreateBreakpoint, bp.m_assetName, bp.m_documentLine);
        }

        EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
    }

    void Context::CreateBreakpoint(const AZStd::string& fromAssetId, int lineNumber)
    {
        DocumentInfoMap::iterator actualDocument = m_documentInfoMap.find(fromAssetId);
        AZ_Assert(actualDocument != m_documentInfoMap.end(), "Invalid document lookup.");
        DocumentInfo& doc = actualDocument->second;

        AZ::Uuid breakpointUID = AZ::Uuid::CreateRandom();

        // when a breakpoint is created, we need to find out what the most recent blob is that was applied
        // to patch over that line number in that document, and apply it to that blob.

        // first, let's find if we've patched or run any blobs.  By default, the doc name will be the asset name.
        AZStd::string debugName = doc.m_assetName;

        AZ_TracePrintf(LUAEditorDebugName, "Context::CreateBreakpoint( %s )\n", debugName.c_str());

        BreakpointMap::pair_iter_bool newInsertion = m_pBreakpointSavedState->m_Breakpoints.insert(breakpointUID);
        AZ_Assert(newInsertion.second, "Breakpoint already exists!");
        Breakpoint& newBreakpoint = newInsertion.first->second;
        newBreakpoint.m_assetName = debugName;
        newBreakpoint.m_breakpointId = breakpointUID;
        newBreakpoint.m_assetId = fromAssetId;
        newBreakpoint.m_documentLine = lineNumber;

        // we now know the 'debug name' (a string) that was submitted to the piece of code that this breakpoint is for, and we know the line number
        // inside that blob that the breakpoint wants to be set on.

        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, CreateBreakpoint, debugName, lineNumber);

        //AZ_TracePrintf(LUAEditorDebugName, "Setting breakpoint in '%s' on line %i (In document %s line %i)\n", debugName.c_str(), lineNumber, doc.m_displayName.c_str(), lineNumber);

        EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
    }

    void Context::DeleteBreakpoint(const AZ::Uuid& breakpointUID)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::DeleteBreakpoint()\n");

        BreakpointMap::iterator finder = m_pBreakpointSavedState->m_Breakpoints.find(breakpointUID);

        //AZ_Assert(finder != m_breakpoints.end(), "Breakpoint referred to, but not found!");

        if (finder != m_pBreakpointSavedState->m_Breakpoints.end())
        {
            Breakpoint& bp = finder->second;

            AZ_TracePrintf(LUAEditorDebugName, "  -  Removed breakpoint in '%s' on line '%i'\n", bp.m_assetName.c_str(), bp.m_documentLine);

            EBUS_EVENT(LUAEditorDebuggerMessages::Bus, RemoveBreakpoint, bp.m_assetName, bp.m_documentLine);

            m_pBreakpointSavedState->m_Breakpoints.erase(finder);

            EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
        }
    }

    // Find any breakpoints that no longer have any attachment and remove them
    void Context::CleanUpBreakpoints()
    {
        // Build a list of orphaned breakpoints
        std::vector<AZ::Uuid> invalidBreakpoints;
        for (auto const &breakpoint : m_pBreakpointSavedState->m_Breakpoints)
        {
            if (!m_fileIO->Exists(breakpoint.second.m_assetName.c_str()))
            {
                invalidBreakpoints.emplace_back(breakpoint.second.m_breakpointId);
            }
        }

        for (auto const &id : invalidBreakpoints)
        {
            DeleteBreakpoint(id);
        }

        // submit the updated list
        EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
    }

    //TODO: add a valid-invalid state to local breakpoints
    // default new breaks to INVALID
    // use the response here to set matching breaks to VALID
    void Context::OnBreakpointAdded(const AZStd::string& /*assetIdString*/, int /*lineNumber*/)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Debug agent added breakpoint at %s, line %d.\n", assetIdString.c_str(), lineNumber + 1);
    }

    void Context::OnBreakpointRemoved(const AZStd::string& /*assetIdString*/, int /*lineNumber*/)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Debug agent removed breakpoint at %s, line %d.\n", assetIdString.c_str(), lineNumber + 1);
    }

    void Context::MoveBreakpoint(const AZ::Uuid& breakpointUID, int lineNumber)
    {
        // moving a breakpoint will cause it to update where it is in the document in question
        // however, we don't actually re-transmit the breakpoint over the wire, because we haven't re-run the script
        // this is just housekeeping so that when the network says a certain breakpoint came in at a certain place,
        // we know what they're talking about.

        if (lineNumber >= 0)
        {
            BreakpointMap::iterator finder = m_pBreakpointSavedState->m_Breakpoints.find(breakpointUID);
            if (finder != m_pBreakpointSavedState->m_Breakpoints.end())
            {
                Breakpoint& bp = finder->second;
                bp.m_documentLine = lineNumber;
                AZ_TracePrintf(LUAEditorDebugName, "Breakpoint '%s' updated to point at document line '%i'\n", bp.m_assetName.c_str(), bp.m_documentLine);
            }

            // send out the update
            EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointsUpdate, m_pBreakpointSavedState->m_Breakpoints);
        }
    }

    void Context::OnDebuggerAttached()
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnDebuggerAttached()\n");

        EBUS_EVENT(Context_ControlManagement::Bus, OnDebuggerAttached);
        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, EnumRegisteredClasses, m_currentTargetContext.c_str());
        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, EnumRegisteredEBuses, m_currentTargetContext.c_str());
        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, EnumRegisteredGlobals, m_currentTargetContext.c_str());
        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, OnConnectedToDebugger);
        EBUS_EVENT(LUAWatchesDebuggerMessages::Bus, OnDebuggerAttached);

        SynchronizeBreakpoints();
    }

    void Context::OnDebuggerRefused()
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnDebuggerRefused()\n");

        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, OnDisconnectedFromDebugger);
        EBUS_EVENT(Context_ControlManagement::Bus, OnDebuggerDetached);
        EBUS_EVENT(LUAWatchesDebuggerMessages::Bus, OnDebuggerDetached);
    }

    void Context::OnDebuggerDetached()
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnDebuggerDetached()\n");

        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, OnDisconnectedFromDebugger);
        EBUS_EVENT(Context_ControlManagement::Bus, OnDebuggerDetached);
        EBUS_EVENT(LUAWatchesDebuggerMessages::Bus, OnDebuggerDetached);
    }

    // this happens when a breakpoint is hit:
    void Context::OnBreakpointHit(const AZStd::string& relativePath, int lineNumber)
    {
        // Convert from debug path (relative) to absolute path (how the Lua IDE stores files)
        AZStd::string absolutePath = relativePath.substr(1);
        EBUS_EVENT(AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, absolutePath, absolutePath);

        //AZ_TracePrintf(LUAEditorDebugName, "Breakpoint '%s' was hit on line %i\n", assetIdString.c_str(), lineNumber);
        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, GetCallstack);
        EBUS_EVENT(LUAEditor::LUABreakpointRequestMessages::Bus, RequestEditorFocus, absolutePath, lineNumber);

        AZStd::string assetId = absolutePath;

        // let's see if we can find an open document
        DocumentInfoMap::iterator actualDocument = m_documentInfoMap.find(assetId.c_str());
        if (actualDocument == m_documentInfoMap.end())
        {
            // the document might have been closed
            AssetOpenRequested(assetId, true);

            // let's see if we can find an open document
            DocumentInfoMap::iterator actualDocumentIterator = m_documentInfoMap.find(assetId.c_str());
            if (actualDocumentIterator != m_documentInfoMap.end())
            {
                actualDocumentIterator->second.m_PresetLineAtOpen = lineNumber;
            }

            // early out after requesting a background data load
            return;
        }

        int actualDocumentLineNumber = lineNumber;

        // we now know what document that the breakpoint is talking about.
        // we could just assume that the document has not changed since we saw the breakpoint applied, but its possible that is has in fact shifted.
        // do we have any breakpoints established for that particular blob?
        {
            BreakpointMap::iterator it = m_pBreakpointSavedState->m_Breakpoints.begin();
            for (; it != m_pBreakpointSavedState->m_Breakpoints.end(); ++it)
            {
                const Breakpoint& bp = it->second;
                if (bp.m_assetId == assetId)
                {
                    if (bp.m_documentLine == lineNumber)
                    {
                        // this is that breakpoint!
                        actualDocumentLineNumber = bp.m_documentLine; // bump it if its shifted:
                        EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointHit, bp);
                        break;
                    }
                }
            }

            if (it == m_pBreakpointSavedState->m_Breakpoints.end())
            {
                // it's an imaginary break resulting from one of the STEP calls
                // dummy up a bp and send that as the message
                Breakpoint dbp;
                dbp.m_breakpointId = AZ::Uuid::CreateNull();
                dbp.m_assetId = "";
                dbp.m_documentLine = lineNumber;

                EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointHit, dbp);
            }
        }

        //AZ_TracePrintf(LUAEditorDebugName, "That translates to line number %i in document '%s'\n", actualDocumentLineNumber, doc.m_displayName.c_str());

        // what do we actually do?
        // we need to
        // 1. FOCUS that window
        // 2. INDICATE that we are 'stopped'
        // 3. Update any watched variables
        // 4. Show a program cursor on that line!
        // 5.  Enable the step over and so on, the debugging controls basically.

        // focus the window:
        ProvisionalShowAndFocus(true);

        // are we already tracking it?
        // tell the view that it needs to focus that document!
        m_pLUAEditorMainWindow->OnRequestFocusView(assetId);
        m_pLUAEditorMainWindow->MoveProgramCursor(assetId, actualDocumentLineNumber);
    }

    void Context::OnReceivedAvailableContexts(const AZStd::vector<AZStd::string>& contexts)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnReceivedAvailableContexts()\n");

        m_targetContexts = contexts;

        size_t i;
        for (i = 0; i < m_targetContexts.size(); ++i)
        {
            if (m_currentTargetContext == m_targetContexts[i])
            {
                break;
            }
        }
        if (i >= m_targetContexts.size())
        {
            m_targetContexts.clear();
            m_currentTargetContext = "Default";
        }

        EBUS_EVENT(LUAEditor::Context_ControlManagement::Bus, OnTargetContextPrepared, m_currentTargetContext);

        RequestAttachDebugger();
    }

    void Context::OnReceivedRegisteredClasses(const AzFramework::ScriptUserClassList& classes)
    {
        AZ_TracePrintf(LUAEditorDebugName, "Context::OnReceivedRegisteredClasses()\n");
        // Reset the class reference for the current target
        ContextReference& reference = m_reference[m_currentTargetContext];
        reference.m_classes = classes;
        UpdateReferenceWindow();
    }

    void Context::OnReceivedRegisteredEBuses(const AzFramework::ScriptUserEBusList& ebuses)
    {
        AZ_TracePrintf(LUAEditorDebugName, "Context::OnReceivedRegisteredEBuses()\n");

        ContextReference& reference = m_reference[m_currentTargetContext];
        reference.m_buses = ebuses;
        UpdateReferenceWindow();
    }

    void Context::OnReceivedRegisteredGlobals(const AzFramework::ScriptUserMethodList& methods, const AzFramework::ScriptUserPropertyList& properties)
    {
        AZ_TracePrintf(LUAEditorDebugName, "Context::OnReceivedRegisteredGlobals()\n");

        ContextReference& reference = m_reference[m_currentTargetContext];
        reference.m_globals.m_methods = methods;
        reference.m_globals.m_properties = properties;
        UpdateReferenceWindow();
    }

    AZStd::string GetTooltip(const AzFramework::ScriptUserPropertyInfo& propInfo)
    {
        static const char* lut[2][2] =
        {
            { "Locked", "WO" },
            { "RO", "R/W" }
        };
        AZStd::string rw = lut[propInfo.m_isRead][propInfo.m_isWrite];
        return propInfo.m_name + "[" + rw + "]";
    }

    AZStd::string GetTooltip(const AzFramework::ScriptUserMethodInfo& methodInfo)
    {
        return methodInfo.m_name + "(" + methodInfo.m_dbgParamInfo + ")";
    }

    AZStd::string GetTooltip(const AzFramework::ScriptUserClassInfo& classInfo)
    {
        return classInfo.m_name + "()";
    }

    AZStd::string GetTooltip(const AzFramework::ScriptUserEBusInfo& ebusInfo)
    {
        return ebusInfo.m_name;
    }

    void Context::UpdateReferenceWindow()
    {
        const ContextReference& reference = m_reference[m_currentTargetContext];

        m_LUAKeywords.clear();
        AddDefaultLUAKeywords();

        m_LUALibraryFunctions.clear();
        AddDefaultLUALibraryFunctions();

        m_referenceModel->clear();

        // Globals
        ReferenceItem* global = aznew ReferenceItem("Globals", AZ::u64(0));
        global->setToolTip("Globals");
        global->setWhatsThis("Global Methods and Variables");
        m_referenceModel->appendRow(global);

        for (const auto& methodInfo : reference.m_globals.m_methods)
        {
            ReferenceItem* method = aznew ReferenceItem(methodInfo.m_name.c_str(), AZ::u64(0));
            method->setToolTip(QString::fromUtf8(GetTooltip(methodInfo).c_str()));
            method->setWhatsThis(QString::fromUtf8(methodInfo.m_name.c_str()));

            global->appendRow(method);

            m_LUALibraryFunctions.insert(methodInfo.m_name);
        }
        for (const auto& propInfo : reference.m_globals.m_properties)
        {
            ReferenceItem* variable = aznew ReferenceItem(propInfo.m_name.c_str(), AZ::u64(0));
            variable->setToolTip(QString::fromUtf8(GetTooltip(propInfo).c_str()));
            variable->setWhatsThis(QString::fromUtf8(propInfo.m_name.c_str()));

            global->appendRow(variable);

            m_LUAKeywords.insert(propInfo.m_name);
        }

        // Classes
        ReferenceItem* classes = aznew ReferenceItem("Classes", AZ::u64(0));
        classes->setToolTip("Classes");
        classes->setWhatsThis("Classes");
        m_referenceModel->appendRow(classes);

        for (const auto& classInfo : reference.m_classes)
        {
            ReferenceItem* classItem = aznew ReferenceItem(QString::fromUtf8(classInfo.m_name.c_str()), 0);
            classItem->setToolTip(QString::fromUtf8(classInfo.m_name.c_str()));
            classItem->setWhatsThis(QString::fromUtf8(classInfo.m_name.c_str()));
            classes->appendRow(classItem);

            for (const auto& methodInfo : classInfo.m_methods)
            {
                ReferenceItem* methodItem = aznew ReferenceItem(QString::fromUtf8((methodInfo.m_name + "( " + methodInfo.m_dbgParamInfo + " )").c_str()), 0);
                methodItem->setToolTip(QString::fromUtf8(GetTooltip(methodInfo).c_str()));
                methodItem->setWhatsThis(QString::fromUtf8(methodInfo.m_name.c_str()));

                classItem->appendRow(methodItem);

                m_LUALibraryFunctions.insert(classInfo.m_name + "." + methodInfo.m_name);
            }
            for (const auto& propInfo : classInfo.m_properties)
            {
                ReferenceItem* propItem = aznew ReferenceItem(propInfo.m_name.c_str(), 0);
                propItem->setToolTip(QString::fromUtf8(GetTooltip(propInfo).c_str()));
                propItem->setWhatsThis(QString::fromUtf8(propInfo.m_name.c_str()));

                classItem->appendRow(propItem);

                m_LUALibraryFunctions.insert(classInfo.m_name + "." + propInfo.m_name);
            }
        }

        // Buses
        ReferenceItem* buses = aznew ReferenceItem("EBuses", AZ::u64(0));
        buses->setToolTip("EBuses");
        buses->setWhatsThis("EBuses");
        m_referenceModel->appendRow(buses);

        for (auto const &ebusInfo : reference.m_buses)
        {
            // Make a reference item from the info-block for displaying in the class hierarchy and add it to the reference table.
            ReferenceItem* ebus = aznew ReferenceItem(QString::fromUtf8(ebusInfo.m_name.c_str()), 0);
            ebus->setToolTip(QString::fromUtf8(GetTooltip(ebusInfo).c_str()));
            ebus->setWhatsThis(QString::fromUtf8(ebusInfo.m_name.c_str()));
            buses->appendRow(ebus);

            if (!ebusInfo.m_events.empty())
            {
                ReferenceItem* eventRoot = aznew ReferenceItem("Event", 0);
                ReferenceItem* broadcastRoot = (ebusInfo.m_canBroadcast) ? aznew ReferenceItem("Broadcast", 0) : nullptr;
                ReferenceItem* notificationsRoot = (ebusInfo.m_hasHandler) ? aznew ReferenceItem("Notifications", 0) : nullptr;

                for (auto const &eventInfo : ebusInfo.m_events)
                {
                    // Construct the visual element for displaying in the reference pane.
                    ReferenceItem* eventItem = aznew ReferenceItem(QString::fromUtf8(GetTooltip(eventInfo).c_str()), 0);
                    eventItem->setToolTip(QString::fromUtf8(GetTooltip(eventInfo).c_str()));
                    eventItem->setWhatsThis(QString::fromUtf8(eventInfo.m_name.c_str()));

                    if (eventInfo.m_category == "Event")
                    {
                        eventRoot->appendRow(eventItem);
                        m_LUALibraryFunctions.insert(ebusInfo.m_name + ".Event." + eventInfo.m_name);

                    }
                    else if (eventInfo.m_category == "Broadcast" && broadcastRoot)
                    {
                        broadcastRoot->appendRow(eventItem);
                        m_LUALibraryFunctions.insert(ebusInfo.m_name + ".Broadcast." + eventInfo.m_name);
                    }
                    else if (eventInfo.m_category == "Notification" && notificationsRoot)
                    {
                        notificationsRoot->appendRow(eventItem);
                    }
                    else
                    {
                        // This should not happen, but in the case that we somehow have a handler or broadcast
                        // and nowhere to attach it, let's at least not leak it
                        delete eventItem;
                    }
                }

                // Add the root nodes that have children to the bus tree, delete empty roots
                for (ReferenceItem* rootNode : { eventRoot, broadcastRoot, notificationsRoot })
                {
                    if (rootNode && rootNode->rowCount() > 0)
                    {
                        ebus->appendRow(rootNode);
                    }
                    else if (rootNode)
                    {
                        delete rootNode;
                    }
                }
            }
        }

        if (m_pLUAEditorMainWindow)
        {
            Q_EMIT m_pLUAEditorMainWindow->OnReferenceDataChanged();
        }

        EBUS_EVENT(HighlightedWordNotifications::Bus, LUALibraryFunctionsUpdated);
    }

    void Context::OnReceivedLocalVariables(const AZStd::vector<AZStd::string>& vars)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnReceivedLocalVariables()\n");

        EBUS_EVENT(LUALocalsTrackerMessages::Bus, LocalsUpdate, vars);

        for (size_t i = 0; i < vars.size(); ++i)
        {
            //AZ_TracePrintf(LUAEditorDebugName, "\t%s\n", vars[i].c_str());
            EBUS_EVENT(LUAEditorDebuggerMessages::Bus, GetValue, vars[i]);
        }
    }

    void Context::OnReceivedCallstack(const AZStd::vector<AZStd::string>& callstack)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnReceivedCallstack()\n");

        LUAEditor::StackList sl;

        for (size_t i = 0; i < callstack.size(); ++i)
        {
            //AZ_TracePrintf(LUAEditorDebugName, "stack[%d] %s\n", i, callstack[i].c_str());
            const char* stackLine = callstack[i].c_str();

            // strings starting with a pointer address aren't useful and break the format
            if (stackLine)
            {
                const size_t tempSize = 4096;

                if (!isdigit(stackLine[0]))
                {
                    if (stackLine[0] == '[')
                    {
                        // LUA format
                        LUAEditor::StackEntry s;

                        const char* fb = strchr(stackLine, '@');
                        if (fb)
                        {
                            ++fb;
                            const char* fe = strchr(fb, '(');
                            if (fe)
                            {
                                char temp[tempSize];
                                --fe;
                                memcpy(temp, fb, fe - fb);
                                temp[ fe - fb ] = 0;
                                s.m_blob = temp;

                                int line = 0;
                                const char* ns = strchr(stackLine, '(');
                                if (ns)
                                {
                                    ++ns;
                                    line = atoi(ns) - 1; // -1 offset to bridge editor vs display
                                }

                                s.m_blobLine = line;
                                sl.push_back(s);
                            }
                        }
                    }
                    else
                    {
                        // standard VS format
                        LUAEditor::StackEntry s;

                        const char* fb = stackLine;
                        const char* fe = strchr(fb, '(');
                        if (fe)
                        {
                            char temp[tempSize];
                            --fe;
                            ptrdiff_t pdt = fe - fb;
                            if (pdt < (tempSize - 1))
                            {
                                memcpy(temp, fb, pdt);
                                temp[ pdt ] = 0;
                                s.m_blob = stackLine;
                            }
                            else
                            {
                                pdt = tempSize - 5;
                                memcpy(temp, fb, pdt);
                                temp[ pdt++ ] = '.';
                                temp[ pdt++ ] = '.';
                                temp[ pdt++ ] = '.';
                                temp[ pdt ] = 0;
                                s.m_blob = stackLine;
                            }

                            int line = 0;
                            const char* ns = strchr(stackLine, '(');
                            if (ns)
                            {
                                ++ns;
                                line = atoi(ns) - 1; // -1 offset to bridge editor vs display
                            }

                            s.m_blobLine = line;
                            sl.push_back(s);
                        }
                    }
                }
                else
                {
                    // function pointers
                    LUAEditor::StackEntry s;
                    s.m_blobLine = 0;
                    s.m_blob = stackLine;
                    sl.push_back(s);
                }
            }
        }

        EBUS_EVENT(LUAEditor::LUAStackTrackerMessages::Bus, StackUpdate, sl);
    }

    void Context::RequestWatchedVariable(const AZStd::string& varName)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "RequestWatchedVariable: name=%s\n", varName.c_str());

        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, GetValue, varName);
    }

    void Context::OnReceivedValueState(const AZ::ScriptContextDebug::DebugValue& value)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Received LUA var: name=%s, val=%s, type=0x%x, flags=0x%x\n", value.m_name.c_str(), value.m_value.c_str(), (int)value.m_assetType, (int)value.m_flags);

        EBUS_EVENT(LUAEditor::LUAWatchesDebuggerMessages::Bus, WatchesUpdate, value);
    }

    void Context::OnSetValueResult(const AZStd::string& /*name*/, bool /*success*/)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "SetValue(%s) %s.\n", name.c_str(), success ? "succeeded" : "failed");
    }

    void Context::OnExecutionResumed()
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnExecutionResumed()\n");

        if (m_pLUAEditorMainWindow)
        {
            m_pLUAEditorMainWindow->MoveProgramCursor("", -1);
        }

        EBUS_EVENT(LUAEditor::LUAStackTrackerMessages::Bus, StackClear);
        EBUS_EVENT(LUABreakpointTrackerMessages::Bus, BreakpointResume);
    }

    void Context::OnExecuteScriptResult(bool success)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::OnExecutionScriptResult( %d )\n", success);

        EBUS_EVENT(LUAEditorMainWindowMessages::Bus, OnExecuteScriptResult, success);
    }

    void Context::ResetTargetContexts()
    {
        m_targetContexts.clear();
        m_currentTargetContext = "Default";

        EBUS_EVENT(LUAEditor::Context_ControlManagement::Bus, OnTargetContextPrepared, m_currentTargetContext);
    }

    void Context::SetCurrentTargetContext(AZStd::string& contextName)
    {
        //AZ_TracePrintf(LUAEditorDebugName, "Context::SetCurrentTargetContext()\n");

        RequestDetachDebugger();

        // is this a valid context, in our existing list from the target?
        size_t i;
        for (i = 0; i < m_targetContexts.size(); ++i)
        {
            if (contextName == m_targetContexts[i])
            {
                m_currentTargetContext = contextName;
                break;
            }
        }
        if (i >= m_targetContexts.size())
        {
            ResetTargetContexts();
        }

        EBUS_EVENT(LUAEditor::Context_ControlManagement::Bus, OnTargetContextPrepared, m_currentTargetContext);

        UpdateReferenceWindow();
        RequestAttachDebugger();
    }

    //////////////////////////////////////////////////////////////////////////
    //Debug Request messages
    //////////////////////////////////////////////////////////////////////////
    void Context::RequestDetachDebugger()
    {
        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, DetachDebugger);
    }
    void Context::RequestAttachDebugger()
    {
        EBUS_EVENT(LUAEditorDebuggerMessages::Bus, AttachDebugger, m_currentTargetContext.c_str());
    }

    ReferenceItem::ReferenceItem(const QIcon& icon, const QString& text, size_t id)
        : QStandardItem(icon, text)
        , m_Id(id)
    {
    }

    ReferenceItem::ReferenceItem(const QString& text, size_t id)
        : QStandardItem(text)
        , m_Id(id)
    {
    }

    void LUAEditorContextSavedState::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<LUAEditorContextSavedState, AZ::UserSettings >()
                ->Version(1)
                ->Field("m_MainEditorWindowIsVisible", &LUAEditorContextSavedState::m_MainEditorWindowIsVisible)
                ->Field("m_MainEditorWindowIsOpen", &LUAEditorContextSavedState::m_MainEditorWindowIsOpen);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // ContextFactory
    //////////////////////////////////////////////////////////////////////////
    void Context::Reflect(AZ::ReflectContext* reflection)
    {
        AzToolsFramework::LogPanel::BaseLogPanel::Reflect(reflection);
        AzToolsFramework::QTreeViewWithStateSaving::Reflect(reflection);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            Breakpoint::Reflect(reflection);
            BreakpointSavedState::Reflect(reflection);
            LUAEditorMainWindowSavedState::Reflect(reflection);
            LUAEditorContextSavedState::Reflect(reflection);
            SyntaxStyleSettings::Reflect(reflection);

            serializeContext->Class<Context>()
                ->Version(10)
                ;
        }


        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection);
        if (behaviorContext)
        {
            behaviorContext->Class<Context>("LUAEditor")->
                Method("SaveLayout", &Context::SaveLayout)->
                Method("LoadLayout", &Context::LoadLayout);

            behaviorContext->Property("luaEditor", BehaviorValueGetter(&s_pLUAEditorScriptPtr), nullptr);
        }
    }

    void Context::AssetCompilationSuccess(const AZStd::string& assetPath)
    {
        if (IsLuaAsset(assetPath))
        {
            AZ_TracePrintf(LUAEditorInfoName, "Compilation Successful - %s\n", assetPath.c_str());
        }
    }

    void Context::AssetCompilationFailed(const AZStd::string& assetPath)
    {
        if (IsLuaAsset(assetPath))
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_failedAssetMessagesMutex);
            m_failedAssets.push(assetPath);

            EBUS_QUEUE_FUNCTION(AZ::SystemTickBus, &Context::ProcessFailedAssetMessages, this);
        }
    }

    void Context::ProcessFailedAssetMessages()
    {
        AZStd::string currentAsset;
        bool foundAsset = false;
        do
        {
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_failedAssetMessagesMutex);
                if (m_failedAssets.empty())
                {
                    foundAsset = false;
                }
                else
                {
                    foundAsset = true;
                    currentAsset = AZStd::move(m_failedAssets.front());
                    m_failedAssets.pop();
                }
            }

            if (foundAsset)
            {
                AZStd::string msg = AZStd::string::format("Compilation Failed! (%s)\n", currentAsset.c_str());
                AZ_Warning(LUAEditorInfoName, false, msg.c_str());

                AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobInfoResult = AZ::Failure();
                AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(jobInfoResult, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfo, currentAsset, false);
                if (jobInfoResult.IsSuccess())
                {
                    const AzToolsFramework::AssetSystem::JobInfo &jobInfo = jobInfoResult.GetValue()[0];
                    AZ::Outcome<AZStd::string> logResult = AZ::Failure();
                    AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(logResult, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetJobLog, jobInfo.m_jobRunKey);
                    if (logResult.IsSuccess())
                    {
                        // Errors should come in the form of <timestamp> filename.lua:####: errormsg
                        std::regex errorRegex(".+\\.lua:(\\d+):(.*)");

                        AzToolsFramework::Logging::LogLine::ParseLog(logResult.GetValue().c_str(), logResult.GetValue().size(),
                            [this, &currentAsset, &errorRegex](AzToolsFramework::Logging::LogLine& logLine)
                        {
                            if ((logLine.GetLogType() == AzToolsFramework::Logging::LogLine::TYPE_WARNING) || (logLine.GetLogType() == AzToolsFramework::Logging::LogLine::TYPE_ERROR))
                            {
                                if (m_pLUAEditorMainWindow)
                                {
                                    m_errorData.push_back(new CompilationErrorData());
                                    auto errorData = m_errorData.back();

                                    // Get the full path from the currentAsset
                                    bool pathFound = false;
                                    AZStd::string fullPath;
                                    AzToolsFramework::AssetSystemRequestBus::BroadcastResult(pathFound, &AzToolsFramework::AssetSystemRequestBus::Events::GetFullSourcePathFromRelativeProductPath, currentAsset, errorData->m_filename);
                                    // Lower this so that it matches the asset_id used by the rest of the lua id when referring to open files
                                    AZStd::to_lower(errorData->m_filename.begin(), errorData->m_filename.end());

                                    // Errors should come in the form of <timestamp> filename.lua:####: errormsg
                                    AZStd::string logString = logLine.ToString();
                                    // Default the final message to the entire line in case it can't be parsed for line number and actual error
                                    AZStd::string finalMessage = logString;

                                    // Try to extract the line number here
                                    std::smatch match;
                                    std::string stdLogString = logString.c_str();
                                    bool matchFound = std::regex_search(stdLogString, match, errorRegex);

                                    if (matchFound)
                                    {
                                        int lineNumber = 0;
                                        if (AZ::StringFunc::LooksLikeInt(match[1].str().c_str(), &lineNumber))
                                        {
                                            errorData->m_lineNumber = lineNumber;
                                            finalMessage = match[2].str().c_str();
                                        }
                                    }

                                    m_pLUAEditorMainWindow->AddMessageToLog(logLine.GetLogType(), LUAEditorInfoName, finalMessage.c_str(), errorData);
                                }
                            }
                        });
                    }
                }
            }
        } while (foundAsset); // keep doing this as long as there's failed assets in the queue
    }

    bool Context::IsLuaAsset(const AZStd::string& assetPath)
    {
        return AZ::StringFunc::Path::IsExtension(assetPath.c_str(), ".lua");
    }
}
