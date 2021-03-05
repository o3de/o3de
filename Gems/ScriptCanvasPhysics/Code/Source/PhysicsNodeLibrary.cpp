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

#include "ScriptCanvasPhysics_precompiled.h"

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
