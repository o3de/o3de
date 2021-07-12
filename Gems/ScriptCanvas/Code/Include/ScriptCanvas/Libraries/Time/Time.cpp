/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Libraries.h>

#include "Time.h"
#include <ScriptCanvas/Core/Attributes.h>

#include <ScriptCanvas/Internal/Nodeables/BaseTimer.h>
#include <ScriptCanvas/Internal/Nodes/BaseTimerNode.h>

namespace ScriptCanvas
{
    namespace Library
    {
        void Time::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Time, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Time>("Timing", "Time related operations.")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Math.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".time")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TimeNodeTitlePalette")
                        ;
                }
            }

            ScriptCanvas::Nodeables::Time::BaseTimer::Reflect(reflection);
            ScriptCanvas::Nodes::Internal::BaseTimerNode::Reflect(reflection);
        }

        void Time::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::Time::Countdown>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::Time::Duration>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::Time::HeartBeat>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::Time::TickDelay>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::Time::TimeDelay>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::Time::Timer>(nodeRegistry);

            AddNodeToRegistry<Time, ScriptCanvas::Nodes::DelayNodeableNode>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::DurationNodeableNode>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::HeartBeatNodeableNode>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::TimeDelayNodeableNode>(nodeRegistry);
            AddNodeToRegistry<Time, ScriptCanvas::Nodes::TimerNodeableNode>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Time::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Time::Countdown::CreateDescriptor(),
                ScriptCanvas::Nodes::Time::TickDelay::CreateDescriptor(),
                ScriptCanvas::Nodes::Time::TimeDelay::CreateDescriptor(),
                ScriptCanvas::Nodes::Time::Duration::CreateDescriptor(),
                ScriptCanvas::Nodes::Time::HeartBeat::CreateDescriptor(),
                ScriptCanvas::Nodes::Time::Timer::CreateDescriptor(),

                ScriptCanvas::Nodes::DelayNodeableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::DurationNodeableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::HeartBeatNodeableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::TimeDelayNodeableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::TimerNodeableNode::CreateDescriptor(),
                });
        }
    }

}
