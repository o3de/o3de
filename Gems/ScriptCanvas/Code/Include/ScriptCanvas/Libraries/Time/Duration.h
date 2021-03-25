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

#pragma once

#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>

#include <AzCore/Component/TickBus.h>

#include <AzCore/RTTI/TypeInfo.h>

#include <Include/ScriptCanvas/Libraries/Time/Duration.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Time
        {
            //! Deprecated: see DurationNodeableNode
            class Duration 
                : public Node
                , AZ::TickBus::Handler
            {

            public:

                SCRIPTCANVAS_NODE(Duration);

                Duration();

            protected:

                float m_durationSeconds;
                float m_elapsedTime;

                //! Internal counter to track time elapsed
                float m_currentTime;

                void OnInputSignal(const SlotId&) override;
            
            protected:

                void OnDeactivate() override;
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

            };
        }
    }
}
