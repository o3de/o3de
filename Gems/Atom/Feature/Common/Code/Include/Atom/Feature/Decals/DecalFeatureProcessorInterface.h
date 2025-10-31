/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Quaternion.h>

namespace AZ
{
    class Vector3;
    namespace RPI
    {
        class MaterialAsset;
        class Buffer;
    }

    namespace Render
    {
        struct DecalData
        {
            AZStd::array<float, 3> m_position = {
                { 0.0f, 0.0f, 0.0f }
            };
            float m_opacity = 1.0f;
            AZStd::array<float, 4> m_quaternion = {
                { 0, 0, 0, 1 }
            };
            AZStd::array<float, 3> m_halfSize = {
                { 0.5f, 0.5f, 0.5f }
            };
            float m_angleAttenuation = 1.0f;
            float m_normalMapOpacity = 1.0f;
            // Decals with a larger sort key appear over top of smaller sort keys.
            uint8_t m_sortKey = 0;
            uint32_t m_textureArrayIndex = UnusedIndex;
            uint32_t m_textureIndex = UnusedIndex;

            AZStd::array<float, 3> m_decalColor = { { 1.0f, 1.0f, 1.0f } };
            float m_decalColorFactor = 1.0f;
            static constexpr uint32_t UnusedIndex = std::numeric_limits< uint32_t>::max();
        };

        //! DecalFeatureProcessorInterface provides an interface to acquire, release, and update a decal. This is necessary for code outside of
        //! the Atom features gem to communicate with the DecalTextureArrayFeatureProcessor.
        class DecalFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DecalFeatureProcessorInterface, "{4A64E427-7F9F-4AF7-B414-69EA91323827}", AZ::RPI::FeatureProcessor);

            using DecalHandle = RHI::Handle<uint16_t, class Decal>;

            //! Creates a new decal which can be referenced by the returned DecalHandle. Must be released via ReleaseDecal() when no longer needed.
            virtual DecalHandle AcquireDecal() = 0;

            //! Releases a Decal.
            virtual bool ReleaseDecal(DecalHandle handle) = 0;

            //! Creates a new Decal by copying data from an existing DecalHandle.
            virtual DecalHandle CloneDecal(DecalHandle handle) = 0;

            //! Sets all of the the decal data for the provided DecalHandle.
            virtual void SetDecalData(DecalHandle handle, const DecalData& data) = 0;

            //! Sets the position of the decal
            virtual void SetDecalPosition(DecalHandle handle, const AZ::Vector3& position) = 0;

            //! Sets the color of the decal
            virtual void SetDecalColor(DecalHandle handle, const AZ::Vector3& color) = 0;

            //! Sets the color factor of the decal
            virtual void SetDecalColorFactor(DecalHandle handle, float colorFactor) = 0;

            //! Sets the orientation of the decal
            virtual void SetDecalOrientation(DecalHandle handle, const AZ::Quaternion& orientation) = 0;

            //! Sets the half size of the decal
            virtual void SetDecalHalfSize(DecalHandle handle, const Vector3& halfSize) = 0;

            //! Sets the angle attenuation of the decal. Increasing this increases the transparency as the angle between the decal and geometry gets larger.
            virtual void SetDecalAttenuationAngle(DecalHandle handle, float angleAttenuation) = 0;

            //! Sets the opacity of the decal
            virtual void SetDecalOpacity(DecalHandle handle, float opacity) = 0;

            //! Sets the opacity of the decal normal map
            virtual void SetDecalNormalMapOpacity(DecalHandle handle, float opacity) = 0;

            //! Sets the transform of the decal
            //! Equivalent to calling SetDecalPosition() + SetDecalOrientation() + SetDecalHalfSize()
            //! @{
            virtual void SetDecalTransform(DecalHandle handle, const AZ::Transform& world) = 0;
            virtual void SetDecalTransform(DecalHandle handle, const AZ::Transform& world, const AZ::Vector3& nonUniformScale) = 0;
            //! @}

            //! Sets the material information for this decal
            virtual void SetDecalMaterial(DecalHandle handle, const AZ::Data::AssetId) = 0;

            //! Sets the sort key for the decal. Decals with a larger sort key appear over top of smaller sort keys.
            virtual void SetDecalSortKey(DecalHandle handle, uint8_t sortKey) = 0;

            //! Returns a GPU readable buffer containing the contiguous array of decals.
            virtual const Data::Instance<RPI::Buffer> GetDecalBuffer() const = 0;

            //! Returns the number of decals currently in the buffer.
            virtual uint32_t GetDecalCount() const = 0;
        };
    } // namespace Render
} // namespace AZ
