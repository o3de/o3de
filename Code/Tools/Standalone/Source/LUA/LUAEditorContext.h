/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/UserSettings/UserSettings.h>
#include "LUAEditorContextInterface.h"
#include "LUAEditorContextMessages.h"
#include "LUABreakpointTrackerMessages.h"
#include "LUAStackTrackerMessages.h"
#include "LUATargetContextTrackerMessages.h"
#include "LUAWatchesDebuggerMessages.h"
#include "LUAEditorContextMessages.h"
#include "AzFramework/TargetManagement/TargetManagementAPI.h"
#include "ClassReferencePanel.hxx"
#include "LUAEditorMainWindow.hxx"
#include "LUAEditorStyleMessages.h"
#include <AzToolsFramework/SourceControl/SourceControlAPI.h>
#include <AzToolsFramework/UI/LegacyFramework/Core/EditorFrameworkAPI.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzCore/Script/ScriptTimePoint.h>
#include <QStandardItem>

#pragma once

namespace AZ
{
    namespace IO
    {
        class FileIOBase;
    }
}

namespace LUAEditor
{
    class ReferenceItem;
    class BreakpointSavedState;

    //////////////////////////////////////////////////////////////////////////
    // Context
    // all editor components are responsible for maintaining the list of documents they are responsible for
    // and setting up editing facilities on those asset types in that "space".
    // editor contexts are components and have component ID's because we will communicate to them via buses.

    // this is the non-gui side of lua editing.
    // for example, keeping track of assets, opening documents, that sort of thing.

    // even though the editor can run headlessly, it always registers its gui stuff, they just never get called
    // this is because even headlessly, some GUI stuff (like qtScintilla stufF) might be required to actually parse the resources
    // its also harmless to register GUI components because they don't do anything until you send them messages like 'register your GUI stuff'
    // in general, the editor component for a particular kind of asset will be the one that registers its GUI stuff.
    class Context
        : public AZ::Component
        , private LegacyFramework::CoreMessageBus::Handler
        , private ContextInterface::Handler
        , private Context_DocumentManagement::Handler
        , private Context_DebuggerManagement::Handler
        , private AzFramework::TargetManagerClient::Bus::Handler
        , private LUABreakpointRequestMessages::Handler
        , private LUAWatchesRequestMessages::Handler
        , private LUATargetContextRequestMessages::Handler
        , private LUAStackRequestMessages::Handler
        , private HighlightedWords::Handler
        , private AzFramework::AssetSystemInfoBus::Handler
    {
        friend class ContextFactory;
    public:

        AZ_COMPONENT(Context, "{8F606ADE-8D29-4239-9DF4-53E5E42D9685}");

        Context();
        virtual ~Context();

        Context(const Context&);


        //////////////////////////////////////////////////////////////////////////
        // AZ::Component
        virtual void Init();
        virtual void Activate();
        virtual void Deactivate();
        //////////////////////////////////////////////////////////////////////////

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //////////////////////////////////////////////////////////////////////////
        // EditorFramework::CoreMessageBus::Handler
        virtual void RunAsAnotherInstance();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::AssetSystemInfoBus::Handler
        virtual void AssetCompilationSuccess(const AZStd::string& assetPath) override;
        virtual void AssetCompilationFailed(const AZStd::string& assetPath) override;
        //////////////////////////////////////////////////////////////////////////


        //////////////////////////////////////////////////////////////////////////
        // IPC Handlers
        bool OnIPCOpenFiles(const AZStd::string& parameters);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ContextInterface Messages
        // it is an error to call GetDocumentData when the data is not yet ready.
        virtual void ShowLUAEditorView();
        // this occurs from time to time, generally triggered when some external event occurs
        // that makes it suspect that its document statuses might be invalid:
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        //Context_DocumentManagement Messages
        virtual void OnNewDocument(const AZStd::string& assetId);
        virtual void OnLoadDocument(const AZStd::string& assetId, bool errorOnNotFound);
        virtual void OnCloseDocument(const AZStd::string& assetId);
        virtual void OnSaveDocument(const AZStd::string& assetId, bool bCloseAfterSaved, bool bSaveAs);
        virtual bool OnSaveDocumentAs(const AZStd::string& assetId, bool bCloseAfterSaved);
        virtual void NotifyDocumentModified(const AZStd::string& assetId, bool modified);
        virtual void DocumentCheckOutRequested(const AZStd::string& assetId);
        virtual void RefreshAllDocumentPerforceStat();
        virtual void OnReloadDocument(const AZStd::string assetId);

        virtual void UpdateDocumentData(const AZStd::string& assetId, const char* dataPtr, const AZStd::size_t dataLength);
        virtual void GetDocumentData(const AZStd::string& assetId, const char** dataPtr, AZStd::size_t& dataLength);
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // Target Manager
        //////////////////////////////////////////////////////////////////////////
        virtual void DesiredTargetConnected(bool connected);
        virtual void DesiredTargetChanged(AZ::u32 newTargetID, AZ::u32 oldTargetID);

        //////////////////////////////////////////////////////////////////////////
        //Context_DebuggerManagement Messages
        //////////////////////////////////////////////////////////////////////////
        void ExecuteScriptBlob(const AZStd::string& fromAssetId, bool executeLocal);
        virtual void SynchronizeBreakpoints();
        virtual void CreateBreakpoint(const AZStd::string& fromAssetId, int lineNumber);
        virtual void MoveBreakpoint(const AZ::Uuid& breakpointUID, int lineNumber);
        virtual void DeleteBreakpoint(const AZ::Uuid& breakpointUID);
        virtual void CleanUpBreakpoints();

        // These come from the VM
        virtual void OnDebuggerAttached();
        virtual void OnDebuggerRefused();
        virtual void OnDebuggerDetached();
        virtual void OnBreakpointHit(const AZStd::string& assetIdString, int lineNumber);
        virtual void OnBreakpointAdded(const AZStd::string& assetIdString, int lineNumber);
        virtual void OnBreakpointRemoved(const AZStd::string& assetIdString, int lineNumber);
        virtual void OnReceivedAvailableContexts(const AZStd::vector<AZStd::string>& contexts);
        virtual void OnReceivedRegisteredClasses(const AzFramework::ScriptUserClassList& classes);
        virtual void OnReceivedRegisteredEBuses(const AzFramework::ScriptUserEBusList& ebuses);
        virtual void OnReceivedRegisteredGlobals(const AzFramework::ScriptUserMethodList& methods, const AzFramework::ScriptUserPropertyList& properties);
        virtual void OnReceivedLocalVariables(const AZStd::vector<AZStd::string>& vars);
        virtual void OnReceivedCallstack(const AZStd::vector<AZStd::string>& callstack);
        virtual void OnReceivedValueState(const AZ::ScriptContextDebug::DebugValue& value);
        virtual void OnSetValueResult(const AZStd::string& name, bool success);
        virtual void OnExecutionResumed();
        virtual void OnExecuteScriptResult(bool success);


        //////////////////////////////////////////////////////////////////////////
        //BreakpointTracker Messages
        //////////////////////////////////////////////////////////////////////////
        virtual const BreakpointMap* RequestBreakpoints();
        virtual void RequestEditorFocus(const AZStd::string& assetIdString, int lineNumber);
        virtual void RequestDeleteBreakpoint(const AZStd::string& assetIdString, int lineNumber);

        //////////////////////////////////////////////////////////////////////////
        //StackTracker Messages
        //////////////////////////////////////////////////////////////////////////
        virtual void RequestStackClicked(const AZStd::string& stackString, int lineNumber);

        //////////////////////////////////////////////////////////////////////////
        //TargetContextTracker Messages
        //////////////////////////////////////////////////////////////////////////
        virtual const AZStd::vector<AZStd::string> RequestTargetContexts();
        virtual const AZStd::string RequestCurrentTargetContext();
        virtual void SetCurrentTargetContext(AZStd::string& contextName);

        //////////////////////////////////////////////////////////////////////////
        //Watch window messages
        //////////////////////////////////////////////////////////////////////////
        virtual void RequestWatchedVariable(const AZStd::string& varName);

        //////////////////////////////////////////////////////////////////////////
        //Debug Request messages
        //////////////////////////////////////////////////////////////////////////
        virtual void RequestDetachDebugger();
        virtual void RequestAttachDebugger();

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework CoreMessages
        virtual void OnRestoreState(); // sent when everything is registered up and ready to go, this is what bootstraps stuff to get going.
        virtual bool OnGetPermissionToShutDown();
        virtual bool CheckOkayToShutDown();
        virtual void OnSaveState(); // sent to everything when the app is about to shut down - do what you need to do.
        virtual void OnDestroyState();
        virtual void ApplicationDeactivated();
        virtual void ApplicationActivated();
        virtual void ApplicationShow(AZ::Uuid id);
        virtual void ApplicationHide(AZ::Uuid id);
        virtual void ApplicationCensus();
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // HighlightedWords
        virtual const HighlightedWords::LUAKeywordsType* GetLUAKeywords() { return &m_LUAKeywords; }
        virtual const HighlightedWords::LUAKeywordsType* GetLUALibraryFunctions() { return &m_LUALibraryFunctions; }

        // internal data structure for the LUA debugger class/member/property reference panel
        // this is what we serialize and work with
        struct ContextReference
        {
            struct
            {
                AzFramework::ScriptUserMethodList m_methods;
                AzFramework::ScriptUserPropertyList m_properties;
            } m_globals;

            AzFramework::ScriptUserClassList m_classes;
            AzFramework::ScriptUserEBusList m_buses;
        };
        // map of context -> reference info
        AZStd::unordered_map<AZStd::string, ContextReference> m_reference;

        void UpdateReferenceWindow();
        void ResetTargetContexts();

        // this bridges to the QT data model built from our internal data handling format
        QStandardItemModel* m_referenceModel;
    protected:
        static void Reflect(AZ::ReflectContext* reflection);

    private:
        //////////////////////////////////////////////////////////////////////////
        // AssetManagementMessages
        //Our callback that tell us when the asset read request finishes
        void DataLoadDoneCallback(bool success, const AZStd::string& assetId);

        //Our callback that tell us when the asset write request finishes
        void DataSaveDoneCallback(bool success, const AZStd::string& assetId);

        // the asset manager will invoke this when the user indicates the intent to open an asset of a given type with a given name.
        // it will be up to the component that manages the state to actually perform what is needed to achieve this.
        virtual void AssetOpenRequested(const AZStd::string& assetId, bool errorOnNotFound);
        void OpenAssetByPhysicalPath(const char* physicalPath);

        void ProcessFailedAssetMessages();
        AZStd::mutex m_failedAssetMessagesMutex; // protects m_failedAssets from draining and adding entries into it from different threads at the same time
        AZStd::queue<AZStd::string> m_failedAssets;

        // the asset manager will invoke this when the user indicates they want to make a new asset of a given type.
        //virtual void AssetNewRequested(const AzToolsFramework::RegisteredAssetType& descriptor);
        //////////////////////////////////////////////////////////////////////////


        void AddDefaultLUAKeywords();
        void AddDefaultLUALibraryFunctions();

        bool m_queuedOpenRecent;
        AZStd::string mostRecentlyOpenedDocumentView;
        void OpenMostRecentDocumentView();

        void PerforceStatResponseCallback(bool success, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZStd::string& assetId);
        void PerforceRequestEditCallback(bool success, const AzToolsFramework::SourceControlFileInfo& fileInfo, const AZStd::string& assetId);

        bool IsLuaAsset(const AZStd::string& assetPath);

        typedef AZStd::unordered_map<AZStd::string, DocumentInfo> DocumentInfoMap;
        DocumentInfoMap m_documentInfoMap;

        LUAEditorMainWindow* m_pLUAEditorMainWindow;
        bool m_connectedState;

        AZStd::vector<AZStd::string> m_targetContexts;
        AZStd::string m_currentTargetContext;

        AZStd::vector<AZStd::string> m_filesToOpen;

    public:

        // Script interface:
        void LoadLayout();
        void SaveLayout();

    private:

        AZStd::vector<CompilationErrorData*> m_errorData;

        // utility
        void ProvisionalShowAndFocus(bool forceShow = false, bool forcedHide = false);

        // breakpoint types are carried by the tracker bus, the context uses those types internally as do listeners
        AZStd::intrusive_ptr<BreakpointSavedState> m_pBreakpointSavedState;

        //AssetTypes::LUAIntermediateHandler *m_pAssetHandler;
        AZStd::atomic_int m_numOutstandingOperations;
        bool m_bShuttingDown;
        bool m_bProcessingActivate;

        // these documents have been modified whilst the user was alt tabbed, we should check them:
        void ProcessReloadCheck();
        bool m_bReloadCheckQueued;
        AZStd::unordered_set<AZStd::string> m_reloadCheckDocuments;

        HighlightedWords::LUAKeywordsType m_LUAKeywords;
        HighlightedWords::LUAKeywordsType m_LUALibraryFunctions;

        AZ::IO::FileIOBase* m_fileIO;

        LegacyFramework::IPCHandleType m_ipcOpenFilesHandle;
    };

    class ReferenceItem
        : public QStandardItem
    {
    public:
        AZ_CLASS_ALLOCATOR(ReferenceItem, AZ::SystemAllocator, 0);

        ReferenceItem(const QIcon& icon, const QString& text, size_t id);
        ReferenceItem(const QString& text, size_t id);
        ~ReferenceItem() {}

        size_t GetTypeID() {return m_Id; }
    protected:
        size_t m_Id;
    };

    ////////////////////////////////////////////////////////////////////////////
    ////Context Factory
    //class ContextFactory : public AZ::ComponentFactory<Context>
    //{
    //public:
    //  AZ_CLASS_ALLOCATOR(ContextFactory,AZ::SystemAllocator,0)

    //  ContextFactory() : AZ::ComponentFactory<Context>(AZ_CRC("LUAEditor::Context", 0x304bb93b)){}
    //  virtual const char* GetName() {return "LUAEditor::Context";}
    //  virtual void        Reflect(AZ::ReflectContext* reflection);
    //};

    class LUAEditorContextSavedState
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(LUAEditorContextSavedState, "{3FEBF499-760C-4275-AF47-C1D5A131D4BA}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(LUAEditorContextSavedState, AZ::SystemAllocator, 0);

        bool m_MainEditorWindowIsVisible;
        bool m_MainEditorWindowIsOpen;

        LUAEditorContextSavedState()
            : m_MainEditorWindowIsVisible(true)
            , m_MainEditorWindowIsOpen(true) {}

        static void Reflect(AZ::ReflectContext* reflection);
    };
};

