/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Libraries.h>
#include "Logic.h"
#include <ScriptCanvas/Core/Attributes.h>

namespace ScriptCanvas
{
    namespace Library
    {
        void Logic::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Logic, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Logic>("Logic", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Logic.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".logic")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "LogicNodeTitlePalette")
                        ;
                }

            }

            Nodes::Logic::WeightedRandomSequencer::ReflectDataTypes(reflection);
        }

        void Logic::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Logic;
            AddNodeToRegistry<Logic, And>(nodeRegistry);
            AddNodeToRegistry<Logic, Not>(nodeRegistry);
            AddNodeToRegistry<Logic, Or>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Logic::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Logic::And::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Not::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Or::CreateDescriptor(),
                });
        }
    }
}
