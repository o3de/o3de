/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Internal/Nodes/StringFormatted.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/String/Format.h>
#include <ScriptCanvas/Libraries/String/Print.h>

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
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/String.png")
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
        }

        AZStd::vector<AZ::ComponentDescriptor*> String::GetComponentDescriptors()
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors = {
                ScriptCanvas::Nodes::String::Format::CreateDescriptor(),
                ScriptCanvas::Nodes::String::Print::CreateDescriptor()
            };

            return descriptors;
        }
    }
}
