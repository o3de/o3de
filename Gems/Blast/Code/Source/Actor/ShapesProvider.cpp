/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StdAfx.h"

#include <Actor/ShapesProvider.h>

namespace Blast
{
    ShapesProvider::ShapesProvider(AZ::EntityId entityId, AzPhysics::RigidBodyConfiguration configuration)
        : m_entityId(entityId)
        , m_configuration(configuration)
    {
        PhysX::ColliderComponentRequestBus::Handler::BusConnect(m_entityId);
    }

    ShapesProvider::~ShapesProvider()
    {
        PhysX::ColliderComponentRequestBus::Handler::BusDisconnect(m_entityId);
    }

    AzPhysics::ShapeColliderPairList ShapesProvider::GetShapeConfigurations()
    {
        return {};
    }

    AZStd::vector<AZStd::shared_ptr<Physics::Shape>> ShapesProvider::GetShapes()
    {
        return m_shapes;
    }

    void ShapesProvider::AddShape(const AZStd::shared_ptr<Physics::Shape> shape)
    {
        m_shapes.push_back(shape);
    }

    AzPhysics::RigidBodyConfiguration ShapesProvider::GetRigidBodyConfiguration()
    {
        return m_configuration;
    }

    AzPhysics::SimulatedBodyConfiguration ShapesProvider::GetSimulatedBodyConfiguration()
    {
        return static_cast<AzPhysics::SimulatedBodyConfiguration>(m_configuration);
    }
} // namespace Blast
