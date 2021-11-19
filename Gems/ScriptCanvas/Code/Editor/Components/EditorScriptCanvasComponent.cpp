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
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Core/ScriptCanvasBus.h>
#include <Editor/Assets/ScriptCanvasAssetTrackerBus.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/PerformanceStatisticsBus.h>


namespace EditorScriptCanvasComponentCpp
{
    enum Version
    {
        PrefabIntegration = 10,

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
            overrides.m_source = AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset>(assetHolder.GetAssetId(), assetHolder.GetAssetType(), assetHolder.GetAssetHint());;

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
                ->Field("m_assetHolder", &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder)
                ->Field("runtimeDataIsValid", &EditorScriptCanvasComponent::m_runtimeDataIsValid)
                ->Field("runtimeDataOverrides", &EditorScriptCanvasComponent::m_variableOverrides)
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
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder, "Script Canvas Asset", "Script Canvas asset associated with this component")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_variableOverrides, "Properties", "Script Canvas Graph Properties")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent()
        : EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset>())
    {
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent(AZ::Data::Asset<ScriptCanvasAsset> asset)
        : m_scriptCanvasAssetHolder()
    {
        if (asset.GetId().IsValid())
        {
            m_scriptCanvasAssetHolder.SetAsset(asset.GetId());
        }

        m_scriptCanvasAssetHolder.SetScriptChangedCB([this](AZ::Data::AssetId assetId) { OnScriptCanvasAssetChanged(assetId); });
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
        AZ::Data::AssetId assetId = m_scriptCanvasAssetHolder.GetAssetId();
        if (assetId.IsValid())
        {
            // Pathname from the asset doesn't seem to return a value unless the asset has been loaded up once(which isn't done until we try to show it).
            // Using the Job system to determine the asset name instead.
            AZ::Outcome<AzToolsFramework::AssetSystem::JobInfoContainer> jobOutcome = AZ::Failure();
            AzToolsFramework::AssetSystemJobRequestBus::BroadcastResult(jobOutcome, &AzToolsFramework::AssetSystemJobRequestBus::Events::GetAssetJobsInfoByAssetID, assetId, false, false);

            AZStd::string assetPath;
            AZStd::string assetName;

            if (jobOutcome.IsSuccess())
            {
                AzToolsFramework::AssetSystem::JobInfoContainer& jobs = jobOutcome.GetValue();

                // Get the asset relative path
                if (!jobs.empty())
                {
                    assetPath = jobs[0].m_sourceFile;
                }

                // Get the asset file name
                assetName = assetPath;

                if (!assetPath.empty())
                {
                    AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), assetName);
                    SetName(assetName);
                }
            }
        }
    }

    void EditorScriptCanvasComponent::OpenEditor()
    {
        m_scriptCanvasAssetHolder.OpenEditor();
    }

    void EditorScriptCanvasComponent::CloseGraph()
    {
        AZ::Data::AssetId assetId = m_scriptCanvasAssetHolder.GetAssetId();
        if (assetId.IsValid())
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::CloseScriptCanvasAsset, assetId);
        }
    }

    void EditorScriptCanvasComponent::Init()
    {
        EditorComponentBase::Init();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        m_scriptCanvasAssetHolder.Init(GetEntityId(), GetId());
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Activate()
    {
        EditorComponentBase::Activate();

        AZ::EntityId entityId = GetEntityId();

        EditorContextMenuRequestBus::Handler::BusConnect(entityId);
        EditorScriptCanvasComponentRequestBus::Handler::BusConnect(entityId);

        EditorScriptCanvasComponentLoggingBus::Handler::BusConnect(entityId);
        EditorLoggingComponentNotificationBus::Broadcast(&EditorLoggingComponentNotifications::OnEditorScriptCanvasComponentActivated, GetNamedEntityId(), GetGraphIdentifier());

        AZ::Data::AssetId fileAssetId = m_scriptCanvasAssetHolder.GetAssetId();

        if (fileAssetId.IsValid())
        {
            AssetTrackerNotificationBus::Handler::BusConnect(fileAssetId);
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
        }
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Deactivate()
    {
        AssetTrackerNotificationBus::Handler::BusDisconnect();

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

        auto assetTreeOutcome = LoadEditorAssetTree(m_scriptCanvasAssetHolder.GetAssetId(), m_scriptCanvasAssetHolder.GetAssetHint());
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
            AZ_Error("ScriptCanvasBuilder", false, "Runtime information did not build for ScriptCanvas Component using asset: %s", m_scriptCanvasAssetHolder.GetAssetId().ToString<AZStd::string>().c_str());
            return;
        }

        auto runtimeComponent = gameEntity->CreateComponent<ScriptCanvas::RuntimeComponent>();
        runtimeComponent->TakeRuntimeDataOverrides(ConvertToRuntime(m_variableOverrides));
    }

    void EditorScriptCanvasComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        if (m_removedCatalogId == assetId)
        {
            if (!m_scriptCanvasAssetHolder.GetAssetId().IsValid())
            {
                SetPrimaryAsset(assetId);
                m_removedCatalogId.SetInvalid();
            }
        }
    }
    void EditorScriptCanvasComponent::OnCatalogAssetRemoved(const AZ::Data::AssetId& removedAssetId, const AZ::Data::AssetInfo& /*assetInfo*/)
    {
        AZ::Data::AssetId assetId = m_scriptCanvasAssetHolder.GetAssetId();
        if (assetId == removedAssetId)
        {
            m_removedCatalogId = assetId;
            SetPrimaryAsset({});
        }
    }

    void EditorScriptCanvasComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        m_scriptCanvasAssetHolder.ClearAsset();

        if (assetId.IsValid())
        {
            ScriptCanvasMemoryAsset::pointer memoryAsset;
            AssetTrackerRequestBus::BroadcastResult(memoryAsset, &AssetTrackerRequests::GetAsset, assetId);

            if (memoryAsset)
            {
                m_scriptCanvasAssetHolder.SetAsset(memoryAsset->GetFileAssetId());
                OnScriptCanvasAssetChanged(memoryAsset->GetFileAssetId());
                SetName(memoryAsset->GetTabName());
            }
            else
            {
                auto scriptCanvasAsset = AZ::Data::AssetManager::Instance().FindAsset(assetId, AZ::Data::AssetLoadBehavior::Default);
                if (scriptCanvasAsset)
                {
                    m_scriptCanvasAssetHolder.SetAsset(assetId);
                }
            }
        }

        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    AZ::Data::AssetId EditorScriptCanvasComponent::GetAssetId() const
    {
        return m_scriptCanvasAssetHolder.GetAssetId();
    }

    AZ::EntityId EditorScriptCanvasComponent::GetGraphEntityId() const
    {
        AZ::EntityId scriptCanvasEntityId;
        AZ::Data::AssetId assetId = m_scriptCanvasAssetHolder.GetAssetId();

        if (assetId.IsValid())
        {
            AssetTrackerRequestBus::BroadcastResult(scriptCanvasEntityId, &AssetTrackerRequests::GetScriptCanvasId, assetId);
        }

        return scriptCanvasEntityId;
    }

    void EditorScriptCanvasComponent::OnAssetReady(const ScriptCanvasMemoryAsset::pointer asset)
    {
        OnScriptCanvasAssetReady(asset);
    }

    void EditorScriptCanvasComponent::OnAssetSaved(const ScriptCanvasMemoryAsset::pointer asset, bool isSuccessful)
    {
        if (isSuccessful)
        {
            OnScriptCanvasAssetReady(asset);
        }
    }

    void EditorScriptCanvasComponent::OnAssetReloaded(const ScriptCanvasMemoryAsset::pointer asset)
    {
        OnScriptCanvasAssetReady(asset);
    }

    void EditorScriptCanvasComponent::OnScriptCanvasAssetChanged(AZ::Data::AssetId assetId)
    {
        AssetTrackerNotificationBus::Handler::BusDisconnect();
        ScriptCanvas::GraphIdentifier newIdentifier = GetGraphIdentifier();
        newIdentifier.m_assetId = assetId;

        ScriptCanvas::GraphIdentifier oldIdentifier = GetGraphIdentifier();
        oldIdentifier.m_assetId = m_previousAssetId;

        EditorLoggingComponentNotificationBus::Broadcast(&EditorLoggingComponentNotifications::OnAssetSwitched, GetNamedEntityId(), newIdentifier, oldIdentifier);

        m_previousAssetId = m_scriptCanvasAssetHolder.GetAssetId();

        // Only clear our variables when we are given a new asset id
        // or when the asset was explicitly set to empty.
        //
        // i.e. do not clear variables when we lose the catalog asset.
        if ((assetId.IsValid() && assetId != m_removedCatalogId)
            || (!assetId.IsValid() && !m_removedCatalogId.IsValid()))
        {
            ClearVariables();
        }

        if (assetId.IsValid())
        {
            AssetTrackerNotificationBus::Handler::BusConnect(assetId);

            ScriptCanvasMemoryAsset::pointer memoryAsset;
            AssetTrackerRequestBus::BroadcastResult(memoryAsset, &AssetTrackerRequests::GetAsset, assetId);
            if (memoryAsset && memoryAsset->GetAsset().GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
            {
                OnScriptCanvasAssetReady(memoryAsset);
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

    void EditorScriptCanvasComponent::SetAssetId(const AZ::Data::AssetId& assetId)
    {
        if (m_scriptCanvasAssetHolder.GetAssetId() != assetId)
        {
            // Invalidate the previously removed catalog id if we are setting a new asset id
            m_removedCatalogId.SetInvalid();
            SetPrimaryAsset(assetId);
        }
    }

    bool EditorScriptCanvasComponent::HasAssetId() const
    {
        return m_scriptCanvasAssetHolder.GetAssetId().IsValid();
    }

    ScriptCanvas::GraphIdentifier EditorScriptCanvasComponent::GetGraphIdentifier() const
    {
        // For now we don't want to deal with disambiguating duplicates of the same script running on one entity.
        // Should that change we need to add the component id back into this.
        return ScriptCanvas::GraphIdentifier(m_scriptCanvasAssetHolder.GetAssetId(), 0);
    }

    void EditorScriptCanvasComponent::OnScriptCanvasAssetReady(const ScriptCanvasMemoryAsset::pointer memoryAsset)
    {
        if (memoryAsset->GetFileAssetId() == m_scriptCanvasAssetHolder.GetAssetId())
        {
            auto assetData = memoryAsset->GetAsset();
            [[maybe_unused]] AZ::Entity* scriptCanvasEntity = assetData->GetScriptCanvasEntity();
            AZ_Assert(scriptCanvasEntity, "This graph must have a valid entity");
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
