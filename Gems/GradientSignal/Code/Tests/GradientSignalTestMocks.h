/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/std/hash.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <AzCore/Casting/lossy_cast.h>

#include <SurfaceData/Tests/SurfaceDataTestMocks.h>

namespace UnitTest
{
    struct GradientSignalTest
        : public ::testing::Test
    {
    protected:
        AZ::ComponentApplication m_app;
        AZ::Entity* m_systemEntity = nullptr;

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_memoryBlocksByteSize = 128 * 1024 * 1024;
            m_systemEntity = m_app.Create(appDesc);
            m_app.AddEntity(m_systemEntity);
        }

        void TearDown() override
        {
            m_app.Destroy();
            m_systemEntity = nullptr;
        }

        void TestFixedDataSampler(const AZStd::vector<float>& expectedOutput, int size, AZ::EntityId gradientEntityId)
        {
            GradientSignal::GradientSampler gradientSampler;
            gradientSampler.m_gradientId = gradientEntityId;

            for(int y = 0; y < size; ++y)
            {
                for (int x = 0; x < size; ++x)
                {
                    GradientSignal::GradientSampleParams params;
                    params.m_position = AZ::Vector3(static_cast<float>(x), static_cast<float>(y), 0.0f);

                    const int index = y * size + x;
                    float actualValue = gradientSampler.GetValue(params);
                    float expectedValue = expectedOutput[index];

                    EXPECT_NEAR(actualValue, expectedValue, 0.01f);
                }
            }
        }

        AZStd::unique_ptr<AZ::Entity> CreateEntity()
        {
            return AZStd::make_unique<AZ::Entity>();
        }

        void ActivateEntity(AZ::Entity* entity)
        {
            entity->Init();
            EXPECT_EQ(AZ::Entity::State::Init, entity->GetState());

            entity->Activate();
            EXPECT_EQ(AZ::Entity::State::Active, entity->GetState());
        }

        template <typename Component, typename Configuration>
        AZ::Component* CreateComponent(AZ::Entity* entity, const Configuration& config)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>(config);
        }

        template <typename Component>
        AZ::Component* CreateComponent(AZ::Entity* entity)
        {
            m_app.RegisterComponentDescriptor(Component::CreateDescriptor());
            return entity->CreateComponent<Component>();
        }
    };

    struct MockGradientRequestsBus
        : public GradientSignal::GradientRequestBus::Handler
    {
        MockGradientRequestsBus(const AZ::EntityId& id)
        {
            BusConnect(id);
        }

        ~MockGradientRequestsBus()
        {
            BusDisconnect();
        }

        float m_GetValue = 0.0f;
        float GetValue([[maybe_unused]] const GradientSignal::GradientSampleParams& sampleParams) const override
        {
            return m_GetValue;
        }

        bool IsEntityInHierarchy(const AZ::EntityId &) const override
        {
            return false;
        }
    };

    struct MockGradientArrayRequestsBus
        : public GradientSignal::GradientRequestBus::Handler
    {
        MockGradientArrayRequestsBus(const AZ::EntityId& id, const AZStd::vector<float>& data, int rowSize)
            : m_getValue(data), m_rowSize(rowSize)
        {
            BusConnect(id);

            // We expect each value to get requested exactly once.
            m_positionsRequested.reserve(data.size());
        }

        ~MockGradientArrayRequestsBus()
        {
            BusDisconnect();
        }

        float GetValue(const GradientSignal::GradientSampleParams& sampleParams) const override
        {
            const auto& pos = sampleParams.m_position;
            const int index = azlossy_caster<float>(pos.GetY() * float(m_rowSize) + pos.GetX());

            m_positionsRequested.push_back(sampleParams.m_position);

            return m_getValue[index];
        }

        bool IsEntityInHierarchy(const AZ::EntityId &) const override
        {
            return false;
        }

        AZStd::vector<float> m_getValue;
        int m_rowSize;
        mutable AZStd::vector<AZ::Vector3> m_positionsRequested;
    };

    struct MockGradientPreviewContextRequestBus
        : public GradientSignal::GradientPreviewContextRequestBus::Handler
    {
        MockGradientPreviewContextRequestBus(const AZ::EntityId& id, const AZ::Aabb& previewBounds, bool constrainToShape)
            : m_previewBounds(previewBounds)
            , m_constrainToShape(constrainToShape)
            , m_id(id)
        {
            BusConnect(id);
        }

        ~MockGradientPreviewContextRequestBus()
        {
            BusDisconnect();
        }

        AZ::EntityId GetPreviewEntity() const override { return m_id; }
        AZ::Aabb GetPreviewBounds() const override { return m_previewBounds; }
        bool GetConstrainToShape() const override { return m_constrainToShape; }

    protected:
        AZ::EntityId m_id;
        AZ::Aabb m_previewBounds;
        bool m_constrainToShape;
    };

}
