/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/IndexedDataVector.h>
#include <Atom/Feature/Decals/DecalFeatureProcessorInterface.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <AtomCore/Instance/Instance.h>
#include <Atom/Feature/Utils/IndexableList.h>
#include <Decals/DecalTextureArray.h>
#include <Decals/AsyncLoadTracker.h>

namespace AZ
{
    namespace RPI
    {
        class Material;
        class Buffer;
    }

    namespace Render
    {
        class DecalTextureArrayFeatureProcessor final
            : public AZ::Render::DecalFeatureProcessorInterface
            , public Data::AssetBus::MultiHandler
        {
        public:

            AZ_RTTI(AZ::Render::DecalTextureArrayFeatureProcessor, "{5E8365FA-BEA7-4D02-9A5C-67E6810D5465}", AZ::Render::DecalFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            DecalTextureArrayFeatureProcessor() = default;
            virtual ~DecalTextureArrayFeatureProcessor() = default;
            DecalTextureArrayFeatureProcessor(const DecalTextureArrayFeatureProcessor&) = delete;
            DecalTextureArrayFeatureProcessor& operator=(const DecalTextureArrayFeatureProcessor&) = delete;

            // FeatureProcessor overrides ...
            void Activate() override;

            void Deactivate() override;
            void Simulate(const RPI::FeatureProcessor::SimulatePacket& packet) override;
            void Render(const RPI::FeatureProcessor::RenderPacket& packet) override;

            DecalHandle AcquireDecal() override;
            bool ReleaseDecal(const DecalHandle handle) override;
            DecalHandle CloneDecal(const DecalHandle handle) override;
            void SetDecalData(const DecalHandle handle, const DecalData& data) override;

            const Data::Instance<RPI::Buffer> GetDecalBuffer() const override;
            uint32_t GetDecalCount() const override;

            //! Sets the position of the decal
            void SetDecalPosition(const DecalHandle handle, const AZ::Vector3& position) override;

            //! Sets the orientation of the decal
            void SetDecalOrientation(const DecalHandle handle, const AZ::Quaternion& orientation) override;

            //! Sets the half size of the decal
            void SetDecalHalfSize(const DecalHandle handle, const Vector3& size) override;

            //! Sets the angle attenuation of the decal. Increasing this increases the transparency as the angle between the decal and geometry gets larger.
            void SetDecalAttenuationAngle(const DecalHandle handle, float angleAttenuation) override;

            //! Sets the opacity of the decal
            void SetDecalOpacity(const DecalHandle handle, float opacity) override;

            //! Sets the decal sort key. Decals with a larger sort key appear over top of smaller sort keys.
            void SetDecalSortKey(const DecalHandle handle, uint8_t sortKey) override;

            //! Sets the transform of the decal
            //! Equivalent to calling SetDecalPosition() + SetDecalOrientation() + SetDecalHalfSize()
            //! @{
            void SetDecalTransform(const DecalHandle handle, const AZ::Transform& world) override;
            void SetDecalTransform(const DecalHandle handle, const AZ::Transform& world, const AZ::Vector3& nonUniformScale) override;
            //! @}

            //! Sets the material information for this decal
            void SetDecalMaterial(const DecalHandle handle, const AZ::Data::AssetId id) override;

        private:

            // Number of size and format permutations
            // This number should match the number of texture arrays in Decals/ViewSrg.azsli
            static constexpr int NumTextureArrays = 5;
            static constexpr const char* FeatureProcessorName = "DecalTextureArrayFeatureProcessor";

            struct DecalLocation
            {
                int textureArrayIndex = -1;
                int textureIndex = -1;
            };

            struct DecalLocationAndUseCount
            {
                DecalLocation m_location;
                int m_useCount = 0;
            };

            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;

            void SetPackedTexturesToSrg(const RPI::ViewPtr& view);
            void CacheShaderIndices();

            // This call could fail (returning nullopt) if we run out of texture arrays
            AZStd::optional<DecalLocation> AddMaterialToTextureArrays(const AZ::RPI::MaterialAsset* materialAsset);

            int FindTextureArrayWithSize(const RHI::Size& size) const;
            void RemoveMaterialFromDecal(const uint16_t decalIndex);
            void SetDecalTextureLocation(const DecalHandle& handle, const DecalLocation location);
            void QueueMaterialLoadForDecal(const AZ::Data::AssetId material, const DecalHandle handle);
            bool RemoveDecalFromTextureArrays(const DecalLocation decalLocation);
            AZ::Data::AssetId GetMaterialUsedByDecal(const DecalHandle handle) const;
            void PackTexureArrays();

            IndexedDataVector<DecalData> m_decalData;

            // Texture arrays are organized one per texture size permutation.
            // e.g. There may be a situation where we have 3 texture arrays:
            // 24 textures @ 128x128
            // 16 textures @ 256x256
            // 4 textures @ 512x512
            IndexableList < AZStd::pair < AZ::RHI::Size, DecalTextureArray>> m_textureArrayList;

            AZStd::array<AZStd::array<RHI::ShaderInputImageIndex, DecalMapType_Num>, NumTextureArrays> m_decalTextureArrayIndices;
            GpuBufferHandler m_decalBufferHandler;

            AsyncLoadTracker<DecalHandle> m_materialLoadTracker;
            AZStd::unordered_map< AZ::Data::AssetId, DecalLocationAndUseCount> m_materialToTextureArrayLookupTable;

            bool m_deviceBufferNeedsUpdate = false;
        };
    } // namespace Render
} // namespace AZ
