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

#include "Time.h"
#include <ScriptCanvas/Core/Attributes.h>

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
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/Math.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".time")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TimeNodeTitlePalette")
                        ;
                }
            }

            ScriptCanvas::Nodes::Internal::BaseTimerNode::Reflect(reflection);
        }

        void Time::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Time;
            AddNodeToRegistry<Time, Countdown>(nodeRegistry);
            AddNodeToRegistry<Time, Duration>(nodeRegistry);
            AddNodeToRegistry<Time, HeartBeat>(nodeRegistry);
            AddNodeToRegistry<Time, TickDelay>(nodeRegistry);
            AddNodeToRegistry<Time, TimeDelay>(nodeRegistry);
            AddNodeToRegistry<Time, Timer>(nodeRegistry);
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
            });
        }
    }

}