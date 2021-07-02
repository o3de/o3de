/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <AzCore/Serialization/EditContext.h>

#include <Editor/Nodes/EditorLibrary.h>
#include <Editor/Nodes/ScriptCanvasAssetNode.h>


namespace ScriptCanvasEditor
{
    namespace Library
    {
        void Editor::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Editor, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Editor>("Editor", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/All.png")
                        ;
                }
            }
        }

        void Editor::InitNodeRegistry(ScriptCanvas::NodeRegistry& nodeRegistry)
        {
            ScriptCanvas::Library::AddNodeToRegistry<Editor, ScriptCanvasAssetNode>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Editor::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvasAssetNode::CreateDescriptor(),
            });
        }
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors()
    {
        return Library::Editor::GetComponentDescriptors();
    }
}
