/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <CoreLights/ShadowmapAtlas.h>

#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <AtomCore/Instance/Instance.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Preprocessor/Enum.h>

namespace AZ
{
    namespace RPI
    {
        class ShaderResourceGroup;
    }

    namespace Render
    {
        AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(EsmChildPassKind, uint32_t,
            (Exponentiation, 0),
            KawaseBlur0,
            KawaseBlur1);

        //! This pass outputs filtered shadowmap images used in ESM.
        //! ESM is an abbreviation of Exponential Shadow Maps.
        class EsmShadowmapsPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(EsmShadowmapsPass);
            using Base = RPI::ParentPass;

        public:
            AZ_CLASS_ALLOCATOR(EsmShadowmapsPass, SystemAllocator, 0);
            AZ_RTTI(EsmShadowmapsPass, "453E9AF0-C38F-4EBC-9871-8471C3D5369A", RPI::ParentPass);

            struct FilterParameter
            {
                uint32_t m_isEnabled = false;
                AZStd::array<uint32_t, 2> m_shadowmapOriginInSlice = { {0, 0 } }; // shadowmap origin in the slice of the atlas.
                uint32_t m_shadowmapSize = static_cast<uint32_t>(ShadowmapSize::None); // width and height of shadowmap.
                float m_lightDistanceOfCameraViewFrustum = 0.f;
                float m_n_f_n = 0.f; // n / (f - n)
                float m_n_f = 0.f;   // n - f
                float m_f = 0.f;     // f
                                     // where n: nearDepth, f: farDepth.
            };

            virtual ~EsmShadowmapsPass() = default;
            static RPI::Ptr<EsmShadowmapsPass> Create(const RPI::PassDescriptor& descriptor);

            const Name& GetLightTypeName() const;

            //! This sets the buffer of the table which enable to get shadowmap index
            //! from the coordinate in the atlas.
            //! Note that shadowmpa index is shader light index for a spot light
            //! and it is cascade index for a directional light.
            void SetShadowmapIndexTableBuffer(const Data::Instance<RPI::Buffer>& tableBuffer);

            //! This sets filter parameter buffer.
            void SetFilterParameterBuffer(const Data::Instance<RPI::Buffer>& dataBuffer);

            //! This enable/disable children's computations.
            void SetEnabledComputation(bool enabled);

        private:
            EsmShadowmapsPass() = delete;
            explicit EsmShadowmapsPass(const RPI::PassDescriptor& descriptor);

            // Pass Behaviour overrides...
            void ResetInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

            void UpdateChildren();
            // Parameters for both the depth exponentiation pass along with the kawase blur passes
            void SetBlurParameters(Data::Instance<RPI::ShaderResourceGroup> srg, const uint32_t childPassIndex);
            void SetKawaseBlurSpecificParameters(Data::Instance<RPI::ShaderResourceGroup> srg, const uint32_t kawaseBlurIndex);

            bool m_computationEnabled = false;
            Name m_lightTypeName;
            RHI::Size m_shadowmapImageSize;
            uint16_t m_shadowmapArraySize;

            Data::Instance<RPI::Buffer> m_filterTableBuffer;
            AZStd::array<RHI::ShaderInputBufferIndex, EsmChildPassKindCount> m_filterTableBufferIndices;
            AZStd::vector<uint32_t> m_filterCounts;

            AZStd::array<RHI::ShaderInputBufferIndex, EsmChildPassKindCount> m_shadowmapIndexTableBufferIndices;
            Data::Instance<RPI::Buffer> m_shadowmapIndexTableBuffer;
            AZStd::array<RHI::ShaderInputBufferIndex, EsmChildPassKindCount> m_filterParameterBufferIndices;
            Data::Instance<RPI::Buffer> m_filterParameterBuffer;

            AZStd::array<RHI::ShaderInputConstantIndex, 2> m_kawaseBlurConstantIndices;
        };
    } // namespace Render
} // namespace AZ
