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

#include <AzCore/Component/ComponentBus.h>
#include <AzFramework/Physics/ColliderComponentBus.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Shape.h>

#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>

namespace PhysX
{
    class Shape;

    //! Messages serviced by a PhysX collider component.
    //! A PhysX collider component allows collision geometry to be attached to bodies in PhysX.
    class ColliderComponentRequests
        : public AZ::ComponentBus
    {
    public:
        //! Gets the collection of collider configuration / shape configuration pairs used to define the collider's shapes.
        virtual Physics::ShapeConfigurationList GetShapeConfigurations() = 0;

        //! Gets the collection of physics shapes associated with the collider.
        virtual AZStd::vector<AZStd::shared_ptr<Physics::Shape>> GetShapes() = 0;
    };

    using ColliderComponentRequestBus = AZ::EBus<ColliderComponentRequests>;

    //! Allows to supply rigid/world body configuration to [Static]RigidBodyComponent without invoking relevant constructor,
    //! e. g. when attaching component from another gem.
    class BodyConfigurationComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual AzPhysics::RigidBodyConfiguration GetRigidBodyConfiguration() = 0;
        virtual AzPhysics::SimulatedBodyConfiguration GetSimulatedBodyConfiguration() = 0;
    };

    using BodyConfigurationComponentBus = AZ::EBus<BodyConfigurationComponentRequests>;
} // namespace PhysX
