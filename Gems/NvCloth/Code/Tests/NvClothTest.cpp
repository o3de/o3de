/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Component/TickBus.h>

#include <NvCloth/IClothSystem.h>
#include <NvCloth/ICloth.h>
#include <NvCloth/IClothConfigurator.h>
#include <NvCloth/IFabricCooker.h>

#include <TriangleInputHelper.h>

namespace UnitTest
{
    //! Sets up a cloth and colliders for each test case.
    class NvClothTestFixture
        : public ::testing::Test
    {
    protected:
        // ::testing::Test overrides ...
        void SetUp() override;
        void TearDown() override;

        //! Sends tick events to make cloth simulation happen.
        //! Returns the position of cloth particles at tickBefore, continues ticking till tickAfter.
        void TickClothSimulation(const AZ::u32 tickBefore,
            const AZ::u32 tickAfter,
            AZStd::vector<NvCloth::SimParticleFormat>& particlesBefore);

        NvCloth::ICloth* m_cloth = nullptr;
        NvCloth::ICloth::PreSimulationEvent::Handler m_preSimulationEventHandler;
        NvCloth::ICloth::PostSimulationEvent::Handler m_postSimulationEventHandler;
        bool m_postSimulationEventInvoked = false;

    private:
        bool CreateCloth();
        void DestroyCloth();

        // ICloth notifications
        void OnPreSimulation(NvCloth::ClothId clothId, float deltaTime);
        void OnPostSimulation(NvCloth::ClothId clothId,
            float deltaTime,
            const AZStd::vector<NvCloth::SimParticleFormat>& updatedParticles);

        AZ::Transform m_clothTransform = AZ::Transform::CreateIdentity();
        AZStd::vector<AZ::Vector4> m_sphereColliders;
    };

    void NvClothTestFixture::SetUp()
    {
        m_preSimulationEventHandler = NvCloth::ICloth::PreSimulationEvent::Handler(
            [this](NvCloth::ClothId clothId, float deltaTime)
            {
                this->OnPreSimulation(clothId, deltaTime);
            });
        m_postSimulationEventHandler = NvCloth::ICloth::PostSimulationEvent::Handler(
            [this](NvCloth::ClothId clothId, float deltaTime, const AZStd::vector<NvCloth::SimParticleFormat>& updatedParticles)
            {
                this->OnPostSimulation(clothId, deltaTime, updatedParticles);
            });

        bool clothCreated = CreateCloth();
        ASSERT_TRUE(clothCreated);
    }

    void NvClothTestFixture::TearDown()
    {
        DestroyCloth();
    }

    void NvClothTestFixture::TickClothSimulation(const AZ::u32 tickBefore,
        const AZ::u32 tickAfter,
        AZStd::vector<NvCloth::SimParticleFormat>& particlesBefore)
    {
        const float timeOneFrameSeconds = 0.016f; //approx 60 fps

        for (AZ::u32 tickCount = 0; tickCount < tickAfter; ++tickCount)
        {
            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick,
                timeOneFrameSeconds,
                AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));

            if (tickCount == tickBefore)
            {
                particlesBefore = m_cloth->GetParticles();
            }
        }
    }

    bool NvClothTestFixture::CreateCloth()
    {
        const float width = 2.0f;
        const float height = 2.0f;
        const AZ::u32 segmentsX = 10;
        const AZ::u32 segmentsY = 10;

        const TriangleInput planeXY = CreatePlane(width, height, segmentsX, segmentsY);

        // Cook Fabric
        AZStd::optional<NvCloth::FabricCookedData> cookedData = AZ::Interface<NvCloth::IFabricCooker>::Get()->CookFabric(planeXY.m_vertices, planeXY.m_indices);
        if (!cookedData)
        {
            return false;
        }

        // Create cloth instance
        m_cloth = AZ::Interface<NvCloth::IClothSystem>::Get()->CreateCloth(planeXY.m_vertices, *cookedData);
        if (!m_cloth)
        {
            return false;
        }

        m_sphereColliders.emplace_back(512.0f, 512.0f, 35.0f, 1.0f);
        m_clothTransform.SetTranslation(AZ::Vector3(512.0f, 519.0f, 35.0f));

        m_cloth->GetClothConfigurator()->SetTransform(m_clothTransform);
        m_cloth->GetClothConfigurator()->ClearInertia();

        // Add cloth to default solver to be simulated
        AZ::Interface<NvCloth::IClothSystem>::Get()->AddCloth(m_cloth);

        return true;
    }

    void NvClothTestFixture::DestroyCloth()
    {
        if (m_cloth)
        {
            AZ::Interface<NvCloth::IClothSystem>::Get()->RemoveCloth(m_cloth);
            AZ::Interface<NvCloth::IClothSystem>::Get()->DestroyCloth(m_cloth);
        }
    }

    void NvClothTestFixture::OnPreSimulation([[maybe_unused]] NvCloth::ClothId clothId, float deltaTime)
    {
        m_cloth->GetClothConfigurator()->SetTransform(m_clothTransform);

        constexpr float velocity = 1.0f;

        for (auto& sphere : m_sphereColliders)
        {
            sphere.SetY(sphere.GetY() + velocity * deltaTime);
        }

        auto clothInverseTransform = m_clothTransform.GetInverse();
        auto sphereColliders = m_sphereColliders;
        for (auto& sphere : sphereColliders)
        {
            sphere.Set(clothInverseTransform.TransformPoint(sphere.GetAsVector3()), sphere.GetW());
        }
        m_cloth->GetClothConfigurator()->SetSphereColliders(sphereColliders);
    }

    void NvClothTestFixture::OnPostSimulation(
        NvCloth::ClothId clothId, float deltaTime,
        const AZStd::vector<NvCloth::SimParticleFormat>& updatedParticles)
    {
        AZ_UNUSED(clothId);
        AZ_UNUSED(deltaTime);
        AZ_UNUSED(updatedParticles);
        m_postSimulationEventInvoked = true;
    }

    //! Smallest Z and largest Y coordinates for a list of particles before, and a list of particles after simulation for some time.
    struct ParticleBounds
    {
        float m_beforeSmallestZ = std::numeric_limits<float>::max();
        float m_beforeLargestY = -std::numeric_limits<float>::max();

        float m_afterSmallestZ = std::numeric_limits<float>::max();
        float m_afterLargestY = -std::numeric_limits<float>::max();
    };

    static ParticleBounds GetBeforeAndAfterParticleBounds(const AZStd::vector<NvCloth::SimParticleFormat>& particlesBefore,
        const AZStd::vector<NvCloth::SimParticleFormat>& particlesAfter)
    {
        assert(particlesBefore.size() == particlesAfter.size());

        ParticleBounds beforeAndAfterParticleBounds;

        for (size_t particleIndex = 0; particleIndex < particlesBefore.size(); ++particleIndex)
        {
            if (particlesBefore[particleIndex].GetZ() < beforeAndAfterParticleBounds.m_beforeSmallestZ)
            {
                beforeAndAfterParticleBounds.m_beforeSmallestZ = particlesBefore[particleIndex].GetZ();
            }
            if (particlesBefore[particleIndex].GetY() > beforeAndAfterParticleBounds.m_beforeLargestY)
            {
                beforeAndAfterParticleBounds.m_beforeLargestY = particlesBefore[particleIndex].GetY();
            }

            if (particlesAfter[particleIndex].GetZ() < beforeAndAfterParticleBounds.m_afterSmallestZ)
            {
                beforeAndAfterParticleBounds.m_afterSmallestZ = particlesAfter[particleIndex].GetZ();
            }
            if (particlesAfter[particleIndex].GetY() > beforeAndAfterParticleBounds.m_afterLargestY)
            {
                beforeAndAfterParticleBounds.m_afterLargestY = particlesAfter[particleIndex].GetY();
            }
        }

        return beforeAndAfterParticleBounds;
    }

    //! Tests that basic cloth simulation works.
    TEST_F(NvClothTestFixture, Cloth_NoCollision_FallWithGravity)
    {
        const AZ::u32 tickBefore = 150;
        const AZ::u32 tickAfter = 300;
        AZStd::vector<NvCloth::SimParticleFormat> particlesBefore;
        TickClothSimulation(tickBefore, tickAfter, particlesBefore);

        ParticleBounds particleBounds = GetBeforeAndAfterParticleBounds(particlesBefore,
            m_cloth->GetParticles());

        // Cloth was extended horizontally in the y-direction earlier.
        // If cloth fell with gravity, its y-extent should be smaller later,
        // and its z-extent should go lower to a smaller Z value later.
        ASSERT_TRUE((particleBounds.m_afterLargestY < particleBounds.m_beforeLargestY) &&
            (particleBounds.m_afterSmallestZ < particleBounds.m_beforeSmallestZ));
    }

    //! Tests that collision works and pre/post simulation events work.
    TEST_F(NvClothTestFixture, Cloth_Collision_CollidedWithPrePostSimEvents)
    {
        m_cloth->ConnectPreSimulationEventHandler(m_preSimulationEventHandler); // The pre-simulation callback moves the sphere collider towards the cloth every tick.
        m_cloth->ConnectPostSimulationEventHandler(m_postSimulationEventHandler);

        const AZ::u32 tickBefore = 150;
        const AZ::u32 tickAfter = 320;
        AZStd::vector<NvCloth::SimParticleFormat> particlesBefore;
        TickClothSimulation(tickBefore, tickAfter, particlesBefore);

        ParticleBounds particleBounds = GetBeforeAndAfterParticleBounds(particlesBefore,
            m_cloth->GetParticles());

        // Cloth starts extended horizontally (along Y-axis). Simulation makes it swing down with gravity (as tested with the other unit test).
        // Then the sphere collider collides with the cloth and pushes it back up. So it is again extended in the Y-direction and
        // at about the same vertical height (Z-coord) as before.
        const float threshold = 0.25f;
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_beforeSmallestZ , -0.97f, threshold));
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_beforeLargestY, 0.76f, threshold));
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_afterSmallestZ, -1.1f, threshold));
        EXPECT_TRUE(AZ::IsClose(particleBounds.m_afterLargestY, 0.72f, threshold));
        ASSERT_TRUE((fabsf(particleBounds.m_afterLargestY - particleBounds.m_beforeLargestY) < threshold) &&
            (fabsf(particleBounds.m_afterSmallestZ - particleBounds.m_beforeSmallestZ) < threshold));

        // Check that post simulation event was invoked.
        ASSERT_TRUE(m_postSimulationEventInvoked);

        m_preSimulationEventHandler.Disconnect();
        m_postSimulationEventHandler.Disconnect();
    }
} // namespace UnitTest
