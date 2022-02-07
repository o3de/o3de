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

#include <GradientSignal/Ebuses/GradientRequestBus.h>
#include <GradientSignal/Ebuses/GradientPreviewContextRequestBus.h>
#include <GradientSignal/GradientSampler.h>
#include <GradientSignal/ImageAsset.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <SurfaceData/SurfaceDataProviderRequestBus.h>
#include <SurfaceData/SurfaceDataSystemRequestBus.h>
#include <SurfaceData/Tests/SurfaceDataTestMocks.h>

namespace UnitTest
{
    // Mock asset handler for GradientSignal::ImageAsset that we can use in unit tests to pretend to load an image asset with.
    // Also includes utility functions for creating image assets with specific testable patterns.
    struct ImageAssetMockAssetHandler : public AZ::Data::AssetHandler
    {
        //! Creates a deterministically random set of pixel data as an ImageAsset.
        //! \param width The width of the ImageAsset
        //! \param height The height of the ImageAsset
        //! \param seed The random seed to use for generating the random data
        //! \return The ImageAsset in a loaded ready state
        static AZ::Data::Asset<GradientSignal::ImageAsset> CreateImageAsset(AZ::u32 width, AZ::u32 height, AZ::s32 seed);

        //! Creates an ImageAsset where all the pixels are 0 except for the one pixel at the given coordinates, which is set to 1.
        //! \param width The width of the ImageAsset
        //! \param height The height of the ImageAsset
        //! \param pixelX The X coordinate of the pixel to set to 1
        //! \param pixelY The Y coordinate of the pixel to set to 1
        //! \return The ImageAsset in a loaded ready state
        static AZ::Data::Asset<GradientSignal::ImageAsset> CreateSpecificPixelImageAsset(
            AZ::u32 width, AZ::u32 height, AZ::u32 pixelX, AZ::u32 pixelY);

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type) override
        {
            // For our mock handler, always mark our assets as immediately ready.
            return aznew GradientSignal::ImageAsset(id, AZ::Data::AssetData::AssetStatus::Ready);
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            if (ptr)
            {
                delete ptr;
            }
        }

        void GetHandledAssetTypes([[maybe_unused]] AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
        }

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            [[maybe_unused]] AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
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

            SurfaceData::SurfaceDataSystemRequestBus::BroadcastResult(
                m_providerHandle, &SurfaceData::SurfaceDataSystemRequestBus::Events::RegisterSurfaceDataProvider, providerRegistryEntry);
            SurfaceData::SurfaceDataProviderRequestBus::Handler::BusConnect(m_providerHandle);
        }

        void Deactivate() override
        {
            SurfaceData::SurfaceDataSystemRequestBus::Broadcast(
                &SurfaceData::SurfaceDataSystemRequestBus::Events::UnregisterSurfaceDataProvider, m_providerHandle);
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
                surfacePointList = surfacePoints->second;
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

}
