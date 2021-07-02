/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <StartingPointInput_precompiled.h>
#include <InputLibrary.h>

#include <AzCore/Serialization/EditContext.h>

// script canvas
#include <ScriptCanvas/Libraries/Libraries.h>

#include <InputHandlerNodeable.h>
#include <InputNode.h>


namespace StartingPointInput
{
    void InputLibrary::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<InputLibrary, LibraryDefinition>()
                ->Version(1)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<InputLibrary>("Input", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/InputConfig.png")
                    ;
            }
        }
    }

    void InputLibrary::InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry)
    {
        ScriptCanvas::Library::AddNodeToRegistry<InputLibrary, StartingPointInput::InputNode>(nodeRegistry);
        ScriptCanvas::Library::AddNodeToRegistry<InputLibrary, StartingPointInput::Nodes::InputHandlerNodeableNode>(nodeRegistry);
    }

    AZStd::vector<AZ::ComponentDescriptor*> InputLibrary::GetComponentDescriptors()
    {
        return AZStd::vector<AZ::ComponentDescriptor*>({
            StartingPointInput::InputNode::CreateDescriptor(),
            StartingPointInput::Nodes::InputHandlerNodeableNode::CreateDescriptor(),
        });
    }

} // namespace Input
