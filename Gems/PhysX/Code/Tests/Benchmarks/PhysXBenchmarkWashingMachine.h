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

#ifdef HAVE_BENCHMARK

#include <AzCore/Math/Random.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/World.h>

namespace AzPhysics
{
    class Scene;
}

namespace PhysX::Benchmarks
{
    //! Helper to create a cylinder and place a spinning 'blade' inside
    class WashingMachine
        : public Physics::WorldNotificationBus::Handler
    {
    public:
        ~WashingMachine();

        //! Create the washing machine.
        //! @param scene - the physics scene to create the washing machine in.
        //! @param cylinderRadius - inside radius of the cylinder.
        //! @param cylinderHeight - how tall to make the cylinder.
        //! @param position - where to position the cylinder.
        //! @param RPM - how fast to spin the 'blade' in rotations per minute.
        void SetupWashingMachine(AzPhysics::Scene* scene, float cylinderRadius, float cylinderHeight,
            const AZ::Vector3& position, float RPM);

        //! Clean up the machine.
        void TearDownWashingMachine();

        // Physics::WorldNotificationBus::Handler Interface ----------
        void OnPrePhysicsSubtick(float fixedDeltaTime) override;
        // Physics::WorldNotificationBus::Handler Interface ----------
    private:
        //! Helper to animate the 'blade'.
        struct BladeAnimation
        {
            //! Initialize the animation at the given RPM.
            //! @param RPM - how fast to spin the 'blade' in rotations per minute.
            void Init(float RPM);

            //! Step the animation by the given time and return a quaternion of the new rotation.
            //! @param deltaTime - time since last step of the animation.
            //! @return AZ::Quaternion - new rotation after updating.
            AZ::Quaternion StepAnimation(float deltaTime);
        private:
            float m_angularVelocity;
            float m_angularPosition;
        };

        static const int NumCylinderSide = 12;
        AZStd::array<AZStd::unique_ptr<Physics::RigidBodyStatic>, NumCylinderSide> m_cylinder;
        AZStd::unique_ptr<Physics::RigidBody> m_blade;

        BladeAnimation m_bladeAnimation;
    };
} // namespace PhysX::Benchmarks

#endif //HAVE_BENCHMARK
