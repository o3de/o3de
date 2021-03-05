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
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/All.png")
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
} // namespace ScriptCanvasEditor
