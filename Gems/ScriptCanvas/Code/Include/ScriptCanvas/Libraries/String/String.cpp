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

#include <ScriptCanvas/Internal/Nodes/StringFormatted.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Core/Attributes.h>

#include "String.h"

namespace ScriptCanvas
{
    namespace Library
    {
        void String::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<String, LibraryDefinition>()
                    ->Version(0)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<String>("String", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/String.png")
                        ->Attribute(AZ::Edit::Attributes::CategoryStyle, ".string")
                        ->Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "StringNodeTitlePalette")
                        ;
                }
            }

            Nodes::Internal::StringFormatted::Reflect(reflection);
        }

        void String::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::String;
            AddNodeToRegistry<String, Format>(nodeRegistry);
            AddNodeToRegistry<String, Print>(nodeRegistry);
            AddNodeToRegistry<String, Replace>(nodeRegistry);
            AddNodeToRegistry<String, Contains>(nodeRegistry);
            AddNodeToRegistry<String, StartsWith>(nodeRegistry);
            AddNodeToRegistry<String, EndsWith>(nodeRegistry);
            AddNodeToRegistry<String, Split>(nodeRegistry);
            AddNodeToRegistry<String, Join>(nodeRegistry);

            StringNodes::Registrar::AddToRegistry<String>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> String::GetComponentDescriptors()
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors = {
                ScriptCanvas::Nodes::String::Format::CreateDescriptor(),
                ScriptCanvas::Nodes::String::Print::CreateDescriptor(),
                ScriptCanvas::Nodes::String::Replace::CreateDescriptor(),
                ScriptCanvas::Nodes::String::Contains::CreateDescriptor(),
                ScriptCanvas::Nodes::String::StartsWith::CreateDescriptor(),
                ScriptCanvas::Nodes::String::EndsWith::CreateDescriptor(),
                ScriptCanvas::Nodes::String::Split::CreateDescriptor(),
                ScriptCanvas::Nodes::String::Join::CreateDescriptor()
            };

            StringNodes::Registrar::AddDescriptors(descriptors);

            return descriptors;
        }
    }
}
