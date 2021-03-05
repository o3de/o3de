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
#include "DebugLibraryDefinition.h"
#include "Log.h"
#include "DrawText.h"
#include <ScriptCanvas/Libraries/Libraries.h>

namespace ScriptCanvas
{
    namespace Libraries
    {
        void Debug::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Debug, Library::LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Debug>("Debug", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Debug.png")
                        ;
                }
            }
        }

        void Debug::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            Library::AddNodeToRegistry<Debug, Nodes::Debug::DrawTextNode>(nodeRegistry);
            Library::AddNodeToRegistry<Debug, Nodes::Debug::Log>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Debug::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                Nodes::Debug::DrawTextNode::CreateDescriptor(),
                Nodes::Debug::Log::CreateDescriptor(),
            });
        }
    }
}
