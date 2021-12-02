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
#include <AzCore/std/string/wildcard.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>
#include <Editor/Framework/ScriptCanvasGraphUtilities.h>
#include <Editor/Settings.h>
#include <Editor/SystemComponent.h>
#include <Editor/View/Dialogs/NewGraphDialog.h>
#include <Editor/View/Dialogs/SettingsDialog.h>
#include <Editor/View/Widgets/SourceHandlePropertyAssetCtrl.h>
#include <Editor/View/Windows/MainWindow.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <LyViewPaneNames.h>
#include <QMenu>
#include <QMessageBox>
#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/View/EditCtrls/GenericLineEditCtrl.h>

namespace ScriptCanvasEditor
{
    static const size_t cs_jobThreads = 1;

    SystemComponent::SystemComponent()
    {
        AzToolsFramework::AssetSeedManagerRequests::Bus::Handler::BusConnect();
        m_versionExplorer = AZStd::make_unique<VersionExplorer::Model>();
    }

    SystemComponent::~SystemComponent()
    {
        AzToolsFramework::UnregisterViewPane(LyViewPane::ScriptCanvas);
        AzToolsFramework::EditorContextMenuBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        AzToolsFramework::AssetSeedManagerRequests::Bus::Handler::BusDisconnect();
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
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasEditorService", 0x4fe2af98));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("MemoryService", 0x5c4d473c)); // AZ::JobManager needs the thread pool allocator
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        required.push_back(GraphCanvas::GraphCanvasRequestsServiceId);
        required.push_back(AZ_CRC("ScriptCanvasReflectService", 0xb3bfe139));
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SystemComponent::Init()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::EditorContextMenuBus::Handler::BusConnect();
    }

    void SystemComponent::Activate()
    {
        m_assetTracker.Activate();
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

        auto userSettings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC("ScriptCanvasPreviewSettings", 0x1c5a2965), AZ::UserSettings::CT_LOCAL);
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
        AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        ScriptCanvasExecutionBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();

        m_jobContext.reset();
        m_jobManager.reset();
        m_assetTracker.Deactivate();
    }

    void SystemComponent::AddAsyncJob(AZStd::function<void()>&& jobFunc)
    {
        auto* asyncFunction = AZ::CreateJobFunction(AZStd::move(jobFunc), true, m_jobContext.get());
        asyncFunction->Start();
    }

    void SystemComponent::GetEditorCreatableTypes(AZStd::unordered_set<ScriptCanvas::Data::Type>& outCreatableTypes)
    {
        outCreatableTypes.insert(m_creatableTypes.begin(), m_creatableTypes.end());
    }

    void SystemComponent::CreateEditorComponentsOnEntity(AZ::Entity* entity, [[maybe_unused]] const AZ::Data::AssetType& assetType)
    {
        if (entity)
        {
            auto graph = entity->CreateComponent<Graph>();
            entity->CreateComponent<EditorGraphVariableManagerComponent>(graph->GetScriptCanvasId());
        }
    }

    void SystemComponent::PopulateEditorGlobalContextMenu(QMenu* menu, const AZ::Vector2& point, int flags)
    {
        (void)point;
        (void)flags;

        AzToolsFramework::EntityIdList entitiesWithScriptCanvas;

        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::EntityIdList highlightedEntities;

        EBUS_EVENT_RESULT(selectedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetSelectedEntities);

        FilterForScriptCanvasEnabledEntities(selectedEntities, entitiesWithScriptCanvas);

        EBUS_EVENT_RESULT(highlightedEntities,
            AzToolsFramework::ToolsApplicationRequests::Bus,
            GetHighlightedEntities);

        FilterForScriptCanvasEnabledEntities(highlightedEntities, entitiesWithScriptCanvas);

        if (!entitiesWithScriptCanvas.empty())
        {
            QMenu* scriptCanvasMenu = nullptr;
            QAction* action = nullptr;

            // For entities with script canvas component, create a context menu to open any existing script canvases within each selected entity.
            for (const AZ::EntityId& entityId : entitiesWithScriptCanvas)
            {
                if (!scriptCanvasMenu)
                {
                    menu->addSeparator();
                    scriptCanvasMenu = menu->addMenu(QObject::tr("Edit Script Canvas"));
                    scriptCanvasMenu->setEnabled(false);
                    menu->addSeparator();
                }

                AZ::Entity* entity = nullptr;
                EBUS_EVENT_RESULT(entity, AZ::ComponentApplicationBus, FindEntity, entityId);

                if (entity)
                {
                    AZ::EBusAggregateResults<AZ::Data::AssetId> assetIds;
                    EditorContextMenuRequestBus::EventResult(assetIds, entity->GetId(), &EditorContextMenuRequests::GetAssetId);

                    if (!assetIds.values.empty())
                    {
                        QMenu* entityMenu = scriptCanvasMenu;
                        if (entitiesWithScriptCanvas.size() > 1)
                        {
                            scriptCanvasMenu->setEnabled(true);
                            entityMenu = scriptCanvasMenu->addMenu(entity->GetName().c_str());
                            entityMenu->setEnabled(false);
                        }

                        AZStd::unordered_set< AZ::Data::AssetId > usedIds;

                        for (const auto& assetId : assetIds.values)
                        {
                            if (!assetId.IsValid() || usedIds.count(assetId) != 0)
                            {
                                continue;
                            }
 
                            entityMenu->setEnabled(true);
 
                            usedIds.insert(assetId);
 
                            AZStd::string rootPath;
                            AZ::Data::AssetInfo assetInfo = AssetHelpers::GetAssetInfo(assetId, rootPath);
 
                            AZStd::string displayName;
                            AZ::StringFunc::Path::GetFileName(assetInfo.m_relativePath.c_str(), displayName);
 
                            action = entityMenu->addAction(QString("%1").arg(QString(displayName.c_str())));
 
                            QObject::connect(action, &QAction::triggered, [assetId]
                            {
                                AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);
                                GeneralRequestBus::Broadcast(&GeneralRequests::OpenScriptCanvasAsset
                                    , SourceHandle(nullptr, assetId.m_guid, "")
                                    , Tracker::ScriptCanvasFileState::UNMODIFIED, -1);
                            });
                        }
                    }
                }
            }
        }
    }

    void SystemComponent::FilterForScriptCanvasEnabledEntities(AzToolsFramework::EntityIdList& sourceList, AzToolsFramework::EntityIdList& targetList)
    {
        for (const AZ::EntityId& entityId : sourceList)
        {
            if (entityId.IsValid())
            {
                if (EditorContextMenuRequestBus::FindFirstHandler(entityId))
                {
                    if (targetList.end() == AZStd::find(targetList.begin(), targetList.end(), entityId))
                    {
                        targetList.push_back(entityId);
                    }
                }
            }
        }
    }

    AzToolsFramework::AssetBrowser::SourceFileDetails SystemComponent::GetSourceFileDetails(const char* fullSourceFileName)
    {
        if (AZStd::wildcard_match("*.scriptcanvas", fullSourceFileName))
        {
            return AzToolsFramework::AssetBrowser::SourceFileDetails("Editor/Icons/AssetBrowser/ScriptCanvas_16.png");
        }

        // not one of our types.
        return AzToolsFramework::AssetBrowser::SourceFileDetails();
    }

    void SystemComponent::AddSourceFileOpeners
        ( [[maybe_unused]] const char* fullSourceFileName
        , [[maybe_unused]] const AZ::Uuid& sourceUUID
        , [[maybe_unused]] AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers)
    {
        using namespace AzToolsFramework;
        using namespace AzToolsFramework::AssetBrowser;

        bool isScriptCanvasAsset = false;
        ScriptCanvasAssetDescription scriptCanvasAssetDescription;
        if (AZStd::wildcard_match(AZStd::string::format("*%s", scriptCanvasAssetDescription.GetExtensionImpl()).c_str(), fullSourceFileName))
        {
            isScriptCanvasAsset = true;
        }

        if (isScriptCanvasAsset)
        {
            auto scriptCanvasEditorCallback = []([[maybe_unused]] const char* fullSourceFileNameInCall, const AZ::Uuid& sourceUUIDInCall)
            {
                AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
                const SourceAssetBrowserEntry* fullDetails = SourceAssetBrowserEntry::GetSourceByUuid(sourceUUIDInCall);
                if (fullDetails)
                {
                    AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);
 
                    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::OpenViewPane, "Script Canvas");
                    GeneralRequestBus::BroadcastResult(openOutcome
                        , &GeneralRequests::OpenScriptCanvasAsset
                        , SourceHandle(nullptr, sourceUUIDInCall, ""), Tracker::ScriptCanvasFileState::UNMODIFIED, -1);
                }
            };
 
            openers.push_back({ "O3DE_ScriptCanvasEditor", "Open In Script Canvas Editor...", QIcon(), scriptCanvasEditorCallback });
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

    Reporter SystemComponent::RunGraph(AZStd::string_view path, ScriptCanvas::ExecutionMode mode)
    {
        RunGraphSpec runGraphSpec;
        runGraphSpec.graphPath = path;
        runGraphSpec.runSpec.execution = mode;
        return ScriptCanvasEditor::RunGraph(runGraphSpec).front();
    }

    Reporter SystemComponent::RunAssetGraph(AZ::Data::Asset<AZ::Data::AssetData> asset, ScriptCanvas::ExecutionMode mode)
    {
        Reporter reporter;
        RunEditorAsset(asset, reporter, mode);
        return reporter;
    }

    AzToolsFramework::AssetSeedManagerRequests::AssetTypePairs SystemComponent::GetAssetTypeMapping()
    {
        return {
            { "scriptcanvas", "scriptcanvas_compiled" },
            { "scriptcanvas_fn", "scriptcanvas_fn_compiled" }
        };
    }

}
