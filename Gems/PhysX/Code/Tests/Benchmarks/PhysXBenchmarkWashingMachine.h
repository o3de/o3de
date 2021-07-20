/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#ifdef HAVE_BENCHMARK

#include <AzCore/Math/Random.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AzPhysics
{
    class Scene;
}

namespace PhysX::Benchmarks
{
    //! Helper to create a cylinder and place a spinning 'blade' inside
    class WashingMachine
    {
    public:
        WashingMachine();
        ~WashingMachine();

        //! Create the washing machine.
        //! @param sceneHandle A handle to the physics scene to create the washing machine in.
        //! @param cylinderRadius Inside radius of the cylinder.
        //! @param cylinderHeight How tall to make the cylinder.
        //! @param position Where to position the cylinder.
        //! @param RPM How fast to spin the 'blade' in rotations per minute.
        void SetupWashingMachine(AzPhysics::SceneHandle sceneHandle, float cylinderRadius, float cylinderHeight,
            const AZ::Vector3& position, float RPM);

        //! Clean up the machine.
        void TearDownWashingMachine();

    private:
        void UpdateBlade(float fixedDeltaTime);

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
        AZStd::array<AzPhysics::SimulatedBodyHandle, NumCylinderSide> m_cylinder;
        AzPhysics::SimulatedBodyHandle m_blade;
        AzPhysics::SceneHandle m_sceneHandle = AzPhysics::InvalidSceneHandle;

        BladeAnimation m_bladeAnimation;

        AzPhysics::SceneEvents::OnSceneSimulationStartHandler m_sceneStartSimHandler;
    };
} // namespace PhysX::Benchmarks

#endif //HAVE_BENCHMARK
