/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/ImagePool.h>
#include <Atom/RHI/ScopeProducerFunction.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Image/AttachmentImage.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Reflect/Pass/EnvironmentCubeMapPassData.h>

namespace AZ
{
    namespace RPI
    {
        // pass that generates all faces of a Cubemap environment image at a specified point
        class ATOM_RPI_PUBLIC_API EnvironmentCubeMapPass final
            : public ParentPass
        {
        public:
            AZ_RTTI(EnvironmentCubeMapPass, "{B7EA8010-FB24-451C-890B-6E40B94546B9}", ParentPass);
            AZ_CLASS_ALLOCATOR(EnvironmentCubeMapPass, SystemAllocator);

            static Ptr<EnvironmentCubeMapPass> Create(const PassDescriptor& passDescriptor);
            virtual ~EnvironmentCubeMapPass();

            // operations
            void SetPosition(const Vector3& position) { m_position = position; }
            void SetDefaultView();

            // cubemap face size is always 1024, it is downsampled during the asset build by the ImageProcessor
            static constexpr u32 CubeMapFaceSize = 1024;
            static constexpr u32 NumCubeMapFaces = 6;

            // returns true if all faces of the cubemap have been rendered
            bool IsFinished() { return m_renderFace == NumCubeMapFaces; }

            // retrieves rendered cubemap texture data for all faces
            uint8_t* const* GetTextureData() const { return &m_textureData[0]; }

            // retrieves the rendered cubemap texture format
            RHI::Format GetTextureFormat() const { return m_textureFormat; }

        private:

            EnvironmentCubeMapPass() = delete;
            explicit EnvironmentCubeMapPass(const PassDescriptor& passDescriptor);

            // Pass overrides
            void CreateChildPassesInternal() override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            // world space position to render the environment cubemap
            Vector3 m_position;

            // descriptor for the transient output image
            RHI::ImageDescriptor m_outputImageDesc;

            // PassAttachment for the rendered cubemap face
            Ptr<PassAttachment> m_passAttachment;

            // the child pass used to drive rendering of the cubemap pipeline
            Ptr<Pass> m_childPass = nullptr;

            // attachment readback which copies the rendered cubemap faces to the m_textureData buffers
            AZStd::shared_ptr<AZ::RPI::AttachmentReadback> m_attachmentReadback;
            void AttachmentReadbackCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult);

            // camera and viewport state
            RPI::ViewPtr m_view = nullptr;
            RHI::Scissor m_scissorState;
            RHI::Viewport m_viewportState;

            // current cubemap render face index
            u32 m_renderFace = 0;

            // tracks if a readback has already been requested for a face
            bool m_readBackRequested = false;

            // array of texture data for the cubemap faces
            uint8_t* m_textureData[NumCubeMapFaces] = { nullptr };

            // format of the readback texture, set in the callback
            RHI::Format m_textureFormat = RHI::Format::Unknown;

            // tracks the number of frames elapsed before submitting the readback request
            // this is a work-around for a synchronization issue and will be removed after changing the readback mechanism
            // [ATOM-3844] Remove frame delay in EnvironmentCubeMapPass
            static constexpr u32 NumReadBackDelayFrames = 5;
            u32 m_readBackDelayFrames = 0;

            // lock for managing state between this object and the callback
            AZStd::mutex m_readBackLock;
        };

    }   // namespace RPI
}   // namespace AZ
