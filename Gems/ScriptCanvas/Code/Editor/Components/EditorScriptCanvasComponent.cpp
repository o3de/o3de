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
#include <Builder/ScriptCanvasBuilder.h>
#include <Core/ScriptCanvasBus.h>
#include <LyViewPaneNames.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasFileHandling.h>
#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/Components/EditorDeprecationData.h>
#include <ScriptCanvas/Components/EditorGraph.h>
#include <ScriptCanvas/Components/EditorGraphVariableManagerComponent.h>
#include <ScriptCanvas/Components/EditorScriptCanvasComponent.h>
#include <ScriptCanvas/Components/EditorUtils.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>

namespace EditorScriptCanvasComponentCpp
{
    enum Version
    {
        PrefabIntegration = 10,
        InternalDev,
        AddSourceHandle,
        RefactorAssets,
        RemoveRuntimeData,
        SeparateFromConfiguration,
        // add description above
        Current
    };

    bool EditorScriptCanvasComponentVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
    {
        using namespace ScriptCanvasEditor;

        if (rootElement.GetVersion() <= 4)
        {
            int assetElementIndex = rootElement.FindElement(AZ::Crc32("m_asset"));
            if (assetElementIndex == -1)
            {
                return false;
            }

            auto assetElement = rootElement.GetSubElement(assetElementIndex);
            AZ::Data::Asset<Deprecated::ScriptCanvasAsset> scriptCanvasAsset;
            if (!assetElement.GetData(scriptCanvasAsset))
            {
                AZ_Error("Script Canvas", false, "Unable to find Script Canvas Asset on a Version %u Editor ScriptCanvas Component", rootElement.GetVersion());
                return false;
            }

            Deprecated::ScriptCanvasAssetHolder assetHolder;
            assetHolder.m_scriptCanvasAsset = scriptCanvasAsset;

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

            Deprecated::ScriptCanvasAssetHolder assetHolder;
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
            overrides.m_source = SourceHandle(nullptr, assetHolder.m_scriptCanvasAsset.GetId().m_guid, {});

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

        auto scriptCanvasAssetHolderElementIndex = rootElement.FindElement(AZ_CRC_CE("m_assetHolder"));
        if (scriptCanvasAssetHolderElementIndex != -1)
        {
            auto& scriptCanvasAssetHolderElement = rootElement.GetSubElement(scriptCanvasAssetHolderElementIndex);
            Deprecated::ScriptCanvasAssetHolder assetHolder;

            if (!scriptCanvasAssetHolderElement.GetData(assetHolder))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: could not retrieve old 'm_assetHolder'");
                return false;
            }

            auto assetId = assetHolder.m_scriptCanvasAsset.GetId();
            auto path = assetHolder.m_scriptCanvasAsset.GetHint();

            if (!rootElement.AddElementWithData(serializeContext, "sourceHandle", SourceHandle(nullptr, assetId.m_guid, path)))
            {
                AZ_Error("ScriptCanvas", false, "EditorScriptCanvasComponent conversion failed: failed to add 'sourceHandle'");
                return false;
            }
        }

        return true;
    }
}

namespace ScriptCanvasEditor
{
    EditorScriptCanvasComponent::EditorScriptCanvasComponent()
        : EditorScriptCanvasComponent(SourceHandle())
    {
    }

    EditorScriptCanvasComponent::EditorScriptCanvasComponent(const SourceHandle& sourceHandle)
        : m_configuration(sourceHandle)
    {
    }

    EditorScriptCanvasComponent::~EditorScriptCanvasComponent()
    {
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    void EditorScriptCanvasComponent::Activate()
    {
        EditorComponentBase::Activate();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        AZ::EntityId entityId = GetEntityId();
        m_configuration.Refresh();
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_EntireTree_NewContent);
    }

    void EditorScriptCanvasComponent::Deactivate()
    {
        EditorComponentBase::Deactivate();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    void EditorScriptCanvasComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (!m_configuration.HasSource())
        {
            return;
        }

        if (auto buildVariableOptional = m_configuration.CompileLatest())
        {
            auto runtimeComponent = gameEntity->CreateComponent<ScriptCanvas::RuntimeComponent>();
            runtimeComponent->TakeRuntimeDataOverrides(ConvertToRuntime(*buildVariableOptional));
        }
        else
        {
            AZ_Error("ScriptCanvasBuilder", false, "Runtime information did not build for ScriptCanvas Component using source: %s"
                , m_configuration.GetSource().ToString().c_str());
        }
    }

    void EditorScriptCanvasComponent::Reflect(AZ::ReflectContext* context)
    {
        using namespace EditorScriptCanvasComponentCpp;
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorScriptCanvasComponent, EditorComponentBase>()
                ->Version(Version::Current, &EditorScriptCanvasComponentVersionConverter)
                ->Field("configuration", &EditorScriptCanvasComponent::m_configuration)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorScriptCanvasComponent>("Script Canvas", "The Script Canvas component allows you to add a Script Canvas asset to a component, and have it execute on the specified entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }

        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<AZ::EditorScriptCanvasComponentSerializer>()->HandlesType<EditorScriptCanvasComponent>();
        }
    }

    void EditorScriptCanvasComponent::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
    {
        m_configuration.Refresh(SourceHandle(nullptr, assetId.m_guid, {}));
    }
}
