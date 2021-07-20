/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Spawning/Spawning.h>

namespace ScriptCanvas
{
    namespace Library
    {
        void Spawning::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Spawning, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Spawning>("Spawning", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Entity.png");
                }
            }
        }

        void Spawning::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            AddNodeToRegistry<Spawning, ScriptCanvas::Nodes::SpawnNodeableNode>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Spawning::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::SpawnNodeableNode::CreateDescriptor(),
                });
        }
    }
}
