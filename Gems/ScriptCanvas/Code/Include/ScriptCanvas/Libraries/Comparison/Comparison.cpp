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

#include <Libraries/Libraries.h>

#include "Comparison.h"

#include <ScriptCanvas/Core/Attributes.h>

namespace ScriptCanvas
{
    namespace Library
    {
        void Comparison::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Comparison, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Comparison>("Comparisons", "Provides mathematical equality operations")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Category, "Math/Comparisons")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/Logic.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".comparison")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "ComparisonNodeTitlePalette")
                        ;
                }

            }
        }

        void Comparison::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Comparison;
            AddNodeToRegistry<Comparison, EqualTo>(nodeRegistry);
            AddNodeToRegistry<Comparison, NotEqualTo>(nodeRegistry);
            AddNodeToRegistry<Comparison, Less>(nodeRegistry);
            AddNodeToRegistry<Comparison, Greater>(nodeRegistry);
            AddNodeToRegistry<Comparison, LessEqual>(nodeRegistry);
            AddNodeToRegistry<Comparison, GreaterEqual>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Comparison::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Comparison::EqualTo::CreateDescriptor(),
                ScriptCanvas::Nodes::Comparison::NotEqualTo::CreateDescriptor(),
                ScriptCanvas::Nodes::Comparison::Less::CreateDescriptor(),
                ScriptCanvas::Nodes::Comparison::Greater::CreateDescriptor(),
                ScriptCanvas::Nodes::Comparison::LessEqual::CreateDescriptor(),
                ScriptCanvas::Nodes::Comparison::GreaterEqual::CreateDescriptor(),
            });
        }
    } // namespace Library
} // namespace ScriptCanvas


