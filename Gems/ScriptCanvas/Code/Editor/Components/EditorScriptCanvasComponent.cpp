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
        using namespace AzToolsFramework;

        EditorComponentBase::Activate();
        AzToolsFramework::EditorEntityContextNotificationBus::Handler::BusConnect();
        m_handlerSourceCompiled = m_configuration.ConnectToSourceCompiled([this](const Configuration&)
            {
                this->InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
            });

        m_configuration.Refresh();
        InvalidatePropertyDisplay(AzToolsFramework::Refresh_EntireTree);
    }

    void EditorScriptCanvasComponent::Deactivate()
    {
        m_handlerSourceCompiled.Disconnect();
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
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorScriptCanvasComponent, EditorComponentBase>()
                ->Version(Version::Current, &Deprecated::EditorScriptCanvasComponentVersionConverter::Convert)
                ->Field("configuration", &EditorScriptCanvasComponent::m_configuration)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorScriptCanvasComponent>("Script Canvas", "The Script Canvas component allows you to add a Script Canvas asset to a component, and have it execute on the specified entity.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/ScriptCanvas.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/ScriptCanvas/Viewport/ScriptCanvas.svg")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Level"))
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/scripting/script-canvas/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorScriptCanvasComponent::m_configuration, "Configuration", "Script Selection and Property Overrides")
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
        m_configuration.Refresh(SourceHandle(nullptr, assetId.m_guid));
    }
}
