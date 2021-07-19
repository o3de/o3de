/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"

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
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/PerformanceStatisticsBus.h>

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

        return true;
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorScriptCanvasComponent, EditorComponentBase>()
                ->Version(8, &EditorScriptCanvasComponentVersionConverter)
                ->Field("m_name", &EditorScriptCanvasComponent::m_name)
                ->Field("m_assetHolder", &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder)
                ->Field("m_variableData", &EditorScriptCanvasComponent::m_editableData)
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
                    ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/script-canvas/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_scriptCanvasAssetHolder, "Script Canvas Asset", "Script Canvas asset associated with this component")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_editableData, "Properties", "Script Canvas Graph Properties")
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

            ScriptCanvasMemoryAsset::pointer memoryAsset;
            AssetTrackerRequestBus::BroadcastResult(memoryAsset, &AssetTrackerRequests::GetAsset, fileAssetId);

            if (memoryAsset && memoryAsset->GetAsset().GetStatus() == AZ::Data::AssetData::AssetStatus::Ready)
            {
                OnScriptCanvasAssetReady(memoryAsset);
            }
            else
            {
                AssetTrackerRequestBus::Broadcast(&AssetTrackerRequests::Load, m_scriptCanvasAssetHolder.GetAssetId(), m_scriptCanvasAssetHolder.GetAssetType(), nullptr);
            }
        }
    }

    //=========================================================================
    void EditorScriptCanvasComponent::Deactivate()
    {
        AssetTrackerNotificationBus::Handler::BusDisconnect();

        EditorScriptCanvasComponentLoggingBus::Handler::BusDisconnect();
        EditorLoggingComponentNotificationBus::Broadcast(&EditorLoggingComponentNotifications::OnEditorScriptCanvasComponentDeactivated, GetNamedEntityId(), GetGraphIdentifier());

        EditorComponentBase::Deactivate();

        //EditorScriptCanvasAssetNotificationBus::Handler::BusDisconnect();
        EditorScriptCanvasComponentRequestBus::Handler::BusDisconnect();
        EditorContextMenuRequestBus::Handler::BusDisconnect();
    }

    //=========================================================================
    void EditorScriptCanvasComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        AZ::Data::AssetId editorAssetId = m_scriptCanvasAssetHolder.GetAssetId();

        if (!editorAssetId.IsValid())
        {
            return;
        }

        AZ::Data::AssetId runtimeAssetId(editorAssetId.m_guid, AZ_CRC("RuntimeData", 0x163310ae));
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset(runtimeAssetId, azrtti_typeid<ScriptCanvas::RuntimeAsset>(), {});

        /*

        This defense against creating useless runtime components is pending changes the slice update system.
        It also would require better abilities to check asset integrity when building assets that depend
        on ScriptCanvas assets.

        AZ::Data::AssetInfo assetInfo;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, runtimeAssetId);

        if (assetInfo.m_assetType == AZ::Data::s_invalidAssetType)
        {
            AZ_Warning("ScriptCanvas", false, "No ScriptCanvas Runtime Asset information for Entity ('%s' - '%s') Graph ('%s'), asset may be in error or deleted"
                , gameEntity->GetName().c_str()
                , GetEntityId().ToString().c_str()
                , GetName().c_str());
            return;
        }

        AzFramework::AssetSystem::AssetStatus statusResult = AzFramework::AssetSystem::AssetStatus_Unknown;
        AzFramework::AssetSystemRequestBus::BroadcastResult(statusResult, &AzFramework::AssetSystem::AssetSystemRequests::GetAssetStatusById, runtimeAssetId);

        if (statusResult != AzFramework::AssetSystem::AssetStatus_Compiled)
        {
            AZ_Warning("ScriptCanvas", false, "No ScriptCanvas Runtime Asset for Entity ('%s' - '%s') Graph ('%s'), compilation may have failed or not completed"
                , gameEntity->GetName().c_str()
                , GetEntityId().ToString().c_str()
                , GetName().c_str());
            return;
        }
        */

        // #functions2 dependency-ctor-args make recursive
        auto executionComponent = gameEntity->CreateComponent<ScriptCanvas::RuntimeComponent>(runtimeAsset);
        ScriptCanvas::VariableData varData;

        for (const auto& varConfig : m_editableData.GetVariables())
        {
            if (varConfig.m_graphVariable.GetDatum()->Empty())
            {
                AZ_Error("ScriptCanvas", false, "Data loss detected for GraphVariable ('%s') on Entity ('%s' - '%s') Graph ('%s')"
                    , varConfig.m_graphVariable.GetVariableName().data()
                    , gameEntity->GetName().c_str()
                    , GetEntityId().ToString().c_str()
                    , GetName().c_str());
            }
            else
            {
                varData.AddVariable(varConfig.m_graphVariable.GetVariableName(), varConfig.m_graphVariable);
            }
        }

        executionComponent->SetVariableOverrides(varData);
    }

    void EditorScriptCanvasComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        // If we removed out asset due to the catalog removing. Just set it back.
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

        // If the Asset gets removed from disk while the Editor is loaded clear out the asset reference.
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

        AzToolsFramework::ScopedUndoBatch undo("Update Entity With New SC Graph");
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(&AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity, GetEntityId());
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
            LoadVariables(memoryAsset);
            UpdateName();
        }
    }

    /*! Start Variable Block Implementation */
    void EditorScriptCanvasComponent::AddVariable(AZStd::string_view varName, const ScriptCanvas::GraphVariable& graphVariable)
    {
        // We only add component properties to the component
        if (!graphVariable.IsComponentProperty())
        {
            return;
        }

        const auto& variableId = graphVariable.GetVariableId();
        ScriptCanvas::EditableVariableConfiguration* originalVarNameValuePair = m_editableData.FindVariable(variableId);
        if (!originalVarNameValuePair)
        {
            m_editableData.AddVariable(varName, graphVariable);
            originalVarNameValuePair = m_editableData.FindVariable(variableId);
        }

        if (!originalVarNameValuePair)
        {
            AZ_Error("Script Canvas", false, "Unable to find variable with id %s and name %s on the ScriptCanvas Component. There is an issue in AddVariable",
                variableId.ToString().data(), varName.data());
            return;
        }

        // Update the variable name as it may have changed
        originalVarNameValuePair->m_graphVariable.SetVariableName(varName);
        originalVarNameValuePair->m_graphVariable.SetExposureCategory(graphVariable.GetExposureCategory());
        originalVarNameValuePair->m_graphVariable.SetScriptInputControlVisibility(AZ::Edit::PropertyVisibility::Hide);
        originalVarNameValuePair->m_graphVariable.SetAllowSignalOnChange(false);
    }

    void EditorScriptCanvasComponent::AddNewVariables(const ScriptCanvas::VariableData& graphVarData)
    {
        for (auto&& variablePair : graphVarData.GetVariables())
        {
            AddVariable(variablePair.second.GetVariableName(), variablePair.second);
        }
    }

    void EditorScriptCanvasComponent::RemoveVariable(const ScriptCanvas::VariableId& varId)
    {
        m_editableData.RemoveVariable(varId);
    }

    void EditorScriptCanvasComponent::RemoveOldVariables(const ScriptCanvas::VariableData& graphVarData)
    {
        AZStd::vector<ScriptCanvas::VariableId> oldVariableIds;
        for (auto varConfig : m_editableData.GetVariables())
        {
            const auto& variableId = varConfig.m_graphVariable.GetVariableId();

            // We only add component sourced graph properties to the script canvas component, so if this variable was switched to a graph-only property remove it.
            // Also be sure to remove this variable if it's been deleted entirely.
            auto graphVariable = graphVarData.FindVariable(variableId);
            if (!graphVariable || !graphVariable->IsComponentProperty())
            {
                oldVariableIds.push_back(variableId);
            }
        }

        for (const auto& oldVariableId : oldVariableIds)
        {
            RemoveVariable(oldVariableId);
        }
    }

    bool EditorScriptCanvasComponent::UpdateVariable(const ScriptCanvas::GraphVariable& graphDatum, ScriptCanvas::GraphVariable& updateDatum, ScriptCanvas::GraphVariable& originalDatum)
    {
        // If the editable datum is the different than the original datum, then the "variable value" has been overridden on this component

        // Variable values only propagate from the Script Canvas graph to this component if the original "variable value" has not been overridden
        // by the editable "variable value" on this component and the "variable value" on the graph is different than the variable value on this component
        auto isNotOverridden = (*updateDatum.GetDatum()) == (*originalDatum.GetDatum());
        auto scGraphIsModified = (*originalDatum.GetDatum()) != (*graphDatum.GetDatum());

        if (isNotOverridden && scGraphIsModified)
        {
            ScriptCanvas::ModifiableDatumView originalDatumView;
            originalDatum.ConfigureDatumView(originalDatumView);

            originalDatumView.AssignToDatum((*graphDatum.GetDatum()));


            ScriptCanvas::ModifiableDatumView updatedDatumView;
            updateDatum.ConfigureDatumView(updatedDatumView);

            updatedDatumView.AssignToDatum((*graphDatum.GetDatum()));

            return true;
        }
        return false;
    }

    void EditorScriptCanvasComponent::LoadVariables(const ScriptCanvasMemoryAsset::pointer memoryAsset)
    {
        auto assetData = memoryAsset->GetAsset();

        AZ::Entity* scriptCanvasEntity = assetData->GetScriptCanvasEntity();
        AZ_Assert(scriptCanvasEntity, "This graph must have a valid entity");

        auto variableComponent = scriptCanvasEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::GraphVariableManagerComponent>(scriptCanvasEntity) : nullptr;
        if (variableComponent)
        {
            // Add properties from the SC Asset to the SC Component if they do not exist on the SC Component
            AddNewVariables(*variableComponent->GetVariableData());
            RemoveOldVariables(*variableComponent->GetVariableData());
        }

        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    void EditorScriptCanvasComponent::ClearVariables()
    {
        m_editableData.Clear();
    }
    /* End Variable Block Implementation*/
}
