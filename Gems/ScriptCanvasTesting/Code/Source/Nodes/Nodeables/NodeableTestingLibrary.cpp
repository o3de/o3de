/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
