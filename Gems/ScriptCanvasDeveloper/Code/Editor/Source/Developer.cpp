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
#include <ScriptCanvasDeveloperEditor/Developer.h>
#include <ScriptCanvasDeveloperEditor/Mock.h>
#include <ScriptCanvasDeveloperEditor/WrapperMock.h>
#include <ScriptCanvas/Libraries/Libraries.h>

namespace ScriptCanvasDeveloper
{
    namespace Libraries
    {
        void Developer::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Developer, ScriptCanvas::Library::LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Developer>("Developer", "Library of Developer only nodes")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/ScriptCanvas.png")
                        ;
                }
            }
        }

        void Developer::InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry)
        {
            ScriptCanvas::Library::AddNodeToRegistry<Developer, Nodes::Mock>(nodeRegistry);
            ScriptCanvas::Library::AddNodeToRegistry<Developer, Nodes::WrapperMock>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Developer::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                Nodes::Mock::CreateDescriptor(),
                Nodes::WrapperMock::CreateDescriptor()
            });
        }
    }
}