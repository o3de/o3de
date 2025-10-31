/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Profiler/GraphicsProfilerBus.h>
#include <RHI.Profiler/GraphicsProfilerSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>


namespace AZ::RHI
{
    void GraphicsProfilerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<GraphicsProfilerSystemComponent, AZ::Component>()->Version(0);
        }

        if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
        {
            behaviorContext->EBus<GraphicsProfilerBus>("GraphicsProfilerBus")
                ->Event("StartCapture", &GraphicsProfilerBus::Events::StartCapture)
                ->Event("EndCapture", &GraphicsProfilerBus::Events::EndCapture)
                ->Event("TriggerCapture", &GraphicsProfilerBus::Events::TriggerCapture);
        }
    }
}
