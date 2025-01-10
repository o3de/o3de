/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshFeatureProcessorInterface.h>
#include <Atom/Feature/Mesh/StreamBufferViewsBuilderInterface.h>

namespace AZ
{
    namespace Render
    {
        class ShaderStreamBufferViewsBuilder final
            : public StreamBufferViewsBuilderInterface
        {
        public:
            AZ_RTTI(AZ::Render::ShaderStreamBufferViewsBuilder,
                "{427005C5-DB26-4DB2-992C-9E080DE9202C}", AZ::Render::StreamBufferViewsBuilderInterface);
            ShaderStreamBufferViewsBuilder() = delete;
            ShaderStreamBufferViewsBuilder(const MeshFeatureProcessorInterface::MeshHandle& meshHandle);
            virtual ~ShaderStreamBufferViewsBuilder() = default;

            
            ////////////////////////////////////////////////////////////
            // StreamBufferViewsBuilderInterface overrides Start...
            bool AddStream(const char* semanticName, AZ::RHI::Format streamFormat, bool isOptional) override;
            AZ::u8 GetStreamCount() const override;
            AZStd::unique_ptr<ShaderStreamBufferViewsInterface> BuildShaderStreamBufferViews(
                uint32_t lodIndex, uint32_t meshIndex) override;
            const MeshFeatureProcessorInterface::MeshHandle& GetMeshHandle() const override;
            // StreamBufferViewsBuilderInterface overrides End...
            ////////////////////////////////////////////////////////////


        private:
            void FinalizeShaderInputContract();
            AZ::RHI::Ptr<AZ::RHI::BufferView> BuildShaderIndexBufferView(const AZ::RPI::ModelLod::Mesh& modelLodMesh) const;

            static constexpr char LogWindow[] = "ShaderStreamBufferViewsBuilder";

            const AZ::Render::MeshFeatureProcessorInterface::MeshHandle* m_meshHandle = nullptr;

            //! Used to keep track of all calls to @AddStream(...)
            struct StreamInfo
            {
                StreamInfo(const char* semanticName, AZ::RHI::Format streamFormat, bool isOptional)
                    : m_semanticName(semanticName)
                    , m_streamFormat(streamFormat)
                    , m_isOptional(isOptional)
                {
                }
                const char* m_semanticName;
                AZ::RHI::Format m_streamFormat;
                bool m_isOptional;
            };
            AZStd::vector<StreamInfo> m_streamsList;
            //! Instantiated the first time BuildShaderStreamBufferViews() is called.
            AZStd::unique_ptr<AZ::RPI::ShaderInputContract> m_shaderInputContract;
        };

    } // namespace Render
} // namespace AZ
