/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "precompiled.h"

#include <Editor/ReflectComponent.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Editor/Assets/ScriptCanvasAssetHolder.h>
#include <Editor/Assets/ScriptCanvasAssetReference.h>
#include <Editor/Assets/ScriptCanvasAssetInstance.h>

#include <Editor/Nodes/EditorLibrary.h>

#include <Editor/View/Dialogs/Settings.h>
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
        ScriptCanvas::ScriptCanvasData::Reflect(context);
        ScriptCanvasAssetReference::Reflect(context);
        ScriptCanvasAssetInstance::Reflect(context);
        ScriptCanvasAssetHolder::Reflect(context);
        EditorSettings::EditorWorkspace::Reflect(context);        
        EditorSettings::ScriptCanvasEditorSettings::Reflect(context);        
        Library::Editor::Reflect(context);
        LiveLoggingUserSettings::Reflect(context);
        UndoData::Reflect(context);

        // Base Mime Event
        CreateNodeMimeEvent::Reflect(context);
        SpecializedCreateNodeMimeEvent::Reflect(context);
        MultiCreateNodeMimeEvent::Reflect(context);

        // Specific Mime Event Implementations
        CreateClassMethodMimeEvent::Reflect(context);
        CreateNodeGroupMimeEvent::Reflect(context);
        CreateCommentNodeMimeEvent::Reflect(context);
        CreateCustomNodeMimeEvent::Reflect(context);
        CreateEBusHandlerMimeEvent::Reflect(context);
        CreateEBusHandlerEventMimeEvent::Reflect(context);
        CreateEBusSenderMimeEvent::Reflect(context);
        CreateEntityRefNodeMimeEvent::Reflect(context);
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
