/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshVertexStreams.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/Name/Name.h>

namespace AZ
{

    namespace Render
    {
        class SkinnedMeshVertexStreamProperties
            : public SkinnedMeshVertexStreamPropertyInterface
        {
        public:
            AZ_RTTI(AZ::Render::SkinnedMeshVertexStreamProperties, "{8912239E-8412-4B9E-BDE6-AE6BA67A207C}", AZ::Render::SkinnedMeshVertexStreamPropertyInterface);

            SkinnedMeshVertexStreamProperties();

            const SkinnedMeshVertexStreamInfo* GetInputStreamInfo(const RHI::ShaderSemantic& shaderSemantic ) const override;
            const SkinnedMeshVertexStreamInfo& GetInputStreamInfo(SkinnedMeshInputVertexStreams stream) const override;
            const SkinnedMeshVertexStreamInfo& GetStaticStreamInfo(SkinnedMeshStaticVertexStreams stream) const override;
            const SkinnedMeshOutputVertexStreamInfo* GetOutputStreamInfo(const RHI::ShaderSemantic& shaderSemantic) const override;
            const SkinnedMeshOutputVertexStreamInfo& GetOutputStreamInfo(SkinnedMeshOutputVertexStreams stream) const override;

            Data::Asset<RPI::ResourcePoolAsset> GetOutputStreamResourcePool() const override;

            uint32_t GetMaxSupportedVertexCount() const override;
            const RPI::ShaderInputContract& GetComputeShaderInputContract() const override;
        private:
            AZStd::array<SkinnedMeshVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_inputStreamInfo;
            AZStd::array<SkinnedMeshVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams)> m_staticStreamInfo;
            AZStd::array<SkinnedMeshOutputVertexStreamInfo, static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::NumVertexStreams)> m_outputStreamInfo;
            
            Data::Asset<RPI::ResourcePoolAsset> m_outputStreamResourcePool;

            RPI::ShaderInputContract m_computeShaderInputContract;
        };
    } // namespace Render
} // namespace AZ
