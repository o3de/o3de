/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>

#include <Atom/RPI.Public/PerformanceCollectionNotificationBus.h>

AZ_INSTANTIATE_EBUS_SINGLE_ADDRESS(ATOM_RPI_PUBLIC_API, AZ::RPI::PerformaceCollectionNotification)

namespace AZ::RPI
{
    /**
     * Behavior Context forwarder
     */
    class PerformaceCollectionBehaviorHandler
        : public PerformaceCollectionNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            PerformaceCollectionBehaviorHandler, "{61464725-BDE4-465B-96BA-0409D32E29A9}",
            AZ::SystemAllocator,
            OnPerformanceCollectionJobFinished
            );

        void OnPerformanceCollectionJobFinished(const AZStd::string& outputfilePath) override
        {
            Call(FN_OnPerformanceCollectionJobFinished, outputfilePath);
        }
    };

    void PerformaceCollectionNotification::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PerformaceCollectionNotificationBus>("RPIPerformaceCollectionNotificationBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "RPI")
                ->Attribute(AZ::Script::Attributes::Module, "rpi")
                ->Handler<PerformaceCollectionBehaviorHandler>();
        }
    }
} // namespace AZ::RPI
