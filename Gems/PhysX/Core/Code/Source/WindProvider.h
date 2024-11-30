/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Physics/WindBus.h>

namespace PhysX
{
    struct PhysXSystemConfiguration;

    //! Implementation of Physics::WindRequests EBus.
    //! Uses wind tag values to identify entities that serve as wind data providers and
    //! PhysX World Force Regions for wind velocity values.
    class WindProvider final
        : public AZ::Interface<Physics::WindRequests>::Registrar
        , private Physics::WindRequestsBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        WindProvider();
        ~WindProvider();

        // AZ::Interface<Physics::WindRequests>::Registrar
        AZ::Vector3 GetGlobalWind() const override;
        AZ::Vector3 GetWind(const AZ::Vector3& worldPosition) const override;
        AZ::Vector3 GetWind(const AZ::Aabb& aabb) const override;

    private:
        // AZ::TickBus::Handler
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        void CreateNewHandlers(const PhysXSystemConfiguration& configuration);

        void OnConfigurationChanged(const AzPhysics::SystemConfiguration* config);
        AzPhysics::SystemEvents::OnConfigurationChangedEvent::Handler m_physXConfigChangedHandler;

        class EntityGroupHandler;
        AZStd::unique_ptr<EntityGroupHandler> m_globalWindHandler;
        AZStd::unique_ptr<EntityGroupHandler> m_localWindHandler;
    };
} // namespace PhysX

