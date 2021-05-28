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

#include "NodeableTestingLibrary.h"

#include "SharedDataSlotExample.h"
#include "ValuePointerReferenceExample.h"

#include <AzCore/Serialization/EditContext.h>
#include "Source/Nodes/Nodeables/SharedDataSlotExample.generated.h"

namespace ScriptCanvasTesting
{
    void NodeableTestingLibrary::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<NodeableTestingLibrary, ScriptCanvas::Library::LibraryDefinition>()
                ->Version(1);

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<NodeableTestingLibrary>("Nodeable Testing", "");
            }
        }
    }

    void NodeableTestingLibrary::InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry)
    {
        ScriptCanvas::Library::AddNodeToRegistry<NodeableTestingLibrary, ::Nodes::InputMethodSharedDataSlotExampleNode>(nodeRegistry);
        ScriptCanvas::Library::AddNodeToRegistry<NodeableTestingLibrary, ::Nodes::BranchMethodSharedDataSlotExampleNode>(nodeRegistry);

        ScriptCanvas::Library::AddNodeToRegistry<NodeableTestingLibrary, ::Nodes::ReturnTypeExampleNode>(nodeRegistry);
        ScriptCanvas::Library::AddNodeToRegistry<NodeableTestingLibrary, ::Nodes::InputTypeExampleNode>(nodeRegistry);
        ScriptCanvas::Library::AddNodeToRegistry<NodeableTestingLibrary, ::Nodes::BranchInputTypeExampleNode>(nodeRegistry);
        ScriptCanvas::Library::AddNodeToRegistry<NodeableTestingLibrary, ::Nodes::PropertyExampleNode>(nodeRegistry);
    }

    AZStd::vector<AZ::ComponentDescriptor*> NodeableTestingLibrary::GetComponentDescriptors()
    {
        return AZStd::vector<AZ::ComponentDescriptor*>({
            ::Nodes::InputMethodSharedDataSlotExampleNode::CreateDescriptor(),
            ::Nodes::BranchMethodSharedDataSlotExampleNode::CreateDescriptor(),
            ::Nodes::ReturnTypeExampleNode::CreateDescriptor(),
            ::Nodes::InputTypeExampleNode::CreateDescriptor(),
            ::Nodes::BranchInputTypeExampleNode::CreateDescriptor(),
            ::Nodes::PropertyExampleNode::CreateDescriptor()
            });
    }
}
