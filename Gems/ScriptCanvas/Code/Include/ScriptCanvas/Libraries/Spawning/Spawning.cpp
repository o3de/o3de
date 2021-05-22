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
