/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/hash.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <GradientSignal/Components/PerlinGradientComponent.h>
#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Tests/SurfaceDataTestMocks.h>

namespace UnitTest
{
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

            // Because gradients repeat infinitely by default, we mod by rowSize to bring the position into the correct range.
            // The extra "+ rowSize) % rowSize" piece is so that negative values come into the correct range as well.
            // If we just took the absolute value first, the mod would cause us to reverse the lookup on the negative side instead
            // of continuing it.
            int posX = ((static_cast<int>(pos.GetX()) % m_rowSize) + m_rowSize) % m_rowSize;
            int posY = ((static_cast<int>(pos.GetY()) % m_rowSize) + m_rowSize) % m_rowSize;

            const int index = (posY * m_rowSize) + posX;

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

    // Mock out a SurfaceProvider component so that we can control exactly what surface weights get returned
    // at which points for our unit tests.
    struct MockSurfaceProviderComponent
        : public AZ::Component
        , public SurfaceData::SurfaceDataProviderRequestBus::Handler
    {
    public:
        AZ_COMPONENT(MockSurfaceProviderComponent, "{18C71877-DB29-4CEC-B34C-B4B44E05203D}", AZ::Component);

        void Activate() override
        {
            SurfaceData::SurfaceDataRegistryEntry providerRegistryEntry;
            providerRegistryEntry.m_entityId = GetEntityId();
            providerRegistryEntry.m_bounds = m_bounds;
            providerRegistryEntry.m_tags = m_tags;

            // Run through the set of surface points that have been set on this component to find out the maximum number
            // that we'll return for any given input point.
            providerRegistryEntry.m_maxPointsCreatedPerInput = 1;
            for (auto& pointEntry : m_surfacePoints)
            {
                for (size_t index = 0; index < pointEntry.second.GetInputPositionSize(); index++)
                {
                    providerRegistryEntry.m_maxPointsCreatedPerInput =
                        AZ::GetMax(providerRegistryEntry.m_maxPointsCreatedPerInput, pointEntry.second.GetSize(index));
                }
            }

            m_providerHandle = AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->RegisterSurfaceDataProvider(providerRegistryEntry);
            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
        }

        void Deactivate() override
        {
            AZ::Interface<SurfaceData::SurfaceDataSystem>::Get()->UnregisterSurfaceDataProvider(m_providerHandle);
            m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusDisconnect();
        }

        static void Reflect([[maybe_unused]] AZ::ReflectContext* reflect)
        {
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SurfaceDataProviderService"));
        }

        void GetSurfacePoints(const AZ::Vector3& inPosition, SurfaceData::SurfacePointList& surfacePointList) const override
        {
            auto surfacePoints = m_surfacePoints.find(AZStd::make_pair(inPosition.GetX(), inPosition.GetY()));

            if (surfacePoints != m_surfacePoints.end())
            {
                // If we have an entry for this input position, run through all of its points and add them to the passed-in list.
                surfacePoints->second.EnumeratePoints(
                    [inPosition, &surfacePointList](
                        [[maybe_unused]] size_t inPositionIndex, const AZ::Vector3& position, const AZ::Vector3& normal,
                        const SurfaceData::SurfaceTagWeights& weights) -> bool
                    {
                        surfacePointList.AddSurfacePoint(AZ::EntityId(), inPosition, position, normal, weights);
                        return true;
                    });
            }
        }

        // m_surfacePoints is a mapping of locations to surface tags / weights that should be returned.
        AZStd::unordered_map<AZStd::pair<float, float>, SurfaceData::SurfacePointList> m_surfacePoints;

        // m_bounds is the AABB to use for our mock surface provider.
        AZ::Aabb m_bounds;

        // m_tags are the possible set of tags that this provider will return.
        SurfaceData::SurfaceTagVector m_tags;

        SurfaceData::SurfaceDataRegistryHandle m_providerHandle = SurfaceData::InvalidSurfaceDataRegistryHandle;
    };


    // Mock the GradientSignal::PerlinGradientComponent so that its can inject a fixed permutation table for
    // the perlin noise algorithm for consistent unit test results across all platforms
    class MockGradientSignal : public GradientSignal::PerlinGradientComponent
    {
    public:
        AZ_COMPONENT(MockGradientSignal, "{72B18966-6B4A-42C7-86AE-72AB6B1B84C5}", GradientSignal::PerlinGradientComponent);

        MockGradientSignal() = default;
        MockGradientSignal(const GradientSignal::PerlinGradientConfig& configuration)
            : GradientSignal::PerlinGradientComponent(configuration)
        {
        }

        static void Reflect(AZ::ReflectContext*) {}

        void Activate() override
        {
            GradientSignal::PerlinGradientComponent::Activate();

            m_perlinImprovedNoise.reset(aznew GradientSignal::PerlinImprovedNoise(m_testPermutationTable));
        }

        void SetPerlinNoisePermutationTableForTest(const AZStd::array<int, 512>& permutationTable)
        {
            AZStd::copy(permutationTable.cbegin(), permutationTable.cend(), m_testPermutationTable.begin());
        }

        AZStd::array<int, 512>  m_testPermutationTable;
    };
}
