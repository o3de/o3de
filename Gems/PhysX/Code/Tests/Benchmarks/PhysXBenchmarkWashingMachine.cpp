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
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Shape.h>

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

    WashingMachine::~WashingMachine()
    {
        TearDownWashingMachine();
    }

    void WashingMachine::SetupWashingMachine(AzPhysics::Scene* scene, float cylinderRadius, float cylinderHeight,
        const AZ::Vector3& position, float RPM)
    {
        Physics::System* system = AZ::Interface<Physics::System>::Get();
        if (system == nullptr)
        {
            return;
        }
        AZStd::shared_ptr<Physics::World> legacyWorld = scene->GetLegacyWorld();

        //create the cylinder
        const float cylinderWallThickness = AZ::GetMin(25.0f, cylinderRadius);
        const float halfCylinderWallThickness = cylinderWallThickness / 2.0f;
        const float z = position.GetZ() + (cylinderHeight / 2.0f);
        const float cylinderTheta = (AZ::Constants::TwoPi / NumCylinderSide);
        for (int i = 0; i < NumCylinderSide; i++)
        {
            Physics::WorldBodyConfiguration config;
            config.m_position.SetX((cylinderRadius + halfCylinderWallThickness) * std::cos(AZ::Constants::TwoPi * i / NumCylinderSide) + position.GetX());
            config.m_position.SetY((cylinderRadius + halfCylinderWallThickness) * std::sin(AZ::Constants::TwoPi * i / NumCylinderSide) + position.GetY());
            config.m_position.SetZ(z);
            config.m_orientation = AZ::Quaternion::CreateRotationZ(AZ::Constants::HalfPi + (cylinderTheta * i));

            AZStd::unique_ptr<Physics::RigidBodyStatic> cylinderWall = system->CreateStaticRigidBody(config);

            Physics::ColliderConfiguration colliderConfig;
            Physics::BoxShapeConfiguration shapeConfiguration(AZ::Vector3(cylinderRadius, cylinderWallThickness, cylinderHeight));
            AZStd::shared_ptr<Physics::Shape> shape = system->CreateShape(colliderConfig, shapeConfiguration);

            cylinderWall->AddShape(shape);

            legacyWorld->AddBody(*cylinderWall);

            m_cylinder[i] = AZStd::move(cylinderWall);
        }

        //create the prop
        const float bladeLength = cylinderRadius * 2.0f;
        const float bladeHeight = cylinderHeight * 0.75f;

        m_bladeAnimation.Init(RPM);

        Physics::RigidBodyConfiguration bladeRigidBodyConfig;
        bladeRigidBodyConfig.m_kinematic = true;
        bladeRigidBodyConfig.m_mass = 1000.0f;
        bladeRigidBodyConfig.m_position = position;
        bladeRigidBodyConfig.m_position.SetZ(position.GetZ() + (bladeHeight / 2.0f));
        bladeRigidBodyConfig.m_orientation = AZ::Quaternion::CreateRotationZ(0.0f);

        m_blade = system->CreateRigidBody(bladeRigidBodyConfig);

        Physics::ColliderConfiguration bladeColliderConfig;
        Physics::BoxShapeConfiguration bladeShapeConfiguration(AZ::Vector3(bladeLength, 1.0f, bladeHeight));
        AZStd::shared_ptr<Physics::Shape> shape = system->CreateShape(bladeColliderConfig, bladeShapeConfiguration);

        m_blade->AddShape(shape);

        legacyWorld->AddBody(*m_blade);

        Physics::WorldNotificationBus::Handler::BusConnect(legacyWorld->GetWorldId());
    }

    void WashingMachine::TearDownWashingMachine()
    {
        Physics::WorldNotificationBus::Handler::BusDisconnect();

        for (int i = 0; i < NumCylinderSide; i++)
        {
            m_cylinder[i].reset();
        }

        m_blade.reset();
    }

    void WashingMachine::OnPrePhysicsSubtick(float fixedDeltaTime)
    {
        AZ::Quaternion newRot = m_bladeAnimation.StepAnimation(fixedDeltaTime);

        AZ::Transform transform = m_blade->GetTransform();
        transform.SetRotation(newRot);
        m_blade->SetKinematicTarget(transform);
    }
}
#endif //HAVE_BENCHMARK
