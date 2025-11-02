/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleComponent.h"

#include <Atom/RPI.Public/Scene.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <OpenParticleSystem/ParticleFeatureProcessor.h>

namespace OpenParticle
{

    ParticleComponent::ParticleComponent(const ParticleComponentConfig& config)
        : BaseClass(config)
    {
    }

    void ParticleComponent::Reflect(AZ::ReflectContext* context)
    {
        BaseClass::Reflect(context);
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ParticleComponent, BaseClass>()->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<ParticleRequestBus>("ParticleRequestBus")
                ->Attribute(AZ::Script::Attributes::Module, "OpenParticleSystem")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Event("Play", &ParticleRequestBus::Events::Play)
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Notify particle system to play")
                ->Event("Pause", &ParticleRequestBus::Events::Pause)
                    ->Attribute(AZ::Script::Attributes::ToolTip,  "Notify particle system to pause simulation and keep rendering")
                ->Event("Stop", &ParticleRequestBus::Events::Stop)
                    ->Attribute(AZ::Script::Attributes::ToolTip,  "Notify particle system to stop simulation and rendering")
                ->Event("SetVisible", &ParticleRequestBus::Events::SetVisibility)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Visibility modification no longer needed in game mode")
                ->Event("GetVisible", &ParticleRequestBus::Events::GetVisibility)
                    ->Attribute(AZ::Script::Attributes::Deprecated, true)
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                    ->Attribute(AZ::Script::Attributes::ToolTip, "Visibility modification no longer needed in game mode")
                ->VirtualProperty("Visible", "GetVisible", "SetVisible");

            behaviorContext->Class<ParticleComponent>()->RequestBus("ParticleRequestBus");
        }
    }
} // namespace OpenParticle
