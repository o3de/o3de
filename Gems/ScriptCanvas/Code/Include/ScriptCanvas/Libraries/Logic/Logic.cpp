/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            Nodes::Logic::WeightedRandomSequencer::ReflectDataTypes(reflection);

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
        }

        void Logic::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Logic;
            AddNodeToRegistry<Logic, And>(nodeRegistry);
            AddNodeToRegistry<Logic, Any>(nodeRegistry);
            AddNodeToRegistry<Logic, Boolean>(nodeRegistry);
            AddNodeToRegistry<Logic, Break>(nodeRegistry);
            AddNodeToRegistry<Logic, Cycle>(nodeRegistry);
            AddNodeToRegistry<Logic, Gate>(nodeRegistry);
            AddNodeToRegistry<Logic, Indexer>(nodeRegistry);
            AddNodeToRegistry<Logic, IsNull>(nodeRegistry);
            AddNodeToRegistry<Logic, Multiplexer>(nodeRegistry);
            AddNodeToRegistry<Logic, Not>(nodeRegistry);
            AddNodeToRegistry<Logic, Once>(nodeRegistry);
            AddNodeToRegistry<Logic, Or>(nodeRegistry);
            AddNodeToRegistry<Logic, OrderedSequencer>(nodeRegistry);
            AddNodeToRegistry<Logic, Sequencer>(nodeRegistry);            
            AddNodeToRegistry<Logic, TargetedSequencer>(nodeRegistry);
            AddNodeToRegistry<Logic, WeightedRandomSequencer>(nodeRegistry);
            AddNodeToRegistry<Logic, While>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Logic::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Logic::And::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Any::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Boolean::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Break::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Cycle::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Gate::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Indexer::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::IsNull::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Multiplexer::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Not::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Once::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Or::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::OrderedSequencer::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::Sequencer::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::TargetedSequencer::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::WeightedRandomSequencer::CreateDescriptor(),
                ScriptCanvas::Nodes::Logic::While::CreateDescriptor(),
            });
        }
    }
}
