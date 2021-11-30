/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GradientSignal/SmoothStep.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <GradientSignal/Util.h>
#include <GradientSignal/Ebuses/SmoothStepRequestBus.h>

namespace GradientSignal
{
    void SmoothStep::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<SmoothStep>()
                ->Version(0)
                ->Field("FalloffMidpoint", &SmoothStep::m_falloffMidpoint)
                ->Field("FalloffRange", &SmoothStep::m_falloffRange)
                ->Field("FalloffStrength", &SmoothStep::m_falloffStrength)
                ;

            AZ::EditContext* edit = serialize->GetEditContext();
            if (edit)
            {
                edit->Class<SmoothStep>("Smooth Step Gradient", "Smooth Step Gradient")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SmoothStep::m_falloffMidpoint, "Falloff Midpoint", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SmoothStep::m_falloffRange, "Falloff Range", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &SmoothStep::m_falloffStrength, "Falloff Softness", "")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                    ;
            }
        }
    
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SmoothStep>()
                ->Constructor()
                ->Property("falloffMidpoint", BehaviorValueProperty(&SmoothStep::m_falloffMidpoint))
                ->Property("falloffRange", BehaviorValueProperty(&SmoothStep::m_falloffRange))
                ->Property("falloffStrength", BehaviorValueProperty(&SmoothStep::m_falloffStrength))
                ;

            behaviorContext->EBus<SmoothStepRequestBus>("SmoothStepRequestBus")
                ->Attribute(AZ::Script::Attributes::Category, "Vegetation")
                ->Event("GetFallOffMidpoint", &SmoothStepRequestBus::Events::GetFallOffMidpoint)
                ->Event("SetFallOffMidpoint", &SmoothStepRequestBus::Events::SetFallOffMidpoint)
                ->VirtualProperty("FallOffMidpoint", "GetFallOffMidpoint", "SetFallOffMidpoint")
                ->Event("GetFallOffRange", &SmoothStepRequestBus::Events::GetFallOffRange)
                ->Event("SetFallOffRange", &SmoothStepRequestBus::Events::SetFallOffRange)
                ->VirtualProperty("FallOffRange", "GetFallOffRange", "SetFallOffRange")
                ->Event("GetFallOffStrength", &SmoothStepRequestBus::Events::GetFallOffStrength)
                ->Event("SetFallOffStrength", &SmoothStepRequestBus::Events::SetFallOffStrength)
                ->VirtualProperty("FallOffStrength", "GetFallOffStrength", "SetFallOffStrength")
                ;
        }
    }
}
