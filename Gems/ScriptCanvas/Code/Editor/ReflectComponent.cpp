/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ReflectComponent.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Editor/Assets/ScriptCanvasAssetHolder.h>

#include <Editor/View/Dialogs/SettingsDialog.h>
#include <Editor/View/Widgets/LoggingPanel/LiveWindowSession/LiveLoggingWindowSession.h>

#include <Editor/View/Widgets/NodePalette/CreateNodeMimeEvent.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/EBusNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/GeneralNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/SpecializedNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/NodePalette/FunctionNodePaletteTreeItemTypes.h>

#include <ScriptCanvas/Bus/UndoBus.h>

namespace ScriptCanvasEditor
{
    void ReflectComponent::Reflect(AZ::ReflectContext* context)
    {
        SourceHandle::Reflect(context);
        ScriptCanvas::ScriptCanvasData::Reflect(context);
        ScriptCanvasAssetHolder::Reflect(context);
        EditorSettings::EditorWorkspace::Reflect(context);        
        EditorSettings::ScriptCanvasEditorSettings::Reflect(context);        
        LiveLoggingUserSettings::Reflect(context);
        UndoData::Reflect(context);

        // Base Mime Event
        CreateNodeMimeEvent::Reflect(context);
        SpecializedCreateNodeMimeEvent::Reflect(context);
        MultiCreateNodeMimeEvent::Reflect(context);

        // Specific Mime Event Implementations
        CreateClassMethodMimeEvent::Reflect(context);
        CreateGlobalMethodMimeEvent::Reflect(context);
        CreateNodeGroupMimeEvent::Reflect(context);
        CreateCommentNodeMimeEvent::Reflect(context);
        CreateCustomNodeMimeEvent::Reflect(context);
        CreateEBusHandlerMimeEvent::Reflect(context);
        CreateEBusHandlerEventMimeEvent::Reflect(context);
        CreateEBusSenderMimeEvent::Reflect(context);
        CreateGetVariableNodeMimeEvent::Reflect(context);
        CreateSetVariableNodeMimeEvent::Reflect(context);
        CreateVariableChangedNodeMimeEvent::Reflect(context);
        CreateVariableSpecificNodeMimeEvent::Reflect(context);
        CreateFunctionMimeEvent::Reflect(context);

        // Script Events
        CreateScriptEventsHandlerMimeEvent::Reflect(context);
        CreateScriptEventsReceiverMimeEvent::Reflect(context);
        CreateScriptEventsSenderMimeEvent::Reflect(context);
        CreateSendOrReceiveScriptEventsMimeEvent::Reflect(context);
        
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ReflectComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ReflectComponent>("Script Canvas Reflections", "Script Canvas Reflect Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ;
            }
        }
    }

    void ReflectComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ScriptCanvasReflectService", 0xb3bfe139));
    }

    void ReflectComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ScriptCanvasReflectService", 0xb3bfe139));
    }

    void ReflectComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
    }

    void ReflectComponent::Activate()
    {
    }

    void ReflectComponent::Deactivate()
    {
    }
}
