/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/Results.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/string/wildcard.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Network/IRemoteTools.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>
#include <Editor/Assets/ScriptCanvasAssetHelpers.h>
#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include <Editor/Settings.h>
#include <Editor/SystemComponent.h>
#include <Editor/View/Dialogs/SettingsDialog.h>
#include <Editor/View/Widgets/SourceHandlePropertyAssetCtrl.h>
#include <Editor/View/Windows/MainWindow.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <LyViewPaneNames.h>
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <ScriptCanvasContextIdentifiers.h>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/PerformanceStatisticsBus.h>
#include <ScriptCanvas/Utils/ScriptCanvasConstants.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvasEditor
{
    static const size_t cs_jobThreads = 1;

    SystemComponent::SystemComponent()
    {
        AzToolsFramework::AssetSeedManagerRequests::Bus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
        m_versionExplorer = AZStd::make_unique<VersionExplorer::Model>();
    }

    SystemComponent::~SystemComponent()
    {
        AzToolsFramework::UnregisterViewPane(LyViewPane::ScriptCanvas);
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::AssetSeedManagerRequests::Bus::Handler::BusDisconnect();
        AZ::SystemTickBus::Handler::BusDisconnect();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ScriptCanvasEditor::Settings::Reflect(serialize);

            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("Script Canvas Editor", "Script Canvas Editor System Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptCanvasEditorService"));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("ScriptCanvasService"));
        required.push_back(GraphCanvas::GraphCanvasRequestsServiceId);
        required.push_back(AZ_CRC_CE("ScriptCanvasReflectService"));
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SystemComponent::Init()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void SystemComponent::Activate()
    {
#if defined(ENABLE_REMOTE_TOOLS)
        if (auto* remoteToolsInterface = AzFramework::RemoteToolsInterface::Get())
        {
            remoteToolsInterface->RegisterToolingServiceHost(
                ScriptCanvas::RemoteToolsKey, ScriptCanvas::RemoteToolsName, ScriptCanvas::RemoteToolsPort);
        }
#endif

        AZ::JobManagerDesc jobDesc;
        for (size_t i = 0; i < cs_jobThreads; ++i)
        {
            jobDesc.m_workerThreads.push_back({ static_cast<int>(i) });
        }
        m_jobManager = AZStd::make_unique<AZ::JobManager>(jobDesc);
        m_jobContext = AZStd::make_unique<AZ::JobContext>(*m_jobManager);

        PopulateEditorCreatableTypes();

        AzToolsFramework::RegisterGenericComboBoxHandler<ScriptCanvas::VariableId>();
        if (AzToolsFramework::PropertyTypeRegistrationMessages::Bus::FindFirstHandler())
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(&AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType, aznew SourceHandlePropertyHandler());
        }

        SystemRequestBus::Handler::BusConnect();
        ScriptCanvasExecutionBus::Handler::BusConnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();

        auto userSettings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC_CE("ScriptCanvasPreviewSettings"), AZ::UserSettings::CT_LOCAL);
        if (userSettings)
        {
            if (userSettings->m_showUpgradeDialog)
            {
            }
            else
            {
                m_upgradeDisabled = true;
            }
        }

        m_nodeReplacementSystem.LoadReplacementMetadata();
    }

    void SystemComponent::NotifyRegisterViews()
    {
        QtViewOptions options;
        options.canHaveMultipleInstances = false;
        options.isPreview = true;
        options.showInMenu = true;
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/Menu/script_canvas_editor.svg";

        AzToolsFramework::RegisterViewPane<ScriptCanvasEditor::MainWindow>(LyViewPane::ScriptCanvas, LyViewPane::CategoryTools, options);
    }

    void SystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        m_nodeReplacementSystem.UnloadReplacementMetadata();
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ScriptCanvasExecutionBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();

        m_jobContext.reset();
        m_jobManager.reset();
    }

    void SystemComponent::CreateEditorComponentsOnEntity(AZ::Entity* entity, [[maybe_unused]] const AZ::Data::AssetType& assetType)
    {
        if (entity)
        {
            auto graph = entity->CreateComponent<EditorGraph>();
            entity->CreateComponent<EditorGraphVariableManagerComponent>(graph->GetScriptCanvasId());
        }
    }

    void SystemComponent::GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes)
    {
        outCreatableTypes.insert(m_creatableTypes.begin(), m_creatableTypes.end());
    }

    void SystemComponent::RequestGarbageCollect()
    {
        m_isGarbageCollectRequested = true;
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails SystemComponent::GetSourceFileDetails(const char* fullSourceFileName)
    {
        if (AZStd::wildcard_match("*.scriptcanvas", fullSourceFileName))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails("../Editor/Icons/AssetBrowser/ScriptCanvas_80.svg");
        }

        // not one of our types.
        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    void SystemComponent::AddSourceFileCreators
        ( [[maybe_unused]] const char* fullSourceFolderName
        , [[maybe_unused]] const AZ::Uuid& sourceUUID
        , AzToolsFramework::AssetBrowser::SourceFileCreatorList& creators)
    {
        auto scriptCavnasAssetCreator = [](const AZStd::string& fullSourceFolderNameInCallback, [[maybe_unused]] const AZ::Uuid& sourceUUID)
        {
            const AZStd::string defaultFilename = "NewScript";
            const AZStd::string scriptCanvasExtension = ScriptCanvasEditor::SourceDescription::GetFileExtension();

            AZStd::string fullFilepath;
            AZ::StringFunc::Path::ConstructFull(fullSourceFolderNameInCallback.c_str()
                , defaultFilename.c_str()
                , scriptCanvasExtension.c_str()
                , fullFilepath);

            int fileCounter = 0;
            while (AZ::IO::FileIOBase::GetInstance()->Exists(fullFilepath.c_str()))
            {
                fileCounter++;
                const AZStd::string incrementalFilename = defaultFilename + AZStd::to_string(fileCounter);

                AZ::StringFunc::Path::ConstructFull(fullSourceFolderNameInCallback.c_str()
                    , incrementalFilename.c_str()
                    , scriptCanvasExtension.c_str()
                    , fullFilepath);
            }

            const AZ::IO::Path fullAzFilePath = fullFilepath;
            const ScriptCanvas::DataPtr graph = EditorGraph::Create();
            SourceHandle source = SourceHandle::FromRelativePath(graph, fullAzFilePath.RelativePath());
            source = SourceHandle::MarkAbsolutePath(source, fullAzFilePath);

            AZ::IO::FileIOStream fileStream(fullAzFilePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText);
            if (fileStream.IsOpen())
            {
                auto serializeResult = Serialize(*source.Data(), fileStream);
                if (!serializeResult)
                {
                    AZ_Error("ScriptCanvasCreator", false, "Failed to save new ScriptCanvas file: %s", serializeResult.m_errors.c_str());
                }
                else
                {
                    AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotificationBus::Event(
                        AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::FileCreationNotificationBusId,
                        &AzToolsFramework::AssetBrowser::AssetBrowserFileCreationNotifications::HandleAssetCreatedInEditor,
                        source.AbsolutePath().Native(),
                        AZ::Crc32(),
                        true);
                }

                fileStream.Close();
            }
            else
            {
                AZ_Error("ScriptCanvasCreator", false, "Asset creation failed because file failed to open: %s", fullAzFilePath.c_str());
            }
        };

        creators.push_back({ "ScriptCanvas_creator", "ScriptCanvas Graph", QIcon(), scriptCavnasAssetCreator });
    }

    void SystemComponent::AddSourceFileOpeners
        ( [[maybe_unused]] const char* fullSourceFileName
        , [[maybe_unused]] const AZ::Uuid& sourceUUID
        , [[maybe_unused]] AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;

        if (AZ::IO::Path(fullSourceFileName).Extension() == ScriptCanvasEditor::SourceDescription::GetFileExtension())
        {
            auto scriptCanvasOpenInEditorCallback = []([[maybe_unused]] const char* fullSourceFileNameInCall, const AZ::Uuid& sourceUUIDInCall)
            {
                AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());

                auto sourceHandle = CompleteDescription(SourceHandle(nullptr, sourceUUIDInCall));

                if (sourceHandle)
                {
                    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane, "Script Canvas");

                    GeneralRequestBus::BroadcastResult(openOutcome
                        , &GeneralRequests::OpenScriptCanvasAsset
                        , *sourceHandle
                        , Tracker::ScriptCanvasFileState::UNMODIFIED
                        , -1);

                    if (!openOutcome.IsSuccess())
                    {
                        AZ_Error("ScriptCanvas", false, openOutcome.GetError().data());
                    }
                }
                else
                {
                    AZ_Warning("ScriptCanvas", false
                        , "Unabled to find full path for Source UUid %s", sourceUUIDInCall.ToString<AZStd::string>().c_str());
                }
            };

            openers.push_back({ "O3DE_ScriptCanvasEditor"
                , "Open In Script Canvas Editor..."
                , QIcon(ScriptCanvasEditor::SourceDescription::GetIconPath())
                , scriptCanvasOpenInEditorCallback });
        }
    }

    void SystemComponent::OnStartPlayInEditor()
    {
        ScriptCanvas::Execution::PerformanceStatisticsEBus::Broadcast(&ScriptCanvas::Execution::PerformanceStatisticsBus::ClearSnaphotStatistics);
    }

    void SystemComponent::OnStopPlayInEditor()
    {
        AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);
    }

    void SystemComponent::OnSystemTick()
    {
        if (m_isGarbageCollectRequested)
        {
            m_isGarbageCollectRequested = false;
            AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);
        }
    }

    void SystemComponent::OnUserSettingsActivated()
    {
        PopulateEditorCreatableTypes();
    }

    void SystemComponent::PopulateEditorCreatableTypes()
    {
        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Behavior Context should not be missing at this point");

        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        for (const auto& scType : dataRegistry->m_creatableTypes)
        {
            if (scType.first.GetType() == ScriptCanvas::Data::eType::BehaviorContextObject)
            {
                if (const AZ::BehaviorClass* behaviorClass = AZ::BehaviorContextHelper::GetClass(behaviorContext, ScriptCanvas::Data::ToAZType(scType.first)))
                {
                    // BehaviorContext classes with the ExcludeFrom attribute with a value of the ExcludeFlags::All are not added to the list of 
                    // types that can be created in the editor
                    const AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::All;
                    auto excludeClassAttributeData = azrtti_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));
                    if (excludeClassAttributeData && (excludeClassAttributeData->Get(nullptr) & exclusionFlags))
                    {
                        continue;
                    }
                }
            }

            m_creatableTypes.insert(scType.first);
        }
    }

    Reporter SystemComponent::RunAssetGraph(SourceHandle asset, ScriptCanvas::ExecutionMode mode)
    {
        Reporter reporter;
        RunEditorAsset(asset, reporter, mode);
        return reporter;
    }

    Reporter SystemComponent::RunGraph(AZStd::string_view path, ScriptCanvas::ExecutionMode mode)
    {
        RunGraphSpec runGraphSpec;
        runGraphSpec.graphPath = path;
        runGraphSpec.runSpec.execution = mode;
        return ScriptCanvasEditor::RunGraph(runGraphSpec).front();
    }

    AzToolsFramework::AssetSeedManagerRequests::AssetTypePairs SystemComponent::GetAssetTypeMapping()
    {
        return {
            { "scriptcanvas", "scriptcanvas_compiled" },
            { "scriptcanvas_fn", "scriptcanvas_fn_compiled" }
        };
    }

    void SystemComponent::OnActionContextRegistrationHook()
    {
        if (auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get())
        {
            AzToolsFramework::ActionContextProperties contextProperties;
            contextProperties.m_name = "O3DE Script Canvas";

            // Register custom action contexts to allow duplicated shortcut hotkeys to work
            actionManagerInterface->RegisterActionContext(ScriptCanvasIdentifiers::ScriptCanvasActionContextIdentifier, contextProperties);
            actionManagerInterface->RegisterActionContext(ScriptCanvasIdentifiers::ScriptCanvasVariablesActionContextIdentifier, contextProperties);
        }
    }

}
