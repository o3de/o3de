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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/WorldBodyBus.h>
#include <PhysX/ComponentTypeIds.h>

namespace PhysX
{
    class RigidBodyStatic;

    class StaticRigidBodyComponent final
        : public AZ::Component
        , public Physics::WorldBodyRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(StaticRigidBodyComponent, StaticRigidBodyComponentTypeId);

        StaticRigidBodyComponent();
        ~StaticRigidBodyComponent();

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        PhysX::RigidBodyStatic* GetStaticRigidBody();

        // WorldBodyRequestBus
        void EnablePhysics() override;
        void DisablePhysics() override;
        bool IsPhysicsEnabled() const override;
        AZ::Aabb GetAabb() const override;
        Physics::WorldBody* GetWorldBody() override;
        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override;

    private:
        void InitStaticRigidBody();

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AZ::TransformNotificationsBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

        AZStd::unique_ptr<PhysX::RigidBodyStatic> m_staticRigidBody;
    };
} // namespace PhysX
