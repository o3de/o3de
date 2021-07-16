/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Utils/GpuBufferHandler.h>
#include <Atom/Feature/Utils/MultiIndexedDataVector.h>
#include <Atom/Feature/Decals/DecalFeatureProcessorInterface.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>
#include "Atom/RPI.Public/Image/StreamingImage.h"
#include <AtomCore/Instance/Instance.h>

namespace AZ
{
    namespace RPI
    {
        class Material;
    }

    namespace Render
    {
        class DecalFeatureProcessor final
            : public AZ::Render::DecalFeatureProcessorInterface
        {
        public:
            AZ_RTTI(AZ::Render::DecalFeatureProcessor, "{D83C0358-AB43-403D-AB13-3444FE44AEEB}", AZ::Render::DecalFeatureProcessorInterface);

            static void Reflect(AZ::ReflectContext* context);

            DecalFeatureProcessor();
            virtual ~DecalFeatureProcessor() = default;

            // FeatureProcessor overrides ...
            void Activate()override;
            void Deactivate()override;
            void Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)override;
            void Render(const RPI::FeatureProcessor::RenderPacket& packet)override;

            DecalHandle AcquireDecal() override;
            bool ReleaseDecal(DecalHandle handle)override;
            DecalHandle CloneDecal(DecalHandle handle)override;
            void SetDecalData(DecalHandle handle, const DecalData& data)override;

            const Data::Instance<RPI::Buffer> GetDecalBuffer() const override;
            uint32_t GetDecalCount() const override;

            //! Sets the position of the decal
            void SetDecalPosition(DecalHandle handle, const AZ::Vector3& position)override;

            //! Sets the orientation of the decal
            void SetDecalOrientation(DecalHandle handle, const AZ::Quaternion& orientation) override;

            //! Sets the half size of the decal
            void SetDecalHalfSize(DecalHandle handle, const Vector3& size) override;

            //! Sets the angle attenuation of the decal. Increasing this increases the transparency as the angle between the decal and geometry gets larger.
            void SetDecalAttenuationAngle(DecalHandle handle, float angleAttenuation) override;

            //! Sets the opacity of the decal
            void SetDecalOpacity(DecalHandle handle, float opacity) override;

            //! Sets the decal sort key. Decals with a larger sort key appear over top of smaller sort keys.
            void SetDecalSortKey(DecalHandle handle, uint8_t sortKey) override;

            //! Sets the transform of the decal
            //! Equivalent to calling SetDecalPosition() + SetDecalOrientation() + SetDecalHalfSize()
            //! @{
            void SetDecalTransform(DecalHandle handle, const AZ::Transform& world) override;
            void SetDecalTransform(DecalHandle handle, const AZ::Transform& world, const AZ::Vector3& nonUniformScale) override;
            //! @}

            //! Sets the material information for this decal
            void SetDecalMaterial(DecalHandle handle, const AZ::Data::AssetId) override;

        private:

            DecalFeatureProcessor(const DecalFeatureProcessor&) = delete;
            Data::Instance<RPI::Image> GetImageFromMaterial(const AZ::Name& mapName, Data::Instance<RPI::Material> materialInstance) const;
            AZStd::array_view<Data::Instance<RPI::Image>> GetImageArray() const;
            void CacheShaderIndices();

            template<size_t ArrayIndex>
            AZStd::array_view<Data::Instance<RPI::Image>> GetImagesFromDecalData();

            static constexpr const char* FeatureProcessorName = "DecalFeatureProcessor";

            using ImagePtr = Data::Instance<RPI::Image>;
            using DataVector = MultiIndexedDataVector<DecalData, ImagePtr, ImagePtr>;
            DataVector m_decalData;

            GpuBufferHandler m_decalBufferHandler;
            bool m_deviceBufferNeedsUpdate = false;
            RHI::ShaderInputImageIndex m_baseColorMapsIndex;
            RHI::ShaderInputImageIndex m_opacityMapsIndex;

            AZ::Name m_baseColorMapShaderName;
            AZ::Name m_opacityMapShaderName;
            AZ::Name m_baseColorMapPropertyName;
            AZ::Name m_opacityMapPropertyName;
        };

        template<size_t ArrayIndex>
        AZStd::array_view<Data::Instance<RPI::Image>>
        AZ::Render::DecalFeatureProcessor::GetImagesFromDecalData()
        {
            // [GFX TODO][ATOM-4445] Replace this hardcoded constant with atlasing / bindless so we can have far more than 8 decal textures
            // Note this constant also is defined in View.srg
            const size_t MaxDecals = 4;
            size_t numImages = AZStd::min(MaxDecals, m_decalData.GetDataCount());

            AZStd::array_view<ImagePtr> imageArrayView(m_decalData.GetDataVector<ArrayIndex>().begin(), m_decalData.GetDataVector<ArrayIndex>().begin() + numImages);
            return imageArrayView;
        }
    } // namespace Render
} // namespace AZ
