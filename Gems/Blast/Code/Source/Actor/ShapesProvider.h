/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <PhysX/ColliderComponentBus.h>

namespace Blast
{
    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
    class ShapesProvider
        : public PhysX::ColliderComponentRequestBus::Handler
        , public PhysX::BodyConfigurationComponentBus::Handler
    {
    public:
        ShapesProvider(AZ::EntityId entityId, AzPhysics::RigidBodyConfiguration configuration);
        ~ShapesProvider();

        void AddShape(AZStd::shared_ptr<Physics::Shape> shape);

        // This class is not supposed to provide shape configurations, only shapes themselves.
        AzPhysics::ShapeColliderPairList GetShapeConfigurations() override;

        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> GetShapes() override;

        AzPhysics::RigidBodyConfiguration GetRigidBodyConfiguration() override;

        AzPhysics::SimulatedBodyConfiguration GetSimulatedBodyConfiguration() override;

    private:
        AZStd::vector<AZStd::shared_ptr<Physics::Shape>> m_shapes;
        AZ::EntityId m_entityId;
        AzPhysics::RigidBodyConfiguration m_configuration;
    };
    AZ_POP_DISABLE_WARNING
} // namespace Blast
