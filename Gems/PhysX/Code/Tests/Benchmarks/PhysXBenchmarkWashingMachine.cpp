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

#ifdef HAVE_BENCHMARK
#include <Benchmarks/PhysXBenchmarkWashingMachine.h>

#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/SimulatedBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>

namespace PhysX::Benchmarks
{
    float NormalizeAngle(float angle)
    {
        if (angle > AZ::Constants::Pi)
        {
            angle -= AZ::Constants::TwoPi;
        }
        else if (angle < -AZ::Constants::Pi)
        {
            angle += AZ::Constants::TwoPi;
        }
        return angle;
    }

    void WashingMachine::BladeAnimation::Init(float RPM)
    {
        m_angularPosition = 0.0f;
        m_angularVelocity = (RPM / 60.0f) * AZ::Constants::TwoPi; //radians/sec
    }

    AZ::Quaternion WashingMachine::BladeAnimation::StepAnimation(float deltaTime)
    {
        m_angularPosition = NormalizeAngle(m_angularPosition + m_angularVelocity * deltaTime);

        return AZ::Quaternion::CreateRotationZ(m_angularPosition);
    }

    WashingMachine::WashingMachine()
        : m_sceneStartSimHandler([this]([[maybe_unused]]AzPhysics::SceneHandle sceneHandle, float fixedDeltaTime)
            {
                this->UpdateBlade(fixedDeltaTime);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Animation))
    {

    }

    WashingMachine::~WashingMachine()
    {
        if (m_sceneHandle != AzPhysics::InvalidSceneHandle)
        {
            TearDownWashingMachine();
        }
    }

    void WashingMachine::SetupWashingMachine(AzPhysics::SceneHandle sceneHandle, float cylinderRadius, float cylinderHeight,
        const AZ::Vector3& position, float RPM)
    {
        Physics::System* system = AZ::Interface<Physics::System>::Get();
        if (system == nullptr)
        {
            return;
        }
        AzPhysics::Scene* scene = nullptr;
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            scene = physicsSystem->GetScene(sceneHandle);
            m_sceneHandle = sceneHandle; //cache the handle
        }

        scene->RegisterSceneSimulationStartHandler(m_sceneStartSimHandler);

        //create the cylinder
        const float cylinderWallThickness = AZ::GetMin(25.0f, cylinderRadius);
        const float halfCylinderWallThickness = cylinderWallThickness / 2.0f;
        const float z = position.GetZ() + (cylinderHeight / 2.0f);
        const float cylinderTheta = (AZ::Constants::TwoPi / NumCylinderSide);
        for (int i = 0; i < NumCylinderSide; i++)
        {
            AzPhysics::StaticRigidBodyConfiguration config;
            config.m_position.SetX((cylinderRadius + halfCylinderWallThickness) * std::cos(AZ::Constants::TwoPi * i / NumCylinderSide) + position.GetX());
            config.m_position.SetY((cylinderRadius + halfCylinderWallThickness) * std::sin(AZ::Constants::TwoPi * i / NumCylinderSide) + position.GetY());
            config.m_position.SetZ(z);
            config.m_orientation = AZ::Quaternion::CreateRotationZ(AZ::Constants::HalfPi + (cylinderTheta * i));
            Physics::ColliderConfiguration colliderConfig;
            Physics::BoxShapeConfiguration shapeConfiguration(AZ::Vector3(cylinderRadius, cylinderWallThickness, cylinderHeight));
            config.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(&colliderConfig, &shapeConfiguration);
            m_cylinder[i] = scene->AddSimulatedBody(&config);
        }

        //create the prop
        const float bladeLength = cylinderRadius * 2.0f;
        const float bladeHeight = cylinderHeight * 0.75f;

        m_bladeAnimation.Init(RPM);

        AzPhysics::RigidBodyConfiguration bladeRigidBodyConfig;
        bladeRigidBodyConfig.m_kinematic = true;
        bladeRigidBodyConfig.m_mass = 1000.0f;
        bladeRigidBodyConfig.m_position = position;
        bladeRigidBodyConfig.m_position.SetZ(position.GetZ() + (bladeHeight / 2.0f));
        bladeRigidBodyConfig.m_orientation = AZ::Quaternion::CreateRotationZ(0.0f);
        Physics::ColliderConfiguration bladeColliderConfig;
        Physics::BoxShapeConfiguration bladeShapeConfiguration(AZ::Vector3(bladeLength, 1.0f, bladeHeight));
        bladeRigidBodyConfig.m_colliderAndShapeData = AZStd::make_pair(&bladeColliderConfig, &bladeShapeConfiguration);
        m_blade = scene->AddSimulatedBody(&bladeRigidBodyConfig);
    }

    void WashingMachine::TearDownWashingMachine()
    {
        m_sceneStartSimHandler.Disconnect();

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            for (int i = 0; i < NumCylinderSide; i++)
            {
                sceneInterface->RemoveSimulatedBody(m_sceneHandle, m_cylinder[i]);
                m_cylinder[i] = AzPhysics::InvalidSimulatedBodyHandle;
            }
            sceneInterface->RemoveSimulatedBody(m_sceneHandle, m_blade);
            m_blade = AzPhysics::InvalidSimulatedBodyHandle;
        }
        m_sceneHandle = AzPhysics::InvalidSceneHandle;
    }

    void WashingMachine::UpdateBlade(float fixedDeltaTime)
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AZ::Quaternion newRot = m_bladeAnimation.StepAnimation(fixedDeltaTime);

            if(auto* bladeBody = azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(m_sceneHandle, m_blade)))
            {
                AZ::Transform transform = bladeBody->GetTransform();
                transform.SetRotation(newRot);
                bladeBody->SetKinematicTarget(transform);
            }
        }
    }
}
#endif //HAVE_BENCHMARK
