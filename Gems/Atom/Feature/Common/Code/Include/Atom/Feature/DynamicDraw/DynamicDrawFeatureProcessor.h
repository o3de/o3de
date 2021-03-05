/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystemInterface.h>

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Color.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace RHI
    {
        class BufferPool;
        class BufferView;
        class ImagePool;
    }
    namespace RPI
    {
        class ShaderResourceGroup;
    }

    namespace Render
    {
        //! This FeatureProcessor provides a way to push generic DrawPackets to the renderer.
        //! To access this feature from game/editor code, see DynamicDrawSystemInterface.h
        class DynamicDrawFeatureProcessor final
            : public RPI::FeatureProcessor
            , public RPI::DynamicDrawInterface
        {
        public:
            AZ_RTTI(DynamicDrawFeatureProcessor, "{51075139-CB74-4BED-8B6A-8440B53A9EAA}", AZ::RPI::FeatureProcessor);
            AZ_FEATURE_PROCESSOR(DynamicDrawFeatureProcessor);

            static void Reflect(AZ::ReflectContext* context);

            DynamicDrawFeatureProcessor();
            ~DynamicDrawFeatureProcessor();

            // FeatureProcessor overrides...
            void Activate() override;
            void Deactivate() override;
            void Simulate([[maybe_unused]] const RPI::FeatureProcessor::SimulatePacket& packet) override {};
            void Render(const RPI::FeatureProcessor::RenderPacket& packet) override;

            // DynamicDrawInterface overrides...
            void AddDrawPacket(AZStd::unique_ptr<const RHI::DrawPacket> drawPacket) override;
            RHI::Ptr<RHI::BufferPool>& GetInputAssemblyBufferHostPool() override;
            RHI::Ptr<RHI::BufferPool>& GetConstantBufferDevicePool() override;
            RHI::Ptr<RHI::ImagePool>& GetImagePool() override;
            void RegisterGeometryPreRenderNotificationHandler(RPI::DynamicDrawPreRenderNotificationHandler& handler) override;
            void UnregisterGeometryPreRenderNotificationHandler(RPI::DynamicDrawPreRenderNotificationHandler& handler) override;


        private:
            DynamicDrawFeatureProcessor(const DynamicDrawFeatureProcessor&) = delete;

            Data::Instance<AZ::RPI::ShaderResourceGroup> m_shaderResourceGroup;

            // Collects DrawPackets submitted by client code, and uses double-buffering to maintain ownership 
            // over them for a frame while they're in-flight in the renderer
            AZStd::vector<AZStd::unique_ptr<const RHI::DrawPacket>> m_drawPackets[2];
            u32 m_writeBufferIdx = 0;
            u32 m_submitBufferIdx = 1;
            AZStd::mutex m_mutexDrawPackets;

            RHI::Ptr<RHI::BufferPool>   m_inputAssemblyBufferHostPool;
            RHI::Ptr<RHI::BufferPool>   m_constantBufferDevicePool;
            RHI::Ptr<RHI::ImagePool>    m_imagePool;
            AZ::Event<int>              m_preRenderNotificationEvent;
            AZStd::mutex                m_eventMutex;
        };

    } // namespace Render
} // namespace AZ