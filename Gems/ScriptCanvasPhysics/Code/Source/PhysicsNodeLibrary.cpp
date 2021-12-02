/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysicsNodeLibrary.h"
#include "WorldNodes.h"

#include <AzCore/Serialization/EditContext.h>
#include <ScriptCanvas/Libraries/Libraries.h>

namespace ScriptCanvasPhysics
{
    void PhysicsNodeLibrary::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<PhysicsNodeLibrary, LibraryDefinition>()
                ->Version(0)
            ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PhysicsNodeLibrary>("Physics", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Script::Attributes::Category, "Physics")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            SCRIPT_CANVAS_GENERICS_TO_VM(WorldNodes::Registrar, WorldNodes::World, behaviorContext, WorldNodes::k_categoryName);
        }
    }

    void PhysicsNodeLibrary::InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry)
    {
        WorldNodes::Registrar::AddToRegistry<PhysicsNodeLibrary>(nodeRegistry);
    }

    AZStd::vector<AZ::ComponentDescriptor*> PhysicsNodeLibrary::GetComponentDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> descriptors;

        WorldNodes::Registrar::AddDescriptors(descriptors);

        return descriptors;
    }
} // namespace ScriptCanvasPhysics
