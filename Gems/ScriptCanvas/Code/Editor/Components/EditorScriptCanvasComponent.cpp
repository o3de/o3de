/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/NamedEntityId.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Core/ScriptCanvasBus.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>
#include <LyViewPaneNames.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponentSerializer.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/PerformanceStatisticsBus.h>

namespace EditorScriptCanvasComponentCpp
{
    enum Version
    {
        PrefabIntegration = 10,
        InternalDev,
        AddSourceHandle,
        // add description above
        Current
    };
}

namespace ScriptCanvasEditor
{
    static bool EditorScriptCanvasComponentVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        if (rootElement.GetVersion() <= 4)
        {
            int assetElementIndex = rootElement.FindElement(AZ::Crc32("m_asset"));
            if (assetElementIndex == -1)
            {
                return false;
            }

            auto assetElement = rootElement.GetSubElement(assetElementIndex);
            AZ::Data::Asset<ScriptCanvasAsset> scriptCanvasAsset;
            if (!assetElement.GetData(scriptCanvasAsset))
            {
                AZ_Error("Script Canvas", false, "Unable to find Script Canvas Asset on a Version %u Editor ScriptCanvas Component", rootElement.GetVersion());
                return false;
            }

            ScriptCanvasAssetHolder assetHolder;
            assetHolder.SetAsset(scriptCanvasAsset.GetId());

            if (!rootElement.AddElementWithData(serializeContext, "m_assetHolder", assetHolder))
            {
                AZ_Error("Script Canvas", false, "Unable to add ScriptCanvas Asset Holder element when converting from version %u", rootElement.GetVersion())
            }

            rootElement.RemoveElementByName(AZ_CRC("m_asset", 0x4e58e538));
            rootElement.RemoveElementByName(AZ_CRC("m_openEditorButton", 0x9bcb3d5b));
        }

        if (rootElement.GetVersion() <= 6)
        {
            rootElement.RemoveElementByName(AZ_CRC("m_originalData", 0x2aee5989));
        }

        if (rootElement.GetVersion() <= 7)
        {
            rootElement.RemoveElementByName(AZ_CRC("m_variableEntityIdMap", 0xdc6c75a8));
        }

        if (rootElement.GetVersion() <= EditorScriptCanvasComponentCpp::Version::PrefabIntegration)
        {
            auto variableDataElementIndex = rootElement.FindElement(AZ_CRC_CE("m_variableData"));
            if (variableDataElementIndex == -1)
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: 'm_variableData' index was missing");
                return false;
            }

            auto& variableDataElement = rootElement.GetSubElement(variableDataElementIndex);

            ScriptCanvas::EditableVariableData editableData;
            if (!variableDataElement.GetData(editableData))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: could not retrieve old 'm_variableData'");
                return false;
            }

            auto scriptCanvasAssetHolderElementIndex = rootElement.FindElement(AZ_CRC_CE("m_assetHolder"));
            if (scriptCanvasAssetHolderElementIndex == -1)
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: 'm_assetHolder' index was missing");
                return false;
            }

            auto& scriptCanvasAssetHolderElement = rootElement.GetSubElement(scriptCanvasAssetHolderElementIndex);

            ScriptCanvasAssetHolder assetHolder;
            if (!scriptCanvasAssetHolderElement.GetData(assetHolder))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: could not retrieve old 'm_assetHolder'");
                return false;
            }

            rootElement.RemoveElement(variableDataElementIndex);

            if (!rootElement.AddElementWithData(serializeContext, "runtimeDataIsValid", true))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: failed to add 'runtimeDataIsValid'");
                return false;
            }

            ScriptCanvasBuilder::BuildVariableOverrides overrides;
            overrides.m_source = SourceHandle(nullptr, assetHolder.GetAssetId().m_guid, {});

            for (auto& variable : editableData.GetVariables())
            {
                overrides.m_overrides.push_back(variable.m_graphVariable);
            }

            if (!rootElement.AddElementWithData(serializeContext, "runtimeDataOverrides", overrides))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: failed to add 'runtimeDataOverrides'");
                return false;
            }
        }

        if (rootElement.GetVersion() < EditorScriptCanvasComponentCpp::Version::AddSourceHandle)
        {
            ScriptCanvasAssetHolder assetHolder;
            if (!rootElement.FindSubElementAndGetData(AZ_CRC_CE("m_assetHolder"), assetHolder))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: could not retrieve old 'm_assetHolder'");
                return false;
            }

            auto assetId = assetHolder.GetAssetId();
            auto path = assetHolder.GetAssetHint();

            if (!rootElement.AddElementWithData(serializeContext, "runtimeDataOverrides", SourceHandle(nullptr, assetId.m_guid, path)))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: failed to add 'sourceHandle'");
                return false;
            }
        }

        return true;
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorScriptCanvasComponent, EditorComponentBase>()
                ->Version(EditorScriptCanvasComponentCpp::Version::Current, &EditorScriptCanvasComponentVersionConverter)
                ->Field("m_name", &EditorScriptCanvasComponent::m_name)
                ->Field("runtimeDataIsValid", &EditorScriptCanvasComponent::m_runtimeDataIsValid)
                ->Field("runtimeDataOverrides", &EditorScriptCanvasComponent::m_variableOverrides)
                ->Field("sourceHandle", &EditorScriptCanvasComponent::m_sourceHandle)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorScriptCanvasComponent>("Script Canvas", "The Script Canvas component allows you to add a Script Canvas asset to a component, and have it execute on the specified entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/ScriptCanvas.svg")
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/ScriptCanvas/Viewport/ScriptCanvas.svg")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::PrimaryAssetType, ScriptCanvasAssetHandler::GetAssetTypeStatic())
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("UI", 0x27ff46b0))
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Level", 0x9aeacc13))
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/scripting/script-canvas/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_sourceHandle, "Script Canvas Source File", "Script Canvas source file associated with this component")
                        ->Attribute("BrowseIcon", ":/stylesheet/img/UI20/browse-edit-select-files.svg")
                        ->Attribute("EditButton", "")
                        ->Attribute("EditDescription", "Open in Script Canvas Editor")
                        ->Attribute("EditCallback", &EditorScriptCanvasComponent::OpenEditor)
                        ->Attribute(AZ::Edit::Attributes::AssetPickerTitle, "Script Canvas")
                        ->Attribute(AZ::Edit::Attributes::SourceAssetFilterPattern, "*.scriptcanvas")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorScriptCanvasComponent::OnFileSelectionChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_variableOverrides, "Properties", "Script Canvas Graph Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<AZ::EditorScriptCanvasComponentSerializer>()->HandlesType<EditorScriptCanvasComponent>();
        }
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent()
        : EditorScriptCanvasComponent(SourceHandle())
    {
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent(const SourceHandle& sourceHandle)
        : m_sourceHandle(sourceHandle)
    {
    }

    EditorScriptCanvasComponent::~EditorScriptCanvasComponent()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
    }

    const AZStd::string& EditorScriptCanvasComponent::GetName() const
    {
        return m_name;
    }

    void EditorScriptCanvasComponent::UpdateName()
    {
        SetName(m_sourceHandle.Path().Filename().Native());
    }

    void EditorScriptCanvasComponent::OpenEditor(const AZ::Data::AssetId&, const AZ::Data::AssetType&)
    {
        AzToolsFramework::OpenViewPane(LyViewPane::ScriptCanvas);
         
        AZ::Outcome<int, AZStd::string> openOutcome = AZ::Failure(AZStd::string());
         
        if (m_sourceHandle.IsDescriptionValid())
        {
            GeneralRequestBus::BroadcastResult(openOutcome, &GeneralRequests::OpenScriptCanvasAsset, m_sourceHandle, Tracker::ScriptCanvasFileState::UNMODIFIED, -1);
         
            if (!openOutcome)
            {
                AZ_Warning("Script Canvas", openOutcome, "%s", openOutcome.GetError().data());
            }
        }
        else if (GetEntityId().IsValid())
        {
            AzToolsFramework::EntityIdList selectedEntityIds;
            AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(selectedEntityIds, &AzToolsFramework::ToolsApplicationRequests::GetSelectedEntities);
         
            // Going to bypass the multiple selected entities flow for right now.
            if (selectedEntityIds.size() == 1)
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::CreateScriptCanvasAssetFor, AZStd::make_pair(GetEntityId(), GetId()));
            }
        }
    }

    void EditorScriptCanvasComponent::Init()
    {
        EditorComponentBase::Init();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void EditorScriptCanvasComponent::InitializeSource(const SourceHandle& sourceHandle)
    {
        m_sourceHandle = sourceHandle;
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Activate()
    {
        EditorComponentBase::Activate();

        AzToolsFramework::AssetSystemBus::Handler::BusConnect();

        AZ::EntityId entityId = GetEntityId();

        EditorContextMenuRequestBus::Handler::BusConnect(entityId);
        EditorScriptCanvasComponentRequestBus::Handler::BusConnect(entityId);

        EditorScriptCanvasComponentLoggingBus::Handler::BusConnect(entityId);
        EditorLoggingComponentNotificationBus::Broadcast(&EditorLoggingComponentNotifications::OnEditorScriptCanvasComponentActivated, GetNamedEntityId(), GetGraphIdentifier());

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Deactivate()
    {
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();

        EditorScriptCanvasComponentLoggingBus::Handler::BusDisconnect();
        EditorLoggingComponentNotificationBus::Broadcast(&EditorLoggingComponentNotifications::OnEditorScriptCanvasComponentDeactivated, GetNamedEntityId(), GetGraphIdentifier());

        EditorComponentBase::Deactivate();

        EditorScriptCanvasComponentRequestBus::Handler::BusDisconnect();
        EditorContextMenuRequestBus::Handler::BusDisconnect();
    }

    void EditorScriptCanvasComponent::BuildGameEntityData()
    {
        using namespace ScriptCanvasBuilder;

        m_runtimeDataIsValid = false;

        auto assetTreeOutcome = LoadEditorAssetTree(m_sourceHandle);
        if (!assetTreeOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false, "EditorScriptCanvasComponent::BuildGameEntityData failed: %s", assetTreeOutcome.GetError().c_str());
            return;
        }

        EditorAssetTree& editorAssetTree = assetTreeOutcome.GetValue();

        auto parseOutcome = ParseEditorAssetTree(editorAssetTree);
        if (!parseOutcome.IsSuccess())
        {
            AZ_Warning("ScriptCanvas", false, "EditorScriptCanvasComponent::BuildGameEntityData failed: %s", parseOutcome.GetError().c_str());
            return;
        }

        if (!m_variableOverrides.IsEmpty())
        {
            parseOutcome.GetValue().CopyPreviousOverriddenValues(m_variableOverrides);
        }

        m_variableOverrides = parseOutcome.TakeValue();
        m_runtimeDataIsValid = true;
    }

    void EditorScriptCanvasComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (!m_runtimeDataIsValid)
        {
            // this is fine, there could have been no graph set, or set to a graph that failed to compile
            return;
        }

        // build everything again as a sanity check against dependencies. All of the variable overrides that were valid will be copied over
        BuildGameEntityData();

        if (!m_runtimeDataIsValid)
        {
            AZ_Error("ScriptCanvasBuilder", false, "Runtime information did not build for ScriptCanvas Component using asset: %s"
                , m_sourceHandle.ToString().c_str());
            return;
        }

        auto runtimeComponent = gameEntity->CreateComponent<ScriptCanvas::RuntimeComponent>();
        runtimeComponent->TakeRuntimeDataOverrides(ConvertToRuntime(m_variableOverrides));
    }

    void EditorScriptCanvasComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        m_sourceHandle = SourceHandle(nullptr, assetId.m_guid, {});

        auto completeAsset = CompleteDescription(m_sourceHandle);
        if (completeAsset)
        {
            m_sourceHandle = *completeAsset;
        }

        OnScriptCanvasAssetChanged(SourceChangeDescription::SelectionChanged);
        SetName(m_sourceHandle.Path().Filename().Native());
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    AZ::Data::AssetId EditorScriptCanvasComponent::GetAssetId() const
    {
        return m_sourceHandle.Id();
    }

    AZ::u32 EditorScriptCanvasComponent::OnFileSelectionChanged()
    {
        m_sourceHandle = SourceHandle(nullptr, m_sourceHandle.Path());
        CompleteDescriptionInPlace(m_sourceHandle);
        m_previousHandle = {};
        m_removedHandle = {};
        OnScriptCanvasAssetChanged(SourceChangeDescription::SelectionChanged);
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }
    
    void EditorScriptCanvasComponent::OnScriptCanvasAssetChanged(SourceChangeDescription changeDescription)
    {
        ScriptCanvas::GraphIdentifier newIdentifier = GetGraphIdentifier();
        newIdentifier.m_assetId = m_sourceHandle.Id();

        ScriptCanvas::GraphIdentifier oldIdentifier = GetGraphIdentifier();
        oldIdentifier.m_assetId = m_previousHandle.Id();

        EditorLoggingComponentNotificationBus::Broadcast(&EditorLoggingComponentNotifications::OnAssetSwitched, GetNamedEntityId(), newIdentifier, oldIdentifier);

        m_previousHandle = m_sourceHandle.Describe();

        if (changeDescription == SourceChangeDescription::SelectionChanged)
        {
            ClearVariables();
        }

        if (m_sourceHandle.IsDescriptionValid())
        {
            if (!m_sourceHandle.Get())
            {
                if (auto loaded = LoadFromFile(m_sourceHandle.Path().c_str()); loaded.IsSuccess())
                {
                    m_sourceHandle = SourceHandle(loaded.TakeValue(), m_sourceHandle.Id(), m_sourceHandle.Path().c_str());
                }
            }

            if (m_sourceHandle.Get())
            {
                UpdatePropertyDisplay(m_sourceHandle);
            }
        }

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    void EditorScriptCanvasComponent::OnStartPlayInEditor()
    {
        ScriptCanvas::Execution::PerformanceStatisticsEBus::Broadcast(&ScriptCanvas::Execution::PerformanceStatisticsBus::ClearSnaphotStatistics);
    }

    void EditorScriptCanvasComponent::OnStopPlayInEditor()
    {
        AZ::ScriptSystemRequestBus::Broadcast(&AZ::ScriptSystemRequests::GarbageCollect);
    }

    void EditorScriptCanvasComponent::SetAssetId(const SourceHandle& assetId)
    {
        if (m_sourceHandle.Describe() != assetId.Describe())
        {
            // Invalidate the previously removed catalog id if we are setting a new asset id
            m_removedHandle = {};
            SetPrimaryAsset(assetId.Id());
        }
    }

    void EditorScriptCanvasComponent::SourceFileChanged([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        if (fileAssetId == m_sourceHandle.Id())
        {
            if (auto handle = CompleteDescription(SourceHandle(nullptr, fileAssetId, {})))
            {
                m_sourceHandle = *handle;
                // consider queueing on tick bus
                OnScriptCanvasAssetChanged(SourceChangeDescription::Modified);
            }
        }
    }

    void EditorScriptCanvasComponent::SourceFileRemoved([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        if (fileAssetId == m_sourceHandle.Id())
        {
            m_removedHandle = m_sourceHandle;
            OnScriptCanvasAssetChanged(SourceChangeDescription::Removed);
        }
    }

    void EditorScriptCanvasComponent::SourceFileFailed([[maybe_unused]] AZStd::string relativePath
        , [[maybe_unused]] AZStd::string scanFolder, [[maybe_unused]] AZ::Uuid fileAssetId)
    {
        if (fileAssetId == m_sourceHandle.Id())
        {
            m_removedHandle = m_sourceHandle;
            OnScriptCanvasAssetChanged(SourceChangeDescription::Error);
        }
    }

    bool EditorScriptCanvasComponent::HasAssetId() const
    {
        return !m_sourceHandle.Id().IsNull();
    }

    ScriptCanvas::GraphIdentifier EditorScriptCanvasComponent::GetGraphIdentifier() const
    {
        // For now we don't want to deal with disambiguating duplicates of the same script running on one entity.
        // Should that change we need to add the component id back into this.
        return ScriptCanvas::GraphIdentifier(m_sourceHandle.Id(), 0);
    }

    void EditorScriptCanvasComponent::UpdatePropertyDisplay(const SourceHandle& sourceHandle)
    {
        if (sourceHandle.IsGraphValid())
        {
            BuildGameEntityData();
            UpdateName();
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
        }
    }

    void EditorScriptCanvasComponent::ClearVariables()
    {
        m_variableOverrides.Clear();
    }
}
